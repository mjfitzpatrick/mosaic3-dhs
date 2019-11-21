/*  
**  MBNOTIFY.C -- Install the notification handlers on the message bus.
**
**
*/

#include <stdio.h>
#include <sys/signal.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*---------------------------------------------------------------------------
**  MBNOTIFYHANDLERS -- Install event notification handlers.
*/
int
mbNotifyHandlers ()
{
    /* Not yet implemented. */
    return (0);
}


/*---------------------------------------------------------------------------
**  MBADDHOSTADDEDHANDLER -- Add a handler for the PvmHostAdded event.
*/
int
mbAddHostAddedHandler (int tid, mbFunc handler)
{
    int info = 0, tids[10];
    int src = -1, ctx = -1;

    /* Default handler for all processes.
     */
    (void) pvm_addmhf (src, 99, ctx, handler);	/* Host Added	    */
    if ((info = pvm_notify (PvmHostAdd, 99, 0, tids)) < 0) {
	fprintf (stderr, "%s: Err %d: can't install hostAdded notifier\n",
	    mbAppGetName(), info);
    }

    return (info);
}


/*---------------------------------------------------------------------------
**  MBADDTASKEXITHANDLER -- Add a handler for the PvmTaskExit event.
*/
int
mbAddTaskExitHandler (int tid, mbFunc handler)
{
    int info = 0, tids[10];
    int src = -1, tag = 98, ctx = -1;

    (void) pvm_addmhf (src, tag, ctx, handler);	/* Task Exits	    */
    tids[0] = tid;
    if ((info = pvm_notify (PvmTaskExit, tag, 1, tids)) < 0) {
	fprintf (stderr, "%s: Err %d: can't install taskExits notifier\n",
	    mbAppGetName(), info);
    }

    return (info);
}



/*---------------------------------------------------------------------------
**  MBus Handlers -- These procedures are used to handle notification events
**  we get from the system.  For the moment we just print a status message,
**  a more formal method of updating the Supervisor state needs to be done.

int mb_hostAdded (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf ("%s: %d new hosts just added ***\n", mbAppGetName(), n);
}



int mb_taskExited (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf ("*** %s: task %d just exited ***\n", mbAppGetName(), n);
}



int mb_hostDeleted (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf ("*** %d new hosts just deleted ***\n", mbAppGetName(), n);
}

**-------------------------------------------------------------------------*/
