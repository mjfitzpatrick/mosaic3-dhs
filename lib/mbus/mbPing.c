/*  
**  MBPING.C -- Ping a process on the message bus.
*/

#include <stdio.h>
#include <unistd.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*---------------------------------------------------------------------------
**  MBUSPING --  Ping the specified tid to see if it is still alive.
**  A timeout given in milliseconds may be specified before declaring
**  a process dead.
*/
int
mbusPing (int tid, int timeout)
{
    int  len = 0, info, bufid;


    pvm_initsend (PvmDataDefault);
    pvm_pkint (&len, 1, 1);


    if (MB_DEBUG) 
	fprintf (stderr, "mbPing: tid=%d timout=%d\n", tid, timeout);


    if ((info = pvm_send (tid, MB_PING)) < 0) {
        switch (info) {
        case PvmBadParam:
            fprintf (stderr, "Ping to %d fails, bad tid\n", tid);
            return (ERR);
        case PvmSysErr:
            fprintf (stderr, "Ping to %d fails, pvmd not responding\n", tid);
            return (ERR);
        case PvmNoBuf:
            fprintf (stderr, "Ping to %d fails, no active buffer\n", tid);
            return (ERR);
        }
    }

    /* Wait .... */
    usleep ((unsigned int)timeout * 1000);

    /* Now get the ACK.  A negative timeout means block until we get a reply.
     */
    bufid = ((timeout < 0) ? pvm_recv (tid,MB_PING) : pvm_nrecv (tid,MB_PING));

    return ( (bufid <= 0) ? ERR : OK );
}
