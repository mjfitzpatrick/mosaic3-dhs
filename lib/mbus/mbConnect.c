/*  
**  MBCONNECT.C -- Connect the current task to the message bus and initialize
**  the application appropriately.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pvm3.h>
#include <netdb.h>
#include <arpa/inet.h>          /* inet_ntoa() to format IP address */
#include <netinet/in.h>         /* in_addr structure */


#define _LIBMBUS_SOURCE_
#include "mbus.h"


MBHostPtr mbHost;		/* Host name structure.			*/
char *mbGetMBHost();

void mbInitMBHost ();
void mbFreeMBHost ();
void mbSetMBHost ();




/*---------------------------------------------------------------------------
**  MBUSCONNECT -- Connect the client to the message bus.  In the case of
**  a client we'll look first for a running Supervisor and notify them we've
**  come online.  The Client connection is complete when a MB_READY state
**  is sent.
*/
int
mbusConnect (char *whoAmI, char *group, int singleton)
{
    int     i, mytid, super, info, oldval;
    MBusPtr mbus = (MBusPtr) NULL;
    char    msg[SZ_LINE];
    pid_t   pid = getpid();


    /* Initialize host name.
    */
    mbInitMBHost ();
    mbSetMBHost ();


    /*  At this point we're only getting state information from the
     *  application cache, we don't talk to the message bus yet.
     */
    if (mbAppGet (APP_INIT)) {
	fprintf (stderr, "Application already initialized, resetting....\n");
        mbInitApp();

	/* FIXME:  ....disconnect old client.....*/
    }


    /*if (isSupervisor (whoAmI) ) {*/
        if (MB_DEBUG) printf ("Re-Initializing default message bus...\n");
        mbusInitialize (whoAmI, NULL);

	/*  Attach this process to the message bus.
	 */
    	if ((mytid = pvm_mytid()) < 0) {
	    pvm_perror (whoAmI);
	    exit (1);
    	}
    	oldval = pvm_setopt (PvmAutoErr, 0);  /* disable libpvm msgs */
    /*}*/

    /* No MBus pointer found meaning we have no connection established
     * or context yet.  If we're a SUPERVISOR, reinitialize.  Otherwise,
     * now is the time to connect and set the state.
     */
    mbus = mbAppGetMBus ();
    if (mbus == (MBusPtr) NULL) {
	/*  Message bus not initialized by supervisor.
    	 *  Attach this process to the message bus.
	 */
    	if ((mytid = pvm_mytid()) < 0) {
	    pvm_perror (whoAmI);
	    exit (1);
    	}
    	oldval = pvm_setopt (PvmAutoErr, 0);  /* disable libpvm msgs */

        /* Allocate an message bus structure.  We'll update the state from
	 * the supervisor once connected and setup.
         */
        if (MB_DEBUG) printf ("Allocating mbus struct ...\n");
        mbus = (MBusPtr) calloc (1, sizeof (MBus));
    }




    /**********************
     *  CLIENT CODE
     **********************/
    if (! isSupervisor (whoAmI)) {
        /*  If we're a Client, look for a Supervisor.  If we can't find a
         *  Supervisor the simply return an error, individuals client may 
         *  handle it differently.
         */
	for (i=MB_CONNECT_RETRYS; i; i--) {
            info = pvm_lookup (SUPERVISOR, -1, &super);
	    if (info == PvmNoEntry) {
	        fprintf (stderr, "Can't find Supervisor on msgbus, retrying\n");
		sleep (1);

	    } else {
	        if (MB_DEBUG) {
		    printf ("Supervisor on msgbus at tid=%d\n", super);
                    printf ("Registering '%s' with VM....\n", whoAmI);
	        }

                if (pvm_insert (whoAmI, -1, mytid) < 0) {
                    fprintf (stderr, "Register of '%s' failed....\n", whoAmI);
                    return (ERR);

                } else {
		    /* Send the Supervisor a CONNECT message. 
		     */
		    memset (msg, 0, SZ_LINE);
		    sprintf (msg, "{ tid=%d who=%s host=%s pid=%d }",
			mytid, whoAmI, mbGetMBHost(), pid);
		    mbusSend (SUPERVISOR, ANY, MB_CONNECT, msg);
		}
		break;
	    }
	}
	if (i == 0) {
	    if (MB_VERBOSE)
	        fprintf (stderr, "Supervisor not on msgbus, returning\n");
	    mytid *= (-1);
	    super = ERR;
	    /*return (((-1) * mytid)); */
	}


    /*********************
     * SUPERVISOR CODE
     **********************/
    } else if (isSupervisor (whoAmI)) {
	/* If we're a Supervisor, check whether we're already registered.
	 */
        info = pvm_lookup (SUPERVISOR, -1, &super);
	if (info == PvmBadParam) {
	    fprintf (stderr, "Supervisor lookup error, BadParam...\n");
	    return (ERR);

	} else if (info == PvmNoEntry) {
            /*  Supervisor does not exist */
	    if (MB_DEBUG) 
		fprintf (stderr, "Supervisor not found, registering...\n");
            if (pvm_insert (SUPERVISOR, -1, (super = mytid)) < 0) {
                fprintf (stderr, "Supervisor register failed....\n");
                return (ERR);
            }

	} else if (info >= 0) {
	    if (MB_DEBUG)
		fprintf (stderr, 
		    "Supervisor already registered at tid=%d, validating..\n",
		    super);

	    /* Try sending a message to the registered tid to see if it is
	     * alive.
	     */
	    if (mbusPing (super, 500) == OK) {
		if (singleton) {
		    /* tell other supervisor we're taking over.... */
		    printf ("got a ping reply....\n");
		} else {
		    fprintf (stderr, 
		        "ERROR: Supervisor already registered at tid=%d\n",
			super);
	            return (ERR);
		}

	    } else {
		/* The registered supervisor didn't respond, so delete it
		 * from the database and try again.
		 */
		extern int pvmreset();

	        if (MB_DEBUG)
		    fprintf (stderr, "Cleaning up earlier super at %d(%d)...\n",
			info, super);

		/* FIXME:  ....disconnect old supervisor.....*/
		(void) pvmreset (mytid, 1, "", 0);
    		if (pvm_delete (SUPERVISOR, info) != PvmOk)
		    if (MB_DEBUG)
        	        fprintf (stderr, "Supervisor cleanup failed\n");
    		if ((info = pvm_kill (super)) != PvmOk)
		    if (MB_DEBUG)
        	        fprintf (stderr, "Supervisor assasination failed\n");
		/* goto lookup_; */
	    }
        }


	/* Broadcast a message to any running clients telling them we're
	 * now running.
	 */
	memset (msg, 0, SZ_LINE); 
	sprintf (msg, "{ tid=%d who=%s host=%s pid=%d }", 
	    super, SUPERVISOR, mbGetMBHost(), pid);

	mbusBcast (CLIENT, msg, MB_CONNECT);
    }

    /* Save the Supervisor location and other bits about this client.
     */
    mbAppSet (APP_TID, mytid);
    mbAppSet (APP_STID, super);
    mbAppSet (APP_FD, mbGetMBusFD());
    mbAppSetName (whoAmI);
    mbAppSetMBus ((MBusPtr) mbus);


    /* Install the exit handler so we're sure to make a clean getaway.
     */
    atexit (mbusExitHandler);
	        
    if (MB_DEBUG)
	fprintf (stderr, "mbConnect: whoAmI='%s' group='%s'  mbus = 0x%x\n", 
	    whoAmI, group, (int) mbus);


    /* Join a specific group if specified.  A process may be part of
     * multiple groups and so we use the mbus routine the same as a
     * caller would.
     */
    if (group)
	mbusJoinGroup (group);


    /*  Return our tid.
     */
    return (mytid);
}


/*---------------------------------------------------------------------------
**  MBPARSECONNECTMSG --
*/
int
mbParseConnectMsg (char *msg, int *tid, char **who, char **host, int *pid)
{
    int N = sscanf (msg, "{ tid=%d who=%s host=%s pid=%d }", 
	tid, *who, *host, pid);

    return ((N == 4) ? OK : ERR);
}


/*---------------------------------------------------------------------------
**  MBECONNECTTOSUPER --  Establish a connection between the application
**  running on 'mytid' with the Supervisor running at pid@host with
**  'supertid'.
*/
int
mbConnectToSuper (int mytid, int supertid, char *host, int pid)
{
    char   msg[SZ_LINE];
    char   *whoAmI = mbAppGetName ();		/* app name		*/
    int    mysuper = mbAppGet (APP_STID);	/* current app's super  */
    int    mypid   = getpid();
    int	   i, info, super = -1;


    if (mbAppGet(APP_STID) == mytid) {
	fprintf (stderr, "ERROR: Supervisor connecting to itelf.\n");
	return (ERR);
    }


    /*  Look for a Supervisor and send a connect message.
     */
    for (i=MB_CONNECT_RETRYS; i; i--) {
        info = pvm_lookup (SUPERVISOR, -1, &super);
        if (info == PvmNoEntry || (mysuper != supertid)) {
            /* Supervisor not found, so connect to specified supertid.
	     */
            memset (msg, 0, SZ_LINE);
            sprintf (msg, "{ tid=%d who=%s host=%s pid=%d }",
                abs(mytid), whoAmI, host, mypid);
            if (MB_DEBUG) printf ("Supervisor msg '%s'\n", msg);
            if (mbusSend (SUPERVISOR, ANY, MB_CONNECT, msg) != ERR)
		break;
            sleep (1);

        } else {
            if (MB_DEBUG)
                printf ("Supervisor already on msgbus at tid=%d\n", super);

	    /* FIXME -- Need to disconnect from old Supervisor.
            memset (msg, 0, SZ_LINE);
            sprintf (msg, "{ tid=%d who=%s host=%s pid=%d }",
                abs(mytid), whoAmI, host, mypid);
            if (mbusSend (SUPERVISOR, ANY, MB_DISCONNECT, msg) != ERR)
		break;
	     */
            break;
        }
    }

    if (super < 0) {
        if (MB_VERBOSE) fprintf(stderr,"Supervisor not on msgbus, returning\n");
        super = ERR;
    } else {
	mbAppSet (APP_TID, abs (mytid));
	mbAppSet (APP_STID, super);
    }

    return (super);
}



/* Mini-interface for managing the host name of a message sender.  By default
** we'll always use the actual host name, however we can set a simulated name
** and deliver that instead.  Note that we can call the routine to initialize
** and set the simulated host before connecting.
*/

void 
mbInitMBHost ()
{
    static int initialized = 0;

    if (initialized)
	return;

    mbHost = calloc (1, sizeof (MBHost));

    memset (mbHost->actual, 0, SZ_FNAME); 
    memset (mbHost->simulated, 0, SZ_FNAME); 
    mbHost->use_sim = 0;
    initialized++;
}

void 
mbFreeMBHost ()
{
    free (mbHost);
}


void
mbSetMBHost ()
{
    char  hname[SZ_FNAME], *ip;
    struct hostent *host;       	/* host information */
    struct in_addr x_addr;      	/* internet address */


    memset (hname, 0, SZ_FNAME); 
    gethostname (hname, SZ_FNAME);

    for (ip=hname;  *ip; ip++) { 	/* truncate the doman name, if any */
	if (*ip == '.') {
	    *ip = '\0';
	    break;
	}
    }

    if ((host = gethostbyname(hname)) == (struct hostent *)NULL) {
        printf("mbSetHost(); nslookup failed on '%s'\n", hname);
        exit(1);
    }
    x_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
    sprintf (mbHost->ip_addr, "%s", inet_ntoa(x_addr));

    memset (mbHost->actual, 0, SZ_FNAME); 
    strncpy (mbHost->actual, hname, strlen(hname));
}


void
mbSetSimHost (char *simhost, int flag)
{
    memset (mbHost->simulated, 0, SZ_FNAME); 
    strncpy (mbHost->simulated, simhost, strlen(simhost));

    memset (mbHost->ip_addr, 0, SZ_FNAME); 
    strcpy (mbHost->ip_addr, "127.0.0.1");

    mbHost->use_sim = flag;
}


char *
mbGetMBHost ()
{
    return (mbHost->use_sim ? mbHost->simulated : mbHost->actual);
}
