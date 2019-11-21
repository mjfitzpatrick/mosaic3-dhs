#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/un.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Tcl/tcl.h>

/*
#if !defined(_sockUtil_H_)
#include "sockUtil.h"
#endif
#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
*/
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif


#include "super.h"
#include "dcaDhs.h"


#define CALLBACK        0
#define READY           1
#define STATUS          2
#define QUIT            3
#define DCA_MSG         4
#define GUI_MSG         5

#define MAXCONN         5
#define MAX_TRY         5

#define DEF_DOMAIN	"tuc.noao.edu"
/*
#define DEF_DOMAIN	"kpno.noao.edu"
#define DEF_DOMAIN	"ctio.noao.edu"
*/

#define SOCK_DEBUG \
	(getenv("SOCK_DEBUG")!=NULL||access("/tmp/SOCK_DEBUG",F_OK)==0)


void  sup_connectClient(), sup_disconnectClient(), supClientIO();

XtPointer       sup_addInput();

extern int console;



/*****************************************************************************
**  SUPSOCKET.C --  Procedures to support the connection to the Supervisor
**  on the main public socket, as well as client communications assigned to
**  callback port numbers.
**
*/



/*****************************************************************************
**  SUPOPENINET -- Set up an inet port for incoming client connections.
*/
clientIoChanPtr
supOpenInet (supDataPtr sup)
{
    register int s = 0;
    register clientIoChanPtr chan;
    struct sockaddr_in sockaddr;
    int     reuse = 1;

    /* Setting the port to zero disables inet socket support. */
    if (sup->port <= 0)
        return (NULL);

    /*
    if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        goto err;
    */
    if ((s = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        goto err;

    memset ((void *)&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons((short)sup->port);
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
        sizeof(reuse)) < 0)
            goto err;

    if (bind (s, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
        goto err;

    if (listen (s, MAXCONN) < 0)
        goto err;

    /* Allocate and fill in i/o channel descriptor. */
    if ((chan = clientGetChannel (sup))) {
        chan->sup = (XtPointer) sup;
        chan->datain = s;
        chan->dataout = s;
        chan->connected = 0;

        /* Register connectClient callback. */
        chan->id = sup_addInput (sup, s, sup_connectClient, (XtPointer)chan);
        return (chan);
    }

err:
    if (errno == EADDRINUSE)
        fprintf (stderr,"Supervisor: inet port %d already in use - disabled\n",
            sup->port);
    else
        fprintf (stderr, "Supervisor: can't open inet socket %d, errno=%d\n",
            sup->port, errno);

    sup->port = 0;
    if (s)
        close (s);

    return (NULL);
}




/*****************************************************************************
**  SUPCLOSEINET -- Close down the ISM protocol module.  Disconnect all
**  connected clients and close the port.
*/
void
supCloseInet (supDataPtr sup)
{
    register clientIoChanPtr chan = &(sup->pub_chan);
    register int i;


    /* Send a 'quit' message to all connected clients. 
     */
    for (i=0, chan=NULL; i < XtNumber(sup->pub_client); i++) {
        chan = &sup->pub_client[i];
        if (chan->id) {
            sup_removeInput (sup, chan->id);
            chan->id = NULL;
	    chan->connected = 0;
        }
        close (chan->datain);
    }

    /* Close the public request socket. 
     */
    chan = &(sup->pub_chan);
    if (chan->id) {
        sup_removeInput (sup, chan->id);
        chan->id = NULL;
	chan->connected = 0;
    }
    close (chan->datain);
}



/**************************************************************************
** 
**  Low-level procedures
**
**  FIXME:  Need a SIGPIPE handler for the read/write .....
**
**************************************************************************/


/*****************************************************************************
**  SUP_CONNECTCLIENT -- Called when a client has attempted a connection on
**  a socket port.  Accept the connection and set up a new i/o channel to
**  communicate with the new client.
*/
void
sup_connectClient (
  clientIoChanPtr chan,
  int *source,
  XtPointer id
)
{
    register supDataPtr sup = (supDataPtr) chan->sup;
    register int s;


    /* Accept connection. */
    if ((s = accept ((int)*source, (struct sockaddr *)0, (int *)0)) < 0)
        return;

    if (fcntl (s, F_SETFL, O_NDELAY) < 0) {
	fprintf (stderr, "connectClient:  fcntl() FAILS, returning\n");
        close (s);
        return;
    }

    if (SOCK_DEBUG) 
	fprintf (stderr, "Connecting client on %d, sock=%d...\n", *source, s);

    /* Fill in the client i/o channel descriptor. */
    chan->datain = s;
    chan->dataout = s;
    chan->connected = 1;
    chan->id = sup_addInput (sup, s, supClientIO, (XtPointer)chan);

    if (SOCK_DEBUG) 
	fprintf (stderr, "Client ready on %d, port=%d...\n", s, chan->port);
}


/*****************************************************************************
**  SUP_DISCONNECTCLIENT -- Called to close a client connection when EOF is
**  seen on the input port.  Close the connection and free the channel
**  descriptor.
*/
void
sup_disconnectClient (clientIoChanPtr chan)
{
    register supDataPtr sup = (supDataPtr) chan->sup;
    int clid = procFindClient (sup, chan->port, -1);


    if (SOCK_DEBUG) 
	fprintf (stderr, "Disconnecting client on %d, port=%d, clid=%d\n", 
	    chan->datain, chan->port, clid);

    close (chan->datain);
    if (chan->id) {
        sup_removeInput (chan->sup, chan->id);
        chan->port = -1;
	chan->connected = 0;
        chan->id = NULL;
    }

    (void) procDisconnect (sup, clid);

    chan->datain = 0;
}


/*****************************************************************************
**  SUPCLIENTIO -- Xt file i/o callback procedure, called when there is input
**  pending on the (socket-based) data stream to the client.
*/

#define SZMSG           120

void
supClientIO (
  clientIoChanPtr chan,
  int       *fd_addr,
  XtInputId *id_addr
)
{
    register supDataPtr sup = (supDataPtr) chan->sup;
    clientIoChanPtr new_chan;
    int     datain = *fd_addr;
    int     dataout = chan->dataout;
    int	    s, count = 0, pid = 0, clid = -1;
    char    name[SZ_FNAME], host[SZ_FNAME], id[SZ_LINE];
    char    path[SZ_FNAME], hostname[SZ_FNAME];
    char    message[2*SZ_MSGBUF+1], buf[SZ_MSGBUF+1];
    char    status[SZ_MSGBUF+1], domain[SZ_FNAME];
    char    *ip, *op, *text = NULL;
    struct  msgHeader *msgh;


    /* Read the (text) message.
    */
    memset (buf, 0, SZMSG+1);
    count = dcaRecv(datain, buf, SZMSG);
    if (count == EOF) {
	if (console)
            fprintf (stderr,"supClientIO: EOF seen on %d (fd=%d) (port=%d)\n",
		datain, chan->datain, chan->port);
	sup_disconnectClient (chan);
	return;

    } else if (count != SZMSG) {
	if (console)
            fprintf (stderr,"supClientIO: Error receiving message: sz: %d\n",
		count);
        return;
    } 


    bcopy (buf, message, SZMSG);
    memset (chan->msgbuf, 0, SZ_MSGBUF);
    text = buf;

    /*  Messages 
     *  
     *     CALLBACK     - Negotiate a connection on another socket
     *     READY	- client is ready to begin processing
     *     STATUS	- client status message
     *     QUIT	        - client is shutting down and disconnecting
     *
     *     MSG	        - send a message to another object
     */
    if (SOCK_DEBUG)
        fprintf (stderr, "sup_io(%d): message '%s' on fd %d\n",
	    chan->port, text, datain);

    switch (supMsgType (text)) {
    case CALLBACK:

        /* Get the requesting client's name. */
    	memset (id, 0, SZ_LINE);
        sscanf (text, "sup_connect %s", id);
	/*
        sscanf (id, "%s@%s:%d", name, host, pid);
	*/

	for (ip=id, op=name; *ip && *ip != '@'; ip++) *op++ = *ip;
	ip++, *op = '\0';

	for (       op=host; *ip && *ip != ':'; ip++) *op++ = *ip;
	ip++, *op = '\0';

	pid = atoi (ip);

        /* Get a new i/o channel. */
        if ((new_chan = clientGetChannel (sup))) {
    	    struct sockaddr_in sockaddr;
    	    int    backlog = 5, reuse = 1;
	    char   *ip = (char *) NULL;
	    char   *ip_addr = (char *) NULL;

            /* Get path to be used for the inet socket.  */
    	    memset (domain, 0, SZ_FNAME);
    	    (void) getdomainname (domain, SZ_FNAME);
    	    if (strncmp (domain, "(none)", 6) == 0)
        	strcpy (domain, DEF_DOMAIN);

    	    memset (hostname, 0, SZ_FNAME);
    	    if (gethostname (hostname, SZ_FNAME) < 0)
    	        strcpy (hostname, "localhost");

	    for (ip = hostname; *ip && *ip != '.'; ip++)
		;
	    *ip = '\0';
	    ip_addr = supHostLookup (sup, hostname);

    	    memset (path, 0, SZ_FNAME);
	    if (index (hostname, (int)'.') == (char *)NULL)
                sprintf (path, "%s:%d", hostname, new_chan->port);
	    else
		/* Use FQDF from the hostname. */
                sprintf (path, "%s.%s:%d", hostname, domain, new_chan->port);

    	    if (SOCK_DEBUG) {
    	        fprintf (stderr, "sup_io: CONNECT '%s' on socket '%s'\n",
		    name, path);
		fprintf (stderr, "sup_io: CONNECT '%s' h='%s' on socket '%s'\n",
		    (ip_addr ? ip_addr : "no-ip"), hostname, path);
	    }

    	    /* Create the socket. 
    	     */
            if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        	if (SOCK_DEBUG) 
		    fprintf (stderr, "CALLBACK: socket() FAILS, port=%d\n",
			new_chan->port);
                goto err;
	    }
            memset ((void *)&sockaddr, 0, sizeof(sockaddr));
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_port = htons((short)new_chan->port);
            sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                sizeof(reuse)) < 0)
                    goto err;

    	    if (SOCK_DEBUG)
        	fprintf (stderr, "sup_io: new_chan port=%d on fd %d\n",
		    new_chan->port, s);

            if (bind (s,(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0) {
        	if (SOCK_DEBUG) 
		    fprintf (stderr, "CALLBACK: bind() FAILS, port=%d\n",
			new_chan->port);
                goto err;
	    }
            if (listen (s, backlog) < 0) {
        	if (SOCK_DEBUG) 
		    fprintf (stderr, "CALLBACK: listen() FAILS, port=%d\n",
			new_chan->port);
                goto err;
	    }

            strncpy (new_chan->name, name, strlen(name));
            strncpy (new_chan->host, host, strlen(host));
            new_chan->sup = sup;
            new_chan->connected = 0;
            new_chan->datain = s;
	    new_chan->dataout = s;
	    new_chan->pid = pid;
            new_chan->id = sup_addInput (sup, s, sup_connectClient,
		(XtPointer)new_chan);
        }
/*
        strcpy (chan->name, name);
        strcpy (chan->host, host);
        chan->pid = pid;
*/

        /* Now tell the client to call us back on the new channel */
        memset (buf, 0, SZMSG);
        sprintf (buf, "sup_reconnect %s", path);
        clientSockWrite (dataout, buf, SZMSG);

        /* Hang up the current connection (on the public port). */
	if (SOCK_DEBUG) {
	    fprintf (stderr, "CALLBACK: hanging up port=%d  on fd=%d\n", 
		chan->port, chan->datain);
	}
        sup_disconnectClient (chan);
        break;
        
err:
        if (SOCK_DEBUG) {
	    fprintf (stderr, "CALLBACK: Error creating socket.\n");
	    fprintf (stderr, "CALLBACK: Error %d: '%s'\n", 
		errno, strerror(errno));
        }
        sup_disconnectClient (chan);
        break;

    case READY:
    	memset (id, 0, SZ_LINE);
    	memset (chan->name, 0, SZ_FNAME);
    	memset (chan->host, 0, SZ_FNAME);

        sscanf (text, "sup_ready rc_%s", id);
        procBreakoutClientID (id, chan->name, chan->host, &chan->pid);

        if (SOCK_DEBUG) 
    	    fprintf (stderr, "READY: sup_ready '%s' on '%s', port=%d, pid=%d\n",
		chan->name, chan->host, chan->port, chan->pid);

        /* Add the client to the process table.
         */
        procAddClient (sup, chan, chan->name, chan->host, chan->port, 999,
    	    chan->pid, "Ready", 1);
        break;

    case STATUS:
        /* Copy the status string...
         */
        strcpy (status, &text[11]);

        if (SOCK_DEBUG)
    	    fprintf (stderr, "Status: '%s'\n", status);

        if ((clid = procFindClient (sup, chan->port, -1)) < 0) {
    	    fprintf (stderr, "Error finding client in process table.\n");
    	    break;
        }
        if (procUpdateStatus(sup, clid, status, 1) < 0) {
    	    fprintf (stderr, "Error updating status table.\n");
    	    break;
        }
        break;

    case QUIT:
        if (SOCK_DEBUG)
    	    fprintf (stderr, "QUIT: quit '%s' on %d\n", 
		chan->name, chan->datain);
        sup_disconnectClient (chan);
        break;

    case DCA_MSG:
	if (SOCK_DEBUG) {
            msgh = (struct msgHeader *) buf;
            fprintf (stderr, "s_io: DCA Message:  type=%d\n", msgh->type);
        }
	superHandleClient (sup, datain, message);
        break;

    case GUI_MSG:
        fprintf (stderr, "s_io: GUI Message '%s'\n", text);
        break;

    default:
        fprintf (stderr, "s_io: Unknown message '%s'\n", text);
        break;
    }
}



/*****************************************************************************
** SUPMSGTYPE -- Determine the message type.
*/
int
supMsgType (char *message)
{
    register char *ip = message;


    if (strncmp (ip, "sup_", 4) != 0)
        return (DCA_MSG);
    else if (SOCK_DEBUG) 
	fprintf (stderr, "msgType: '%-11.11s'\n", message);

    for (ip=message; isspace(*ip); ip++) ;          /* skip whitespace */

    if (strncmp (ip, "sup_connect", 11) == 0)
        return (CALLBACK);
    else if (strncmp (ip, "sup_quit", 8) == 0)
        return (QUIT);
    else if (strncmp (ip, "sup_ready", 9) == 0)
        return (READY);
    else if (strncmp (ip, "sup_status", 10) == 0)
        return (STATUS);
    else if (strncmp (ip, "sup_send", 8) == 0)
        return (GUI_MSG);
    else 
        return (DCA_MSG);
}


/*****************************************************************************
** SUPHOSTLOOKUP ---   Look up the given host in the name table and return 
** the corresponding IP address.
*/
char *
supHostLookup (supDataPtr sup, char *name)
{
    int  i, nhosts = sup->numHostTable;
    hostTablePtr ht = &sup->hosts[0];
    char host[SZ_FNAME];


    strcpy (host, supNeuter (name));
    for (i=0; i < nhosts; i++) {
        ht = &sup->hosts[i];
	if (strcmp (ht->name, host) == 0) { 
	    printf ("supHostLookup:   MATCH '%s' >>>>>>>>>>>>>>>>>\n", host);
	    return (ht->ip_addr);
	}
    }
    printf ("supHostLookup:   NO MATCH '%s' >>>>>>>>>>>>>>>>>\n", host);

    return ((char *) NULL);
}


/*****************************************************************************
** SUPNEUTER -- Neutralize a host name, i.e. remove the domain and any
** location-specific extensions.
*/
char neuterHost[SZ_FNAME];

char *
supNeuter (char *name)
{
    int i;
    char *ip, *op, host[SZ_FNAME];
    char *extns[] = { "-cr", 
		      "-fr", 
		      "-4m", 
		      NULL };


    /* Remove any domain information from the name.
    */
    memset (host, 0, SZ_FNAME);
    op = &host[0];
    for (ip = name; *ip; ip++) {
	*op++ = *ip;
	if (*ip == '.') {
	    *ip = '\0';
	    break;
	}
    }
    
    /* Now strip off any location-specific extensions from the name.  At
    ** this point, the 'host' string will have no domain name.
    */
    for (ip=host; *ip; ip++) {
	if (*ip == '-') {		/* extns all begin with a '-' */
	    for (i=0; extns[i]; i++) {
	        if (strncasecmp(extns[i], ip, strlen(extns[i])) == 0) {
	            *ip = '\0';
	            break;
	        }
	    }
	}
    }

    memset (neuterHost, 0, SZ_FNAME);
    sprintf (neuterHost, "%s", host);

    return (neuterHost);
}


/*****************************************************************************
** CLIENTGETCHANNEL --- Get a i/o channel descriptor.
*/
clientIoChanPtr
clientGetChannel (supDataPtr sup)
{
    register int i;

    for (i=0;  i < XtNumber(sup->pub_client); i++) {
	if (SOCK_DEBUG)
	    fprintf (stderr, "getChannel: %d:  connected=%d  port=%d\n", 
    		i, sup->pub_client[i].connected, sup->pub_client[i].port);
        if (!sup->pub_client[i].connected) {
            sup->pub_client[i].id = (XtPointer) i;
            sup->pub_client[i].port = sup->port + 1 + sup->chan_offset++;

	    if (SOCK_DEBUG)
		fprintf (stderr, "getChannel: returning %d:  id=%d  port=%d\n", 
		    i, (int) sup->pub_client[i].id, sup->pub_client[i].port);
            return (&sup->pub_client[i]);
        }
    }

    return (NULL);
}


/*****************************************************************************
** CLIENTSOCKREAD -- Read exactly "n" bytes from a client socket descriptor. 
*/
int
clientSockRead (int fd, void *vptr, int nbytes)
{
    char    *ptr = vptr;
    int     nread = 0, nleft = nbytes, nb = 0;

    while (nleft > 0) {
        if ( (nb = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nb = 0;             /* and call read() again */
            else
                return(-1);
        } else if (nb == 0)
            break;                  /* EOF */
        nleft -= nb;
        ptr   += nb;
        nread += nb;
    }

    return (nread);                 /* return no. of bytes read */
}


/*****************************************************************************
** CLIENTSOCKWRITE -- Write exactly "n" bytes to a client socket descriptor. 
*/
int
clientSockWrite (int fd, void *vptr, int nbytes)
{
    char    *ptr = vptr;
    int     nwritten = 0,  nleft = nbytes, nb = 0;


    /* Send the message. */
    while (nleft > 0) {
        if ( (nb = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR)
                nb = 0;             /* and call write() again */
            else
                return (-1);        /* error */
        }
        nleft    -= nb;
        ptr      += nb;
        nwritten += nb;
    }

    return (nwritten);
}
