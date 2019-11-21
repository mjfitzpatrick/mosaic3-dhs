/*  
**  MBHALT.C -- Halt the message bus.
**
**
*/

#include <stdio.h>
#include <sys/signal.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*---------------------------------------------------------------------------
**  MBUSHALT --  Halt the current message bus.
*/
int
mbusHalt ()
{
    if ( isSuperTid (mbAppGet (APP_TID)) )
        /*  FIXME -- Gracefully tell all connected clients to shutdown.  */
        return (pvm_halt());

    else 
	return (ERR);
}
