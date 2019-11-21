#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/errno.h>

#include "dcaDhs.h"

/*
#define SELWIDTH	32
*/
#define SELWIDTH	FD_SETSIZE


#define SR_DEBUG     (getenv("SR_DEBUG")!=NULL||access("/tmp/SR_DEBUG",F_OK)==0)

#if 0
static int chan_read (int fd, void *vptr, int nbytes);
#endif

int dcaRecv (
    int socket,    			/* Socket file descriptor   */
    char *buffer,  			/* Receiving buffer         */
    int size       			/* Number of bytes to read  */
)
{
    int curlen = size, tot = 0, n, debug;
    int recvMsgSize; 
    fd_set allset;


    if ( (debug = SR_DEBUG) ) 
	fprintf (stderr, "dcaRecv expecting: %d on fd=%d\t", size, socket);

    do {
	FD_ZERO (&allset);
	FD_SET (socket, &allset);
	if ((n = select (SELWIDTH, &allset, NULL, NULL, NULL)) < 0) {
	    continue;

	} else if (FD_ISSET(socket, &allset)) {

            if ((recvMsgSize = read (socket, &buffer[tot], curlen)) < 0) {
		if (errno != EAGAIN) {
		    fprintf (stderr, "dcaRecv read() error: %d: %s\n",
		        errno, strerror(errno));
                    return(DCA_ERR);
		}

            } else if (recvMsgSize == 0) {
		if (debug) fprintf (stderr, ": read %d\n", EOF);
		return (EOF);

            } else {
                curlen = curlen - recvMsgSize;
                tot = tot + recvMsgSize;
            }
        }
    } while (curlen > 0) ;

    if (debug) fprintf (stderr, ": read %d bytes\n", tot);

    return tot;

/*  return (chan_read (socket, (void *)buffer, size)); */
}


/* CHAN_READ -- Read exactly "n" bytes from a descriptor. 
 */
#if 0
static int
chan_read (fd, vptr, nbytes)
int     fd; 
void    *vptr; 
int     nbytes;
{
    char    *ptr = vptr;
    int     nread = 0, nleft = nbytes, nb = 0, debug;


    if ( (debug = SR_DEBUG) ) 
	fprintf (stderr, "dcaRecv: expecting %d bytes\t", nbytes);

    while (nleft > 0) {
        if ( (nb = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR || errno == EAGAIN)
                nb = 0;             /* and call read() again */
            else
                return(-1);
        } else if (nb == 0)
            break;                  /* EOF */
        nleft -= nb;
        ptr   += nb;
        nread += nb;
    }

    if (debug)
	fprintf (stderr, ": read %d bytes\n", nread);

    return (nread);                 	/* return no. of bytes read */
}
#endif
