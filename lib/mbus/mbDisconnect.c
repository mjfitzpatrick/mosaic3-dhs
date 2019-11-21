/*  
**  MBDISCONNECT.C -- Disconnect the current process from the message bus and
**  clean up before leaving.  
*/

#include <stdio.h>
#include <string.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"


extern char *mbGetMBHost();


/*---------------------------------------------------------------------------
**  MBUSDISCONNECT -- Disconnect the application from the message bus.
**  If we're a client task we notify the Supervisor we're leaving, otherwise
**  cleanly unregister ourself and disconnect.
*/

int
mbusDisconnect (int tid)
{
    int info = -1, mytid = -1;
    char *appName = mbAppGetName();


    if (tid != (mytid = mbAppGet(APP_TID))) {
	fprintf (stderr, "Invalid tid=%d, app tid=%d\n", tid, mytid);
	return (ERR);
    }


    if (MB_DEBUG)
	fprintf (stderr, "Disconnecting app '%s' on tid %d  super=%d\n",
	    appName, tid, isSuperTid(tid) );

    /* Notify the Supervisor we are leaving (if we're not the super).
    */
    if (! isSuperTid(tid) ) {
        char msg[SZ_LINE];
        pid_t pid = getpid();

        memset (msg, 0, SZ_LINE);
        sprintf (msg, "{ tid=%d who=%s host=%s pid=%d }",
                        tid, appName, mbGetMBHost(), pid);

        if (MB_DEBUG)
	    fprintf (stderr, "Notifying supervisor we are leaving....");

        if ((info = mbusSend (SUPERVISOR, ANY, MB_EXITING, msg)) == ERR)
	    fprintf (stderr, "Error notifying supervisor we are leaving.\n");

        if (MB_DEBUG)
	    fprintf (stderr, "info = %d\n", info);
    }

	
    if (MB_DEBUG)
	printf ("Disconnecting app '%s' on tid %d\n", appName, tid);

    info = pvm_lookup (appName, -1, &tid);
    if (info >= 0 && pvm_delete (appName, info) != PvmOk) {
	pvm_perror ("mbDisconnect");
        fprintf (stderr, "pvm_delete() failed while disconnecting\n");
    }

    /* Notify the PVM Daemon we're leaving.
    */
    if (pvm_exit() < 0) {
	fprintf (stderr, "mbusDisconnect:  Error in pvm_exit()\n");
        return (ERR);
    }


    mbInitApp ();	/* clear state in case we reconnect again later */

    return (OK);
}


/*  MBUSEXITHANDLER -- Onexit procedure so we disconnect cleanly all the time.
*/
void 
mbusExitHandler ()
{
    /*
    MBusPtr mbus = mbAppGetMBus();

    if (mbus) {
	int mytid = mbAppGet (APP_TID);
	int  stat = mbusDisconnect (mytid);
    }
    */
}
