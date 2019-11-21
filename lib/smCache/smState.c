/*  
**  SMSTATE -- Support mini-interface for maintaining a state structure for
**  the cache or an individual segment.  Keeps process-specific information
**  indexed by the pid.
**
**                smStateInit (smState_t *sa)
**
**                 smStateAdd (smState_t *sa, double atime, void *addr)
**              smStateRemove (smState_t *sa, pid_t pid)
**             smStateDefined (smState_t *sa)
**
**            smStateSetATime (smState_t *sa, double atime)
**    atime = smStateGetATime (smState_t *sa)
**            smStateSetLTime (smState_t *sa, double ltime)
**    ltime = smStateGetLTime (smState_t *sa)
**            smStateSetUTime (smState_t *sa, double utime)
**    utime = smStateGetUTime (smState_t *sa)
**           smStateSetReader (smState_t *sa, int reader)
**  reader = smStateGetReader (smState_t *sa)
**             smStateSetAddr (smState_t *sa, void *addr)
**      addr = smStateGetAddr (smState_t *sa)
**              smStateSetSMC (smState_t *sa, void *smc)
**        smc = smStateGetSMC (smState_t *sa)
**         smStateSetAttached (smState_t *sa, int attached)
**  addr = smStateGetAttached (smState_t *sa)
**
** index = smStateLookupByPid (smState_t *sa, pid_t pid)
**                smStateDump (smState_t *sa)
**
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

#include "smCache.h"

static int  smStateLookupByPid (smState_t *sa, pid_t pid);




/*  STATEINIT -- Initialize the state structure.
 */
void
smStateInit (smState_t *sa)
{
    register int i;

    for (i=0; i < MAX_PROCS; i++) 
	sa[i].pid = sa[i].atime = 0, sa[i].addr = (void *) NULL;
}


/*  STATEADD -- Add the element identified by pid to the array.
 */
int
smStateAdd (smState_t *sa, double atime, void *addr, int attached)
{
    int   i, slot;
    pid_t pid = smPid();


    /* Cleanup the state pid array. 
     */
    smStateClean (sa);

    /* See if we're already in the state array.  If so, return.
     */
    for (i=0; sa[i].pid && i < MAX_PROCS; i++)
	if (sa[i].pid == pid)
	    return (OK);

    /* Find first non-empty element. */
    for (i=0; sa[i].pid && i < MAX_PROCS; i++) 
	;
    slot = i;
	
    /* Save the state info. */
    if (slot < MAX_PROCS) {
        sa[slot].pid   = pid;
        sa[slot].atime = (attached ? atime : 0.0);
        sa[slot].ltime = atime; 	/* initial Ltime is the Atime	*/
        sa[slot].utime = 0.0; 		/* initial Utime 		*/
        sa[slot].addr  = addr;
        sa[slot].attached = attached;
	return (OK);

    } else {
	smStateDump (sa, "smState: MAX_PROCS exceeded");
	return (ERR);
    }
}


/*  STATEREMOVE --  Remove the element identified by pid from the array. 
 */
int
smStateRemove (smState_t *sa, pid_t rmpid)
{
    pid_t pid = (rmpid ? rmpid : smPid());
    int   i, index = smStateLookupByPid (sa, pid);

    if (index == ERR)
	return (ERR);

    /* Cleanup the state pid array. 
     */
    smStateClean (sa);

    /* Zero the current index (also clears when index is last element)
     */
    bzero (&sa[index], sizeof (smState_t));

    /* We have the index of the element to be removed, so simply shift
     * everything else in the array down.
     */
    for (i=index; i < (MAX_PROCS-1) ; i++)
	memcpy (&sa[i], &sa[i+1], sizeof (smState_t));

    return (OK);
}


/*  STATEDEFINED -- Simply find out whether we're in the state struct already.
 */
int
smStateDefined (smState_t *sa)
{
    int   i;
    pid_t pid = smPid();


    /* See if we're already in the state array.  If so, return.
     */
    for (i=0; sa[i].pid && i < MAX_PROCS; i++) 
	if (sa[i].pid == pid)
	    return (TRUE);

    return (FALSE);
}


/*  STATESETATIME -- Set the atime element of the named pid.
 */
void
smStateSetATime (smState_t *sa, double atime)
{
    int   index = smStateLookupByPid (sa, smPid());

    if (index != ERR)
	sa[index].atime = atime;
    else
	fprintf (stderr, "Warning: smState pid/atime lookup error.\n");
}


/*  STATEGETATIME -- Get the atime element of the named pid.
 */
double 
smStateGetATime (smState_t *sa)
{
    int   index = smStateLookupByPid (sa, smPid());

    return (((index != ERR) ? sa[index].atime : (double)0.0));
}


/*  STATESETLTIME -- Set the ltime element of the named pid.
 */
void
smStateSetLTime (smState_t *sa, double ltime)
{
    int   index = smStateLookupByPid (sa, smPid());

    if (index != ERR)
	sa[index].ltime = ltime;
    else
	fprintf (stderr, "Warning: smState pid/ltime lookup error.\n");
}


/*  STATEGETLTIME -- Get the ltime element of the named pid.
 */
double 
smStateGetLTime (smState_t *sa)
{
    int   index = smStateLookupByPid (sa, smPid());

    return (((index != ERR) ? sa[index].ltime : (double)0.0));
}


/*  STATESETUTIME -- Set the ltime element of the named pid.
 */
void
smStateSetUTime (smState_t *sa, double utime)
{
    int   index = smStateLookupByPid (sa, smPid());

    if (index != ERR)
	sa[index].utime = utime;
    else
	fprintf (stderr, "Warning: smState pid/utime lookup error.\n");
}


/*  STATEGETUTIME -- Get the ltime element of the named pid.
 */
double 
smStateGetUTime (smState_t *sa)
{
    int   index = smStateLookupByPid (sa, smPid());

    return (((index != ERR) ? sa[index].utime : (double)0.0));
}


/*  STATESETREADER -- Set the ltime element of the named pid.
 */
void
smStateSetReader (smState_t *sa, int reader)
{
    int   index = smStateLookupByPid (sa, smPid());

    if (index != ERR)
	sa[index].reader = reader;
    else
	fprintf (stderr, "Warning: smState pid/reader lookup error.\n");
}


/*  STATEGETREADER -- Get the ltime element of the named pid.
 */
int 
smStateGetReader (smState_t *sa)
{
    int   index = smStateLookupByPid (sa, smPid());

    return (((index != ERR) ? sa[index].reader : (int)ERR));
}


/*  STATESETADDR -- Set the addr element of the named pid.
 */
void
smStateSetAddr (smState_t *sa, void *addr)
{
    int   index = smStateLookupByPid (sa, smPid());

    if (index != ERR)
	sa[index].addr = addr;
    else
	fprintf (stderr, "Warning: smState pid/addr lookup error.\n");
}


/*  STATEGETADDR -- Get the addr element of the named pid.
 */
void *
smStateGetAddr (smState_t *sa)
{
    int   index = smStateLookupByPid (sa, smPid());
    void *addr = ((index != ERR) ? sa[index].addr : (void *)NULL);

    return ((void *)addr);
}


/*  STATESETSMC -- Set the SMC backpointer element of the named pid.
 */
void
smStateSetSMC (smState_t *sa, void *smc)
{
    int   index = smStateLookupByPid (sa, smPid());

    if (index != ERR)
	sa[index].smc = smc;
    else
	fprintf (stderr, "Warning: smState pid/smc lookup error.\n");
}


/*  STATEGETSMC -- Get the SMC backpointer element of the named pid.
 */
void *
smStateGetSMC (smState_t *sa)
{
    int   index = smStateLookupByPid (sa, smPid());
    void *smc = ((index != ERR) ? sa[index].smc : (void *)NULL);

    return ((void *)smc);
}


/*  STATESETATTACHED -- Set the attached flag of the named pid.
 */
void
smStateSetAttached (smState_t *sa, int attached)
{
    int   index = smStateLookupByPid (sa, smPid());

    if (index != ERR) {
        smState_t *s = &sa[index];
	s->attached = attached;
    } else
	fprintf (stderr, "Warning: smState pid/attached lookup error.\n");
}


/*  STATEGETATTACHED -- Get the attached flag of the named pid.
 */
int 
smStateGetAttached (smState_t *sa)
{
    int   index = smStateLookupByPid (sa, smPid());

    return (((index != ERR) ? sa[index].attached : (int)NULL));
}


/*  SMPID -- Utility routine to return the pid of the local process without
 *  having to call getpid() each time.  
 */
pid_t
smPid ()
{
    static int init = 0, pid = 0;

    if (init++)
        return pid;
    else
        return (pid = getpid());
}


/*  Clean -- Clean the state array, i.e. prune any dead pids.  
 */
void
smStateClean (smState_t *sa)
{
    register int i;
    char  procp[SZ_FNAME];


    /* See if we're already in the state array.  If so, return.
     */
    for (i=0; sa[i].pid && i < MAX_PROCS; i++) {
	if (sa[i].pid) {
#ifdef LINUX
            /*  Validate the state against the processes that claim to be 
	     *  using it.  Check that the pid is still running.
             *  
             *  NOTE: This is a linux-specific hack!
             */
            sprintf (procp, "/proc/%d", sa[i].pid);
            if (access (procp, R_OK) != 0) {
                if (SMC_DEBUG)
                    fprintf (stderr, "Cleaning pid %d from state array....\n",
                        sa[i].pid);
                bzero (&sa[i], sizeof (smState_t));
            }
#endif
	    ;
	}
    }
}


/*  DUMP -- Debug print utility.
 */
void
smStateDump (smState_t *sa, char *title)
{
    register int i;

    printf ("%s\n", title);
    printf ("     pid     atime       ltime       utime       At  Addr\n");
    for (i=0; sa[i].pid && i < MAX_PROCS; i++) 
	printf ("%2d:  %-6d  %-10g  %-10g  %-10g  %d   0x%x\n",
	    i,sa[i].pid,sa[i].atime,sa[i].ltime,sa[i].utime,
	    sa[i].attached, (int)sa[i].addr);
}



/*****************************************************************************
 *  Private procedures
 ****************************************************************************/

/*  LOOKUPBYPID -- Return the index on the pid in the state array.
 */
static int
smStateLookupByPid (smState_t *sa, pid_t pid)
{
    int i;

    for (i=0; i < MAX_PROCS; i++) 
	if (sa[i].pid == pid)
	    return (i);

    return (ERR);
}


/*****************************************************************************
 *  Unit Tests
 ****************************************************************************/

#ifdef SM_UNIT_TEST

smState_t	c_state[MAX_PROCS];

main (int argc, char **argv)
{
    int  atime;
    void *addr;

    smStateInit (&c_state);

    smStateAdd (&c_state, 123, 123000, malloc (123));

    smStateDump (c_state);

    smStateRemove (&c_state, 123); 	/* test removal	*/
    smStateDump (c_state);

    printf ("atime for 123:  %d\n",   smStateGetATime (c_state, 123));
    printf (" addr for 789:  0x%x\n", smStateGetAddr (c_state, 789));

} 
#endif


