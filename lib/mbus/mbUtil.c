/*  
**  MBUS Utility Procedures.   
*/

#include <stdio.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"




/*---------------------------------------------------------------------------
** MBUSSUPERTID -- Return the TID of the Supervisor.
*/
int
mbusSuperTid ()
{
     return (mbAppGet (APP_STID));
}


/*---------------------------------------------------------------------------
** ISSUPERTID -- Check whether given is the Supervisor.
*/
int
isSuperTid (int tid)
{
     return ( (tid == mbAppGet(APP_STID)) );
}
