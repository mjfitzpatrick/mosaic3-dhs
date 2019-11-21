/*  
**  MBINIT.C -- Initialize the message bus.
**
**           mbus = mbusInitialize (whoAmI, hostConfig)
**
**                     mbErrorExit ()			// Private procedures
**                          mbQuit ()
*/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*---------------------------------------------------------------------------
**  MBUSINITIALIZE -- Initialize the message bus, starting the local master
**  daemon and all the remote machine daemons as well.  Only the Supervisor
**  should call this function.
*/
MBusPtr
mbusInitialize (char *whoAmI, char *hostfile)
{
    MBusPtr  mbus = (MBusPtr) NULL;
    int      argc = 0, inum, super_init = 1;
    char    *argv[4];


    if (isSupervisor (whoAmI)) {
	if (MB_VERBOSE)
	     fprintf (stderr, "Initializing Supervisor ....\n");
    } else
	super_init=0;


    if (mbAppGet (APP_INIT)) {
	if (MB_VERBOSE)
	     fprintf (stderr, "Already initialized, resetting....\n");
        mbInitApp();

	/* FIXME:  ....disconnect old app and reset properly.....*/
    }


    /* Allocate the message bus structure.
     */
    mbus = (MBusPtr) calloc (1, sizeof (MBus));


    /* Read the hosts configuration file.
     */
    if (super_init && hostfile) {
	strncpy (mbus->hostfile, hostfile, strlen (hostfile));
	argv[argc++] = strdup (hostfile);
    }


    /* Initialize the PVM options.
     */
    mbInitPVM (mbus, argc, argv);

    if (super_init) {
    	/*  Create a DHS Group and start the group server if necessary.
    	 */
    	if (MB_DEBUG) fprintf (stderr, "Setting up DHS group....\n");

    	if ((inum = pvm_joingroup ("dhs")) != 0) {
            if (inum < 0)
                pvm_perror ("Joining DHS group");
            else
                fprintf (stderr, 
		    "\nWarning: DHS already in group, instance=%d\n\n", inum);
    	}


        /* Check for whacked PVM before proceeding, e.g. in case group call 
         * hung and PVM was killed.
         */
        if (pvm_config ((int *)NULL, (int *)NULL,
	    (struct pvmhostinfo **)NULL) < 0)
                mbErrorExit();
	else
            mbus->groups_alive = TRUE; /* assume the best  */

        /* Install the event notification handlers we'll be using.
         */
        mbNotifyHandlers ();

        /* Finally, read the initial hosts configuration file.
        */
        mbReadHostfile (hostfile);
    }


    mbAppSetMBus (mbus);
    mbus->initialized++;			/* ready .....		*/
    if (MB_VERBOSE)
        fprintf (stderr, "Initialize complete\n");


    /*  Return the message bus state structure.
     */
    return (mbus);
}


/*---------------------------------------------------------------------------
** MBINITPVM -- Initialize a local PVM session.  This may be called from a
** Client which doesn't have permission to initialize the message bus but 
** shouldn't be disallowed from starting and waiting for a Supervisor to 
** connect.
*/
int
mbInitPVM (MBusPtr mbus, int argc, char *argv[])
{
    int      cc, se;


    /* Initialize the options.
    pvm_setopt (PvmResvTids, 1);
    pvm_setopt (PvmRoute, PvmDontRoute);
     */

    se = pvm_setopt (PvmAutoErr, 0);
    if ((cc = pvm_start_pvmd (argc, argv, FALSE)) < 0) {
        if (cc == PvmDupHost) {
            if (MB_DEBUG) fprintf (stderr, "Connecting to running PVMD ...\n");
            mbus->new_pvm = FALSE;
        } else {
            pvm_perror ("FATAL: Can't Start PVM");
            exit (-1);
        }
    } else {
        if (MB_DEBUG) fprintf (stderr, "New PVMD started...\n");
        mbus->new_pvm = TRUE;
    }
    pvm_setopt (PvmAutoErr, se);


    /* Initialize the signal handlers.
     */
    pvm_setopt (PvmNoReset, 1);
    signal (SIGALRM, SIG_IGN);
    signal (SIGINT,  (void *) mbQuit);
    signal (SIGQUIT, (void *) mbQuit);

    return (0);
}

/*---------------------------------------------------------------------------
** MBERROREXIT --  Exit with an error.
*/
void
mbErrorExit ()
{
    int cc;
    MBusPtr mbus = mbAppGetMBus();
    int  mytid = mbAppGet (APP_TID);


    if (mytid > 0 && mbus->groups_alive) {
        cc = pvm_getinst ("dhs", mytid);

        if (cc >= 0) {
            cc = pvm_lvgroup ("dhs");

            if (cc < 0)
                printf ("\nError leaving DHS group, cc=%d\n\n", cc);
        }
    }

    exit (-1);
}


/*---------------------------------------------------------------------------
** MBQUIT -- Quit gracefully and disconnect.
*/
void
mbQuit ()
{
    struct pvmtaskinfo *taskp;
    int ntasks, found=0, cc, i;
    MBusPtr mbus = mbAppGetMBus();
    int  mytid = mbAppGet (APP_TID);


    if (mytid > 0) {

        if (mbus->groups_alive) {

	    if (pvm_tasks(0, &ntasks, &taskp) < 0) {
		pvm_perror ("Error Cleaning up DHS Group");
		exit (-1);
	    }

            for (i=0 ; i < ntasks && !found ; i++) {
                if (strcmp("pvmg",  taskp[i].ti_a_out) == 0 || 
		    strcmp("pvmcg", taskp[i].ti_a_out) == 0 )
                        found++;
            }

            if (found) {
                if (pvm_getinst ("dhs", mytid) >= 0) {
                    if ((cc = pvm_lvgroup ("dhs")) < 0) {
                        printf( "\nError leaving DHS group, cc=%d\n\n", cc );
                    }
                }
            } else
                printf( "Warning: Group Server Not Found...\n" );
        }

        printf( "Quitting DHS - pvmd still running.\n" );
        pvm_exit();
    }

    exit (0);
}


    
/*---------------------------------------------------------------------------
** MBREADHOSTFILE -- Read the startup host configuration file.
*/
int
mbReadHostfile (char *hostfile)
{
    return (0);
}
