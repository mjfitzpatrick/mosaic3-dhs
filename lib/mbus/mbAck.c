/*  
 *  MBACK.C -- Acknowledge a message received event.
 *
 */

#include <stdio.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*--------------------------------------------------------------------------
**  MBUSACK --  Send an ACK to the message bus.
**/
int
mbusAck (int tid, int tag)
{
    int ack = OK, info;


    pvm_initsend (PvmDataDefault);
    pvm_pkint (&ack, 1, 1);

    if (MB_DEBUG) 
	fprintf (stderr, "Sending ACK to %d about %d: ", tid, tag);

    if ((info = pvm_send (tid, tag)) < 0) {
	switch (info) {
	case PvmBadParam:
	    fprintf (stderr, "ACK to %d fails, bad tid or msgtag\n", tid);
	    return (ERR);
	case PvmSysErr:
	    fprintf (stderr, "ACK to %d fails, pvmd not responding\n", tid);
	    return (ERR);
	case PvmNoBuf:
	    fprintf (stderr, "ACK to %d fails, no active buffer\n", tid);
	    return (ERR);
	}
   }

    if (MB_DEBUG) 
	fprintf (stderr, "status=%d\n", info);

   return (info);
}
