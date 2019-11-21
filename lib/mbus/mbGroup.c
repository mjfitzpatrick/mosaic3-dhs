/*  
**  MBGROUP.C -- Handle the process groups on the message bus.
**
**              stat = mbusJoinGroup (group)
**             stat = mbusLeaveGroup (group)
**
*/

#include <stdio.h>
#include <sys/signal.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"


/*---------------------------------------------------------------------------
**  MBUSJOINGROUP -- Join the named group.
*/
int
mbusJoinGroup (char *group)
{
    return ( pvm_joingroup (group) );
}


/*---------------------------------------------------------------------------
**  MBUSLEAVEGROUP -- Leave the named group.
*/
int
mbusLeaveGroup (char *group)
{
    return ( pvm_lvgroup (group) );
}
