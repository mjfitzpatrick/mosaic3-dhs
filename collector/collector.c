#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

#include "dcaDhs.h"
#include "mbus.h"


/*****************************************************************************
**  COLLECTOR --  Listen for connection from a client, stuffing the data
**  into the Shared Memory Cache.
**  
**  The Collector must be able to accept input from three different sources:  
**  
**  	1) The socket connection to the PAN who is sending us header/pixel
**	   data as well as configuration messages
**  	2) Commands from the Supervisor requesting status or changing state,
**	   this is done using the Message Bus.  (This mode is not used heavily
**	   so we can limit the amount of processing the Collector needs to do,
**	   however we do use this channel for sending status to the Supervisor)
**  	3) Console command input (used for debugging and in engineering modes)
**  
**  We use the MBUS interface input handler to manage the I/O of the process,  
**  basically running in a select() loop over the input channels and calling
**  a local I/O handler based on the source.
**  	Once the received data are stored to the Shared Memory Cache we detach
**  from those pages and no longer worry about them.  The SMC interface will
**  block us if we exceed available resources.
**	The process may be a "temporary Collector" if the '-temp' flag is set.
**  This means that the task will exit when the client closes the connection
**  rather than wait for another connection.  We use the collector in this
**  mode in cases where we need to temporarily add resources in order to keep
**  pace with the instrument readout (e.g. multiple PAN savers overloading
**  the existing connections.
**
****************************************************************************/


#include <stdio.h>		/* for printf() and fprintf()           */
#include <sys/types.h>		/* for SO_REUSEADDR                     */
#include <sys/socket.h>		/* for socket(), bind(), and connect()  */
#include <arpa/inet.h>		/* for sockaddr_in and inet_ntoa()      */
#include <stdlib.h>		/* for atoi() and exit()                */
#include <string.h>		/* for memset()                         */
#include <unistd.h>		/* for close()                          */

#include "smCache.h"


#define DEF_PORT	4100	/* Default connection port		*/

#define	SZ_MSG		64


smCache_t *smc = (smCache_t *) NULL;
smcPage_t *page = (smcPage_t *) NULL;
smcSegment_t *seg = (smcSegment_t *) NULL;

int procDebug     = 0;			/* main process debug flag      */
int console       = 0;			/* display console messages?    */
int use_mbus      = 1;			/* Use the msgbus for status?   */
int mb_tid        = 0;			/* message bus tid              */
int port          = DEF_PORT;		/* default data port		*/
int tempCollector = 0;			/* Is this a temp collector?	*/

char colID[SZ_FNAME];			/* Collector ID string		*/
int expPageNum 	  = 0;			/* Exposure page number		*/
int sim_mode  	  = 0;			/* Simulation mode		*/


					/* Function prototypes          */
int   colOpenPort (unsigned short port);	
int   col_connectClient (int socket, void *client_data);	
int   col_disconnectClient (int socket);

					/* I/O Handlers			*/
int   colPanHandler  (int fd, void *client_data); 
int   colPanConnect  (int fd, void *client_data); 
int   mbusMsgHandler (int fd, void *client_data);
int   consMsgHandler (int fd, void *client_data);

void  colUsage (void);

					/* External Functions		*/
extern smCache_t *smcOpen();		
extern smcPage_t *smcGetPage();


int
main(int argc, char *argv[])
{
    int servSock;			/* Socket descriptor for server */
    int clntSock;			/* Socket descriptor for client */
    int ServPort;
    struct sockaddr_in ClntAddr;	/* Client address 		*/
    unsigned int clntLen;		/* Len of client addr structure */
    char msg[SZ_MSG], config[SZ_FNAME];
    int   i, level, mb_fd = 0, port = DEF_PORT, debug = 0;



    /* Process the command-line arguments.
    */
    bzero (colID, SZ_FNAME);
    for (i = 1; i < argc; i++) {
	if (strncmp(argv[i], "-help", 3) == 0) {
	    colUsage ();
	    return (0);

	} else if (strncmp(argv[i], "-port", 2) == 0) {
	    port = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-id", 3) == 0) {
	    strcpy (colID, argv[++i]);

	} else if (strncmp(argv[i], "-debug", 3) == 0) {
	    level = atoi(argv[++i]);
	    procDebug = (console ? (level - 10) : level);

	} else if (strncmp(argv[i], "-console", 3) == 0) {
	    console++;
	    procDebug += 30;

	} else if (strncmp(argv[i], "-host", 3) == 0) {
	    mbInitMBHost();
	    if ((argc - i) > 1 && (argv[i+1][0] != '-'))
	        mbSetSimHost (argv[++i], 1);
	    else {
		fprintf (stderr, "Error: '-host' requires an argument\n");
		exit (1);
	    }

	} else if (strncmp(argv[i], "-temp", 3) == 0) {
	    tempCollector++;

	} else if (strncmp(argv[i], "-mbus", 3) == 0) {
	    use_mbus = 1;

	} else if (strncmp(argv[i], "-nombus", 5) == 0) {
	    use_mbus = 0;

	} else if (strncmp(argv[i], "-sim", 4) == 0) {
	    sim_mode = 1;

	} else {
	    colUsage ();
	    return (0);
	}
    }

    /* Sanity check -- Require a Collector ID string.
    */
    if (colID[0] == (char)NULL) {
	fprintf (stderr, "\nERROR: No Collector ID specified!\n\n");
	colUsage ();
	return (0);

    } else if (console)
	fprintf(stderr, "Collector ID:  '%s'....\n", colID);


    /*  Open the Shared Memory Cache.
    */
    sprintf(config, "debug=%d", debug);
    if ((smc = smcOpen(config)) == (smCache_t *) NULL) {
	fprintf(stderr, "Error opening cache, invalid file?.\n");

    } else if (console)
	fprintf(stderr, "Attached to SMC....\n");


    /*  Open the data port and create the client socket.
    */
    ServPort = (unsigned short) port;
    servSock = colOpenPort(ServPort);


    /*  Connect to the message bus for status output.  If we're using the
    **  message bus use the input event handler to process the data,
    **  otherwise do it manually below.  Note that if we call the handler
    **  here we won't return, the client will be connected/disconnect by the
    **  handler and the colPanHandler() procedure will be called when data is
    **  ready on the socket stream.
    */
    if (use_mbus) {
	if ((mb_tid = mbusConnect("Collector", "Collector", FALSE)) <= 0) {
	    fprintf(stderr, "ERROR: Can't connect to message bus.\n");
	    use_mbus = 0;
	}

	if ((mb_fd = mbGetMBusFD()) >= 0) {	/* Add the input handlers. */
	    if (console)
	        fprintf(stderr, "Adding mbus input handler....\n");
	    mbusAddInputHandler (mb_fd, mbusMsgHandler, NULL);
	} else {
	    fprintf(stderr, "ERROR: Can't install MBUS input handler.\n");
	    use_mbus = 0;
	}

        /*  If we're running on a console, accept input from the STDIN so
	**  in engineering mode we can interact with the task.
	*/
	if (console) {
	    fprintf(stderr, "Adding console input handler....\n");
	    mbusAddInputHandler (fileno(stdin), consMsgHandler, NULL);
	}

	/*  Add the input handler for the client connection on the socket.
	*/
	if (console)
	    fprintf(stderr, "Adding socket input handler....\n");
	/*
	mbusAddInputHandler (servSock, colPanHandler, NULL);
	*/
	mbusAddInputHandler (servSock, col_connectClient, NULL);

	/*  Tell the supervisor what our connection port is.
	*/
	memset (msg, 0, SZ_MSG);
	sprintf (msg, "port %d %s", port, colID);
	mbusSend(SUPERVISOR, ANY, MB_SET, msg);

	/*  Send initial status to the Supervisor for display.
	*/ 
	mbusSend(SUPERVISOR, ANY, MB_STATUS, "Waiting for PAN connect...");
	if (console)
	    fprintf (stderr, "Waiting for PAN to connect, id='%s'...\n", colID);

	/*  Begin processing I/O events.  Note: This never returns....
	*/ 
	mbusSend(SUPERVISOR, ANY, MB_READY, "READY COLLECTOR");
        mbusAppMainLoop (NULL);                

    } else {

        /*  The Message bus isn't being used, so connect client manually and
        **  handle the input.  Loop forever ....
        */
        for (;;) {
	    DPRINTF(40, procDebug, "Ready to connect: Handling client %s\n",
		inet_ntoa(ClntAddr.sin_addr));

	    /*  Set the size of the in-out parameter and wait for a client to
	    **  connect 
	    */
	    clntLen = sizeof(ClntAddr);
	    if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr,
			       &clntLen)) < 0) {
	        perror("accept() failed");
	        exit(1);
	    }

	    /* clntSock is connected to a client! 
	    */
	    DPRINTF(40, procDebug, "Connecting client %s\n",
		inet_ntoa(ClntAddr.sin_addr));

	    /* Process all data on the connection until the client disconnects.
 	    */
	    colPanHandler (clntSock, smc);
        }

    }	/* No message bus	*/

    return (0);
}


/* Print task usage help.
*/
void
colUsage ()
{
    printf ("Usage: \n");
    printf ("\tcollector -p port -id <name> -console -temp \n");
    printf ("\t          -[no]mbus -debug lev\n");
    printf ("\n");
}
