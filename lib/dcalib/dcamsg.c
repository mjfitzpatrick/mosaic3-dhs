#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif


#include <stdlib.h>
#include <stdarg.h>
#include "dcaDhs.h"



/* dcaFmtMsg -- Format a message for the DCA.  We are called at a minimum
 * with a message type and a sender ID.  Optional args include the address
 * and size of a data buffer associated with the message.  The header and
 * data buffers are transferred separately during the send, and space
 * allocated for the message is freed.
*/
msgHdrPtr
dcaFmtMsg (XLONG vtype, XLONG whoami, ...)

{
    va_list ap;  /* pints to each unnamed arg in turn */

    /* Struct is freed when we send the message. */
    msgHdrPtr mh = calloc (1, sizeof (msgHeader));

    mh->type   = vtype;
    mh->whoami = whoami;

    /* from global dhs state struct */

    mh->expnum = dhs.expID;	     /* from global dhs state struct */
    strncpy(mh->obset, dhs.obsSetID, 80); 

    va_start(ap, whoami);
    mh->addr   = (long)va_arg(ap, void *);
    if (mh->addr == (long) NULL) {
	va_end(ap);
	return mh;
    }
    mh->size   = va_arg(ap, int);
    va_end(ap);

    return mh;
}



/* dcaSendMsg ( dhsChan *chan, msgHdrPtr msg) */
int dcaSendMsg (int socket, msgHdrPtr msg)
{

    int size;

    if (procDebug > 100) {
	fprintf (stderr, "Header; size=%d  %d\n", sizeof(msgType), msg->size);
	fprintf (stderr, "      ; type=%d  whoami=%d\n", msg->type,msg->whoami);
	fprintf (stderr, "      ; expnum=%g  obset=%s\n",
	    msg->expnum, msg->obset);
    }
    size = sizeof(msgType);
    return(dcaSend (socket, (char *)msg, size));

}


int dcaRecvMsg (int socket, char *client_data, int size)
{
    int istat;

    istat = dcaRecv (socket, client_data, size);
    if (istat != size) {
	return (DCA_ERR);
    }
    return istat;
}


/* dcaRefreshState -- Tell the supervisor to send us all the state
 * information we could possbly want.
dcaRefreshState (int chan)
{
}
 */

int dcaSendfpConfig (int socket, XLONG whoAmI, char *buf, int size)
{
    msgHdrPtr msg;

    /* Send the message header. */
    msg=dcaFmtMsg (DCA_SET|DCA_FP_CONFIG, whoAmI);
    if (dcaSendMsg (socket, msg) == DCA_ERR)
        return DCA_ERR;

    /* Now send the data */
    return(dcaSend (socket, buf, size));
}
