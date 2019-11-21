#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
/* Function prototypes */
#ifdef __STDC__
#include <stddef.h>
#include <stdlib.h>
#endif

#include <mbus.h>


/* Default Values. */
#define	DEF_PORT	2005
#define	DEF_HOST	"localhost"
#define	DEF_DEV		"localhost:2005"

#define TRUE		1
#define FALSE		0
#define OK		0
#define ERR		-1



char	buf[1024];				/* temp buffer		*/

char	*myTimestamp();

extern int errno;


main (int argc, char **argv)
{
    int i, loop = 1, pause = 1;
    char  name[64];


    strcpy (name, "fred\0");			/* initialize		*/
    for (i=1; i < argc; i++) {
        if (strncmp (argv[i], "-name", 2) == 0) {
            strcpy (name, argv[++i]);

        } else if (strncmp (argv[i], "-delay", 3) == 0) {
	    pause = atoi (argv[++i]); 

        } else if (strncmp (argv[i], "-pan", 2) == 0) {
	    panSim (name, pause, loop);

        } else if (strncmp (argv[i], "-rtd", 2) == 0) {
	    rtdSim ("RTD", pause, loop);
        } else if (strncmp (argv[i], "-dca", 2) == 0) {
	    dcaSim ("DCA", pause, loop);
        } else if (strncmp (argv[i], "-col", 2) == 0) {
	    colSim ("Collector", pause, loop);
	}
    }
}


panSim (char *name, int pause, int loop)
{
    int i, fd = 0;

    if ((fd = supOpenConn (DEF_DEV, name)) <= 0)
	return;

    for (i=0; i < loop; i++) {
        supSendStatus (fd, "proc doing fine...");
        sleep (pause+1);
        supSendStatus (fd, "proc feeling tired...");
        sleep (pause+2);
        supSendStatus (fd, "proc quitting...");
        sleep (pause+3);
    }

    supCloseConn (fd);
}



colSim (char *name, int pause, int loop)
{
    int i, ctid = 0, tid;


    if ((ctid = mbusConnect (name, "client", FALSE)) <= 0)
	return;

    for (i=0; i < loop; i++) {
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc receiving data...");
        sleep (pause);
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "trans received 65535 bytes...");
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc done...");
        sleep (pause);
    }
    mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc quitting...");

    mbusDisconnect (ctid);
}


rtdSim (char *name, int pause, int loop)
{
    int i, ctid = 0, tid;


    if ((ctid = mbusConnect (name, "client", FALSE)) <= 0)
	return;

    for (i=0; i < loop; i++) {
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc receiving data...");
        sleep (pause);
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "trans received 65535 bytes...");
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc done...");
        sleep (pause);
    }
    mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc quitting...");

    mbusDisconnect (ctid);
}

dcaSim (char *name, int pause, int loop)
{
    int i, j, ctid = 0, tid;
    char buf[128];


    if ((ctid = mbusConnect (name, "client", FALSE)) <= 0)
	return;

    for (i=0; i < loop; i++) {
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc receiving data...");
        sleep (pause);
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc Configuring Geometry...");
        sleep (pause);
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc Creating image...");
        sleep (pause);
        for (j=0; j < 8; j++) {
	    sprintf (buf, "dca %4d header  %4d data packets -- %s\0", 
		j*2, j*16, myTimestamp());
            mbusSend (SUPERVISOR, ANY, MB_STATUS, buf);
            sleep (pause);
        }
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc creating image...");
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc done...");
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "dca   ");
        sleep (pause);
    }
    mbusSend (SUPERVISOR, ANY, MB_STATUS, "proc quitting...");

    mbusDisconnect (ctid);
}





/*  PROCTIMESTAMP -- Return the time in the form 12:34:56
*/
char *
myTimestamp ()
{
    time_t clock = time (0);
    char   *tstr = ctime (&clock);

    tstr = &tstr[11];
    tstr[8] = '\0';

    return (tstr);
}




/***************************************************************************
**
**  Supervisor Connection code -- Two simple interface procedures that 
**  open and negotiate a connection to the Supervisor, and then close it.
**  The Supervisor is located on a 'device' specified as 'port@hostname'
**  where the hostname us can be a simple host name, a fully-qualified
**  host name, or an IP address.
**
**	fd = supOpenConn (dev, name)	// returns NULL on failure
**	   supSendStatus (fd, msg);	// send status message
**	    supCloseConn (fd);		// close connection
**
***************************************************************************/

#define DEBUG		0

static int supOpenSocket (char *dev);
static int supParseNode (char *dev, unsigned short *host_port, 
				unsigned long *host_addr);
static int supSockRead (int fd, void *vptr, int nbytes);
static int supSockWrite (int fd, void *vptr, int nbytes);



/*  SUPOPENCONN -- Open a connection to the Supervisor.  This procedure handles
**  the connection negotian and returns a file descriptor of the connection.
*/

int
supOpenConn (char *dev, char *name)
{
    int	  omain, fd, rfd;
    pid_t pid = getpid();
    char  msg[128], host[128];


    /*  Open the public port to the supervisor.
     */
    if ((fd = supOpenSocket (dev)) < 0) {
	fprintf (stderr, "Error opening Supervisor on '%s'.\n", dev);
	return (-1);
    }

    bzero (host, 128);			/* get the local host name	*/
    if (gethostname (host, 128) < 0) 
	strcpy (host, "unknown");

					
    /* Send the connect request.
     */
    bzero (msg, 128);
    sprintf (msg, "connect %s@%s:%d\0", name, host, pid);
    if (supSockWrite (fd, msg, strlen (msg)+1) < 0) {
	fprintf (stderr, "Connect Error %d: %s\n", errno, strerror(errno));
	return (-1);			/* ERR return value	*/
    }


    bzero (msg, 128);
    supSockRead (fd, msg, 128); 	/*  get reconnect address    	*/
    close (fd); 			/*  close the original socket  	*/

    /* Reconnect request.
     */
    if ((rfd = supOpenSocket (&msg[10])) < 0) { /* open the new channel */
	fprintf (stderr, "Error reconnecting Supervisor.\n");
	return (-1);
    }

    bzero (msg, 128); 			/* tell em we're ready  */
    sprintf (msg, "ready rc_%s@%s:%d\0", name, host, pid);
    if (supSockWrite (rfd, msg, strlen (msg)+1) < 0) {
	fprintf (stderr, "Connect Error %d: %s\n", errno, strerror(errno));
	return (-1);			/* ERR return value	*/
    }

    return (rfd);
}


/*  SUPSENDSTATUS -- Send a status message to the Supervisor.  We need to
**  format the message ourselves since we'll be talking to a Supervisor
**  socket connection and can't rely on the message bus interface to deliver
**  it, however we use the same format.  The supervisor should know who we
**  are based on the socket being used, so we only need to deliver the
**  status string.
*/

#define SZ_MSGBUF		256

int
supSendStatus (int fd, char *msg)
{
    char msg_buf[SZ_MSGBUF];


    bzero (msg_buf, SZ_MSGBUF);			/* send the status msg	*/
    sprintf (msg_buf, "status %s\0", msg);

    if (supSockWrite (fd, msg_buf, strlen (msg_buf)+1) < 0) {
	fprintf (stderr, "sendStatus Error %d: %s\n", errno, strerror(errno));
	return (-1);				/* ERR return value	*/
    }

    return (0);					/* OK return value	*/
}




/*  SUPCLOSECONN -- Close a connection to the Supervisor.
*/
int
supCloseConn (int fd)
{
    char msg[128];

    sprintf (msg, "quit\0"); 			/* tell em we're leaving  */
    write (fd, msg, strlen(msg));
    close (fd);
}



/******************************************************************************
**  Private procedures
******************************************************************************/

/*  SUPOPENSOCKET -- Open the client side of a socket.
*/
static int
supOpenSocket (char *dev)
{
    struct sockaddr_in sockaddr;
    unsigned short host_port;
    unsigned long  host_addr;
    int    fd, stat;


    if ((stat = supParseNode (dev, &host_port, &host_addr)) < 0)
	return (-1);
 

    /* Get socket. */
    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	printf ("Error from socket(): %d: %s\n", errno, strerror (errno));
        goto err;
    }

    /* Compose network address. */
    bzero ((char *)&sockaddr, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = host_port;
    bcopy ((char *)&host_addr, (char *)&sockaddr.sin_addr, sizeof(host_addr));

    /* Connect to server. */
    if (connect (fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
	printf ("Error from connect(): %d: %s\n", errno, strerror (errno));
        close (fd);
        goto err;
    }

    if (DEBUG)
        fprintf (stderr, "Connection established on '%s' fd=%d\n", dev, fd);

    return (fd);

err:
    fprintf (stderr, "Cannot open server connection on '%s'.\n", dev);
    return (-1);
}



/*  SUPPARSENODE -- Parse a device string, returning the domain type as the 
**  function value and loading the path/host information as
**  needed.
*/
static int 
supParseNode (char *dev, unsigned short *host_port, unsigned long *host_addr)
{
    char     *ip, lhost[128], port_str[128], host_str[128];
    unsigned short i, port;
    struct   servent *sv;
    struct   hostent *hp;


    if (dev == NULL)
        return (-1);

    bzero (lhost, 128);
    bzero (host_str, 128);
    bzero (port_str, 128);

    /* Get host name/address.  This may be specified either has a host
     * name or as an Internet address in dot notation.  If no host
     * name is specified default to the local host.
     */
    if (gethostname (lhost, 128) < 0) {
        strcpy (host_str, "localhost");
        if ((hp = gethostbyname(host_str)))
            bcopy (hp->h_addr, (char *)host_addr, sizeof(*host_addr));
    } else {

        for (i=0, ip=dev; *ip && *ip != ':'; ip++, i++)
	    host_str[i] = *ip;

        if (*ip == '\0' || strcmp (lhost, host_str) == 0) { 
            strcpy (host_str, "localhost");
            if ((hp = gethostbyname(host_str)))
                bcopy (hp->h_addr, (char *)host_addr, sizeof(*host_addr));
        } else if (isdigit (host_str[0])) {
            *host_addr = inet_addr (host_str);
            if ((int)*host_addr == -1)
                return (-1);
        } else if ((hp = gethostbyname(host_str))) {
            bcopy (hp->h_addr, (char *)host_addr, sizeof(*host_addr));
        } else
            return (-1);

    }


    /* Get port number.  Must be specified as a decimal port number.
     */
    for (i=0, ip++; *ip && *ip != ':'; ip++, i++)
	port_str[i] = *ip;
    if (isdigit (port_str[0])) {
        port = atoi (port_str);
        *host_port = htons (port);
    } else
        return (-1);

    return (0);
}



/* SUPSOCKREAD -- Read exactly "n" bytes from the Supervisor socket descriptor. 
*/

static int
supSockRead (fd, vptr, nbytes)
int     fd; 
void    *vptr; 
int     nbytes;
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


/* SUPSOCKWRITE -- Write exactly "n" bytes to the Supervisor socket descriptor. 
*/
static int
supSockWrite (fd, vptr, nbytes)
int     fd; 
void    *vptr; 
int     nbytes;
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
