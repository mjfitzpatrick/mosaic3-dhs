/*  
**  MBBCAST.C -- Broadcast a message to a process group.
**
*/

#include <stdio.h>
#include <sys/signal.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*---------------------------------------------------------------------------
**  MBUSBCAST -- Broadcast a message to a group.
*/
int
mbusBcast (char *group, char *msg, int msgtag)
{
    int  info;


    if (!group)
	return (ERR);

    if (MB_DEBUG) 
        fprintf (stderr, "mbBcast: to='%s' msgtag=%d msg='%s'\n",
            group, msgtag, msg);

    /* Initialize the message buffer.
     */
    info = pvm_initsend (PvmDataRaw) ;

    /* Pack the message.
     */
    mbusPackMsg (FALSE, NULL, "any", msgtag, msg);


    /* Send it.
     */
    if ((info = pvm_bcast (group, msgtag)) < 0) {
        if (MB_VERBOSE) {
	    switch (info) {
	    case PvmSysErr:
	        fprintf (stderr, "BCast fails: pvmd not available\n");
	        break;
	    case PvmBadParam:
	        fprintf (stderr, "BCast fails: invalid msgtag (%d)\n", msgtag);
	        break;
	    case PvmNoGroup:
	        fprintf (stderr, "BCast fails: no such group '%s'\n", group);
	        break;
	    }
        }
	;
    }

    return (info);
}
