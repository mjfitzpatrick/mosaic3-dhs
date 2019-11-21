#include <stdio.h>		/* for printf() and fprintf()           */
#include <sys/types.h>		/* for SO_REUSEADDR                     */
#include <sys/socket.h>		/* for socket(), bind(), and connect()  */
#include <arpa/inet.h>		/* for sockaddr_in and inet_ntoa()      */
#include <fcntl.h>		/* for fcntl()      			*/
#include <stdlib.h>		/* for atoi() and exit()                */
#include <string.h>		/* for memset()                         */

#include "mbus.h"


#define MAXCONN 	5	/* Max connection requests              */

extern	int	console, procDebug;

extern	int	colPanHandler (int socket, void *client_data);




/*************************************************************************
**  colOpenPort -- Create a socket connection on the specified port we'll
**  use to accept client connections.
*/
int
colOpenPort(unsigned short port)
{
    struct sockaddr_in sockaddr;	/* local address        */
    int sock = 0;			/* socket to create     */
    int reuse = 1;			/* make reusable socket */
    extern int procDebug;		


    /* Create socket for incoming connections.
     */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	goto err_;

    /* Construct local address structure.
     */
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);			/* local port    */
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* any interface */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse,
		   sizeof(reuse)) < 0)
	goto err_;


    if (fcntl (sock, F_SETFL, O_NDELAY) < 0) {
        close (sock);
        goto err_;
    }


    /* Bind to the local address.
     */
    if (bind(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0)
	goto err_;

    /* Mark the socket so it will listen for incoming connections.
     */
    if (listen(sock, MAXCONN) < 0)
	goto err_;

    if (procDebug)
	fprintf (stderr, "Opened socket on port %d, fd=%d\n", port, sock);

    /* Add the connection handler.
    mbusAddInputHandler (sock, col_connectClient, NULL);
    */

    return sock;

  err_:
    perror("colOpenPort:  Error opening Collector inet port");
    exit(1);
}


/*****************************************************************************
**  COL_CONNECTCLIENT -- Called when a client has attempted a connection on
**  a socket port.  Accept the connection and set up a new i/o channel to
**  communicate with the new client.
*/
void
col_connectClient (int source, void *client_data)
{
    register int s;


    if (console) 
	fprintf (stderr, "Connecting client on %d...\n", source);

    /* Accept connection. */
    if ((s = accept ((int)source, (struct sockaddr *)0, (int *)0)) < 0)
        return;

    if (fcntl (s, F_SETFL, O_NDELAY) < 0) {
        close (s);
        return;
    }

    /* Fill in the client i/o channel descriptor. */
    mbusAddInputHandler (s, colPanHandler, NULL);

    mbusSend(SUPERVISOR, ANY, MB_SET, "connected 1");
    mbusSend (SUPERVISOR, ANY, MB_STATUS, "PAN connected...");

    if (console) 
	fprintf (stderr, "Client ready on %d...\n", s);
}


/*****************************************************************************
**  COL_DISCONNECTCLIENT -- Called to close a client connection when EOF is
**  seen on the input port.  Close the connection and free the channel
**  descriptor.
*/
void
col_disconnectClient (int source)
{
    if (console) 
	fprintf (stderr, "Disconnecting client on %d...\n", source);

    mbusRemoveInputHandler (source);
    mbusSend(SUPERVISOR, ANY, MB_SET, "connected 0");
    mbusSend (SUPERVISOR, ANY, MB_STATUS, "PAN disconnected...");
    close (source);
}
