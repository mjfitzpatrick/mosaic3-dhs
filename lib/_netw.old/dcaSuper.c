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


#ifdef  SZMSG
#undef  SZMSG
#endif
#define SZMSG		120

in_addr_t inet_addr(const char *cp); 
#define bzero(b,len) ((void)memset((void *)(b),'\0',(len)),(void)0)

/***************************************************************************
**
**  DCA Supervisor Connection code -- Two simple interface procedures that 
**  open and negotiate a connection to the Supervisor, and then close it.
**  The Supervisor is located on a 'device' specified as 'hostname:port'
**  where the hostname us can be a simple host name, a fully-qualified
**  host name, or an IP address.
**
**	fd = dcaSupOpenConn (host, port, name)	// returns NULL on failure
**	   dcaSupSendStatus (fd, msg);		// send status message
**	    dcaSupCloseConn (fd);		// close connection
**
***************************************************************************/

#define SUP_DEBUG (getenv("SUP_DEBUG")!=NULL||access("/tmp/SUP_DEBUG",F_OK)==0)


int dcaSupOpenConn (char *host, int port, char *name);
int dcaSupSendStatus (int fd, char *msg);
int dcaSupCloseConn (int fd);
int dcaSupOpenSocket (char *host, int port);

static int dcaSupParseNode (char *host, int port, unsigned short *host_port, 
				unsigned long *host_addr);
static int dcaSupSockRead (int fd, void *vptr, int nbytes);
static int dcaSupSockWrite (int fd, void *vptr, int nbytes);

extern int dcaUseSim(void);
extern char *dcaActualHost(void), *dcaSimHost(void), *dcaDomain(void);



/*  DCASUPOPENCONN -- Open a connection to the Supervisor.  This procedure
**  handles the connection negotiation and returns a file descriptor of the
**  connection.
*/

int
dcaSupOpenConn (char *host, int port, char *name)
{
    int	  fd, rfd;
    pid_t pid = getpid();
    char  msg[SZMSG], hostname[SZMSG], domain[SZMSG], dev[SZMSG];



    fprintf (stderr, "Opening Supervisor: name='%s' mach='%s' port='%d'\n",
	name, host, port);

    /*  Open the public port to the supervisor.
     */
    if ((fd = dcaSupOpenSocket (host, port)) < 0) {
	fprintf (stderr, "Error opening Supervisor on '%s:%d'.\n", host, port);
	return (-1);
    }

#ifdef USE_LOCAL_HOSTNAME
    bzero (domain, SZMSG);		/* get the local host name	*/
    (void) getdomainname (domain, SZMSG);
    if (strncmp (domain, "(none)", 6) == 0)
	strcpy (domain, "kpno.noao.edu");

    bzero (hostname, SZMSG);		/* get the local host name	*/
    if (gethostname (hostname, SZMSG) < 0) 
	strcpy (hostname, "unknown");
#else
    if (dcaUseSim())
	strcpy (hostname, dcaSimHost());
    else
	strcpy (hostname, dcaActualHost());
    strcpy (domain, dcaDomain());

fprintf (stderr, "\n\nsupOpenConn:  host='%s'  domain='%s'\n", 
    hostname, domain);
fprintf (stderr, "\tuse_sim: %d  act='%s'  sim='%s'  dom='%s'\n\n",
	dcaUseSim(), dcaActualHost(), dcaSimHost(), dcaDomain());
#endif

					
    /* Send the connect request.
     */
    bzero (msg, SZMSG);
    if (index (hostname, (int)'.') == (char *)NULL)
        sprintf (msg, "sup_connect %s@%s:%d", name, hostname, pid);
    else
        /* Use FQDF from the hostname. */
        sprintf (msg, "sup_connect %s@%s.%s:%d", name, hostname, domain, pid);


    if (SUP_DEBUG)
        fprintf (stderr, "Sending reconnect: '%s'\n", msg);
    if (dcaSupSockWrite (fd, msg, SZMSG) < 0) {
	fprintf (stderr, "Connect Error %d: %s\n", errno, strerror(errno));
	return (-1);			/* ERR return value	*/
    }


    bzero (msg, SZMSG);
    dcaSupSockRead (fd, msg, SZMSG); 	/*  get reconnect address    	*/
    close (fd); 			/*  close the original socket  	*/
    if (SUP_DEBUG)
        fprintf (stderr, "Reply to reconnect: '%s'\n", msg);

    /* Reconnect request.
     */
    bzero (dev, SZMSG); 		/* tell em we're ready  */
    strcpy (dev, &msg[14]);
    if ((rfd = dcaSupOpenSocket (dev, -1)) < 0) { /* open new channel */
	fprintf (stderr, "Error reconnecting Supervisor.\n");
	return (-1);
    }

    bzero (msg, SZMSG); 		/* tell em we're ready  */
    sprintf (msg, "sup_ready rc_%s@%s.%s:%d", name, hostname, domain, pid);
    if (SUP_DEBUG)
        fprintf (stderr, "Sending ready: '%s'\n", msg);
    if (dcaSupSockWrite (rfd, msg, SZMSG) < 0) {
	fprintf (stderr, "Connect Error %d: %s\n", errno, strerror(errno));
	return (-1);			/* ERR return value	*/
    }

    /*  Set the Supervisor status for this client.
     */
    bzero (msg, SZMSG); 			
    sprintf (msg, "Client ready ...");
    (void) dcaSupSendStatus (rfd, msg);

    return (rfd);
}


/*  DCASUPSENDSTATUS -- Send a status message to the Supervisor.  We need to
**  format the message ourselves since we'll be talking to a Supervisor
**  socket connection and can't rely on the message bus interface to deliver
**  it, however we use the same format.  The supervisor should know who we
**  are based on the socket being used, so we only need to deliver the
**  status string.
*/

int
dcaSupSendStatus (int fd, char *msg)
{
    char msg_buf[SZMSG];


    if (fd < 0)
	return (-1);

    bzero (msg_buf, SZMSG);			/* send the status msg	*/
    sprintf (msg_buf, "sup_status %s", msg);

    if (dcaSupSockWrite (fd, msg_buf, SZMSG) < 0) {
	fprintf (stderr, "sendStatus Error %d: %s\n", errno, strerror(errno));
	return (-1);				/* ERR return value	*/
    }

    return (0);					/* OK return value	*/
}




/*  DCASUPCLOSECONN -- Close a connection to the Supervisor.
*/
int
dcaSupCloseConn (int fd)
{
    char msg[SZMSG];

    bzero (msg, SZMSG);
    sprintf (msg, "sup_quit");		/* tell em we're leaving */
    write (fd, msg, strlen(msg));
    return close (fd);					/* close the socket	 */
}



/*  DCASUPOPENSOCKET -- Open the client side of a socket.
*/
int
dcaSupOpenSocket (char *host, int port)
{
    struct sockaddr_in sockaddr;
    unsigned short host_port;
    unsigned long  host_addr;
    int    fd, stat;


    if ((stat = dcaSupParseNode (host, port, &host_port, &host_addr)) < 0)
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
	printf ("Error from connect(): %d: %s\n", errno, strerror(errno));
        close (fd);
        goto err;
    }

    if (SUP_DEBUG)
        fprintf (stderr, "Connection established on '%s:%d' fd=%d\n",
	    host, host_port, fd);

    return (fd);

err:
    fprintf (stderr, "Cannot open server connection on '%s:%d'.\n",
	    host, host_port);
    return (-1);
}



/******************************************************************************
**  Private procedures
******************************************************************************/

/*  DCASUPPARSENODE -- Parse a device string, returning the domain type as the 
**  function value and loading the path/host information as needed.
*/
static int 
dcaSupParseNode (char *host, int port, unsigned short *host_port, unsigned long *host_addr)
{
    char     *ip=(char *)NULL, lhost[SZMSG], port_str[SZMSG], host_str[SZMSG];
    unsigned short i;
    struct   hostent *hp;


    if (host == NULL)
        return (-1);

    bzero (lhost, SZMSG);
    bzero (host_str, SZMSG);
    bzero (port_str, SZMSG);

    /* Get host name/address.  This may be specified either has a host
     * name or as an Internet address in dot notation.  If no host
     * name is specified default to the local host.
     */
    if (gethostname (lhost, SZMSG) < 0) {
	/* Error getting local host name, fake it.... 
	 */
        strcpy (host_str, "localhost");
        if ((hp = gethostbyname(host_str)))
            bcopy (hp->h_addr, (char *)host_addr, sizeof(*host_addr));
    } else {

	/* Pull the host name from the host string.
	*/
        for (i=0, ip=host; *ip && *ip != ':'; ip++, i++)
	    host_str[i] = *ip;

/*      if (*ip == '\0' || strcmp (lhost, host_str) == 0) { */
        if (strcmp (lhost, host_str) == 0) { 
            strcpy (host_str, "localhost");
            if ((hp = gethostbyname(host_str))) {
                bcopy (hp->h_addr, (char *)host_addr, sizeof(*host_addr));
	    }

        } else if (isdigit (host_str[0])) {
            *host_addr = inet_addr (host_str);
            if ((int)*host_addr == -1) {
                return (-1);
	    }

        } else if ((hp = gethostbyname(host_str))) {
            bcopy (hp->h_addr, (char *)host_addr, sizeof(*host_addr));

        } else
            return (-1);
    }


    /*  Get port number.  Must be specified as a decimal port number but it
    **  can be passed in explicitly (so just use it), or as part of the host
    **  specification (so parse it).  When the Super is first contacted we'll
    **  have already broken out the node/port, but the reconnect message will
    **  be a node:port device spec so parse it here.
    */
    if (port > 0) {
	*host_port = htons (port);
  
    } else {
	unsigned short nport = 0;

        for (i=0, ip++; *ip && *ip != ':'; ip++, i++)
	    port_str[i] = *ip;

        if (isdigit (port_str[0])) {
            nport = (unsigned short) atoi(port_str);
            *host_port = htons (nport);
        } else {
            return (-1);
        }
    }

    return (0);
}



/*  DCASUPSOCKREAD -- Read exactly "n" bytes from the Supervisor socket
**  descriptor. 
*/

static int
dcaSupSockRead (fd, vptr, nbytes)
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


/*  DCASUPSOCKWRITE -- Write exactly "n" bytes to the Supervisor socket
**  descriptor. 
*/
static int
dcaSupSockWrite (fd, vptr, nbytes)
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
