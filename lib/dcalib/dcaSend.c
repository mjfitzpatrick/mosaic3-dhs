#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "dcaDhs.h"

#define SR_DEBUG     (getenv("SR_DEBUG")!=NULL||access("/tmp/SR_DEBUG",F_OK)==0)

static int chan_write (int fd, void *vptr, int nbytes);


int dcaSend (int socket, char *buffer, int size) 
{
    int istat;

    if (SR_DEBUG)
	fprintf (stderr, "dcaSend sending: %d on fd=%d\n", size, socket);

    istat = chan_write (socket, buffer, size);
    if (istat < 0 || istat != size )
	istat = DCA_ERR;

    return (istat);
}


/* CHAN_WRITE -- Write exactly "n" bytes to a descriptor. 
 */

static int
chan_write (fd, vptr, nbytes)
int     fd; 
void    *vptr; 
int     nbytes;
{
    char    *ptr = vptr;
    int     nwritten = 0,  nleft = nbytes, nb = 0;


    /* Send the message. */
    while (nleft > 0) {
        if ( (nb = write(fd, ptr, nleft)) <= 0) {
            if (errno == EINTR) {
                nb = 0;
            } else {
		fprintf (stderr, "dcaSend error %d: %s\n",
		    errno, strerror (errno));
                return (-1);
	    }
        }
        nleft    -= nb;
        ptr      += nb;
        nwritten += nb;
    }

    return (nwritten);
}
