/*  
**  MBRECV.C -- Receive a message.
**
*/

#include <stdio.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*---------------------------------------------------------------------------
**  MBUSRECV --  Receive a message from another application.  We are normally
**  called from a program i/o handler while waiting for messages.
*/
int
mbusRecv (int *from_tid, int *to_tid, int *subject, char **host, char **msg)
{
    int bufid, info;
    int tid = *to_tid;
    int tag = *subject;
    int nbytes=0, type=0, source=0;
    int host_len, msg_len, get_ack=0;
    char dummy;
    

    if (MB_DEBUG) printf("mbRecv: tid = %d  tag = %d\n", tid, tag);

    /*  On entry, to_tid/subject may be specified as '-1' to indicate that we
     *  will accept a message from any host for any reason.  We unpack the
     *  information from the sender as part of the message and fill it in
     *  on the way out.  The caller must remember to reset this value!
     *
     *  The host/msg pointers may be allocated here and will contain the
     *  data from the message.  The caller is responsible for freeing
     *  these pointers when it's done with them, passing in a static array
     *  will segfault.
    if ((bufid = pvm_nrecv (-1, -1)) < 0) {
     */
    if ((bufid = pvm_recv (tid, tag)) < 0) {
        switch (bufid) {
        case PvmBadParam:
            fprintf (stderr, "mbRecv: %d fails, bad tid/msgtag\n", tid);
            return (ERR);
        case PvmSysErr:
            fprintf (stderr, "mbRecv: %d fails, pvmd not responding\n", tid);
            return (ERR);
        }
    }


    if (USE_ACK) {
        info = pvm_upkint (&get_ack, 1, 1);	/* Ack required?	*/
    } else {
        get_ack = 0;
        info = pvm_upkint (&get_ack, 1, 1);	/* Ack required?	*/
    }
    info = pvm_upkint (from_tid, 1, 1);		/* sender		*/
    info = pvm_upkint (to_tid, 1, 1);		/* target recipient	*/
    info = pvm_upkint (subject, 1, 1);		/* subject		*/

    info = pvm_upkint (&host_len, 1, 1);	/* len of host name	*/
    if (host_len > 0) {				/* host name (optional) */
        *host = calloc (1, host_len);
        info = pvm_upkbyte (*host, host_len-1, 1);
        info = pvm_upkbyte (&dummy, 1, 1);
    } else
	*host = NULL;

    info = pvm_upkint (&msg_len, 1, 1);		/* len of msg body	*/
    if (msg_len > 0) {				/* msg body		*/
        *msg = calloc (1, msg_len);
        info = pvm_upkbyte (*msg, msg_len-1, 1);
        info = pvm_upkbyte (&dummy, 1, 1);
    } else
	*msg = NULL;
    

    if ((info = pvm_bufinfo (bufid, &nbytes, &type, &source)) >= 0) {
        if (MB_DEBUG) {
	    printf ("\nrecv: %d bytes from %d about %d\n", nbytes,source,type);
            printf(
	       "mbRecv(%d): from:%d to:%d subj:%d host:'%s'(%d) msg='%s'(%d)\n",
	       get_ack, *from_tid, *to_tid, *subject, *host, host_len,
	       *msg, msg_len);
        }
    }


    /* Return ACK to the sender if requested.
     */
    return (get_ack ? mbusAck (source, type) : OK);
}
