/*
 *  SMUTIL -- Shared memory cache utility functions.
 *
 *	    key = smUtilInit  (new, cfg)
 *	 addr = smUtilAttach  (memKey, size, create)
 *		smUtilDetach  (memKey, addr, clearLock)
 *	   size = smUtilSize  (inSize, inUnit)
 *	 bool = smUtilExists  (memKey, mode)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <errno.h>

#include "smCache.h"

#define  SHM_DEBUG	1



/*  SMUTILINIT -- Initialize a shared memory segment.  We create a lock
 *  file containing the segment information and return a key used to later
 *  attach to the segment.
 */
key_t 
smUtilInitCache (
  int 	    *new,			/* new segment flag 		*/
  sysConfig_t *cfg 			/* smc config struct		*/
)
{
	int 	posit = 0;
	key_t 	memKey = (key_t) ERR;
	FILE 	*lockFile = (FILE *) NULL;
	char 	fname[SZ_FNAME];


	*new = FALSE; 			/* set new before checking status */
	bzero (fname, SZ_FNAME);


	/* Get the lock file name and try to open it.
	 */
	if (!cfg->cache_path[0])
	    sprintf (cfg->cache_path, DEF_CACHE_FILE, (int) getuid());
	strncpy (fname, cfg->cache_path, strlen (cfg->cache_path));

	if ((lockFile = fopen (fname, "a")) == (FILE *) NULL) {
	    fprintf (stderr, "shmInit:  fopen call failed for file %s. '%s'",
		fname, strerror(errno));
	    cfg->cache_path[0] = (char)NULL;
	    return memKey;
	}


	/* Check whether it is a new file.
	 */
	if ((posit = (int) ftell ((FILE *) lockFile)) == 0)
	    *new = TRUE;

	/* Write the lockfile contents, which are basically just a config
	 * file we'll read back if attaching to an existing cache.
	 */
	if (*new) {
            fprintf (lockFile, "cache_path = %s\n",   cfg->cache_path);
            fprintf (lockFile, "cache_size = %d\n",   cfg->cache_size);
            fprintf (lockFile, "nsegs = %d\n",        cfg->nsegs);
            fprintf (lockFile, "min_seg_size = %d\n", cfg->min_seg_size);
            fprintf (lockFile, "lock_cache = %d\n",   cfg->lock_cache);
            fprintf (lockFile, "lock_segs = %d\n",    cfg->lock_segs);
	}
	fprintf (lockFile, "# init pid=%ld (%d)\n",
		(long)getpid(), (int)time((time_t *)NULL));


	/* Get a token.
	 */
	if ((memKey = ftok (fname, (strlen(fname) & 0xFF))) == ERR)
	    fprintf(stderr, "shmInitCache: ftok call failed. '%s'",
		strerror(errno));

	/* Save the memKey so others can attach later.
	 */
        fprintf (lockFile, "cache_memKey = %ld\n", 
	    (long) (cfg->cache_memKey=memKey));

	fflush (lockFile);
	fclose (lockFile);

	return memKey;
}

/*  SMUTILINITSEGMENT -- Initialize a shared memory segment.
 */
key_t 
smUtilInitSegment (
  int 	*new,				/* new segment flag 		*/
  char	*lockFile 			/* segment lockfile		*/
)
{
	int 	posit = 0;
	key_t 	memKey = (key_t) ERR;
	FILE 	*lfd = (FILE *) NULL;
	char 	fname[SZ_FNAME];


	*new = FALSE; 			/* set new before checking status */
	bzero (fname, SZ_FNAME);

	/* Get the lock file name and try to open it.
	 */
	strncpy (fname, lockFile, strlen (lockFile));

	if ((lfd = fopen (fname, "a")) == (FILE *) NULL) {
	    fprintf (stderr, "shmInit:  fopen call failed for file %s. '%s'",
		fname, strerror(errno));
	    lockFile[0] = (char)NULL;
	    return memKey;
	}

	/* Check whether it is a new file.
	 */
	if ((posit = (int) ftell ((FILE *) lfd)) == 0)
	    *new = TRUE;

	/* Write the lockfile contents, which are basically just a config
	 * file we'll read back if attaching to an existing segment.
	 */
	fprintf (lfd, "# init pid=%ld (%d)\n",
		(long)getpid(), (int)time((time_t *)NULL));

	fflush (lfd);
	fclose (lfd);

	/* Get a token.
	 */
	if ((memKey = ftok (fname, (strlen(fname) & 0xFF))) == ERR)
	    fprintf(stderr, "shmInitSeg: ftok call failed. '%s'", 
		strerror(errno));

	return memKey;
}


/*  SMUTILATTACH --  Attach the current process to the shared memory
 *  segment identified by the key, or create it of the specified size
 *  if necessary.  We return the address of the buffer.
 */
void *
smUtilAttach (
  key_t  memKey,			/* shared memory key		*/
  char	*lockfile,			/* segment lock file		*/
  XLONG  size,				/* size of cache/segment        */
  int 	*create 			/* create segment flag?         */
)
{
	int shmId 	= 0;
	int shmFlag 	= 0x00001B6;	/* mode = 666			*/
	void *retAddr 	= (void *) NULL;
	FILE *lockFile 	= (FILE *) NULL;


	/* Check input parameters.
	 */
	if (memKey == (key_t) NULL || create == (int *) NULL || size == 0L) {
	    fprintf (stderr, "smUtilAttach: bad parameter, %s.\n",
		((memKey == (key_t) NULL) ? "memKey==NULL" :
		    ((size == 0L) ? "size==0" : "create==NULL")));
	    return retAddr;
	}

	/* Get the shared memory segment.
	 */
	if (*create)
	    shmFlag |= IPC_CREAT;
	if ((shmId = shmget((key_t) memKey, size, shmFlag)) == ERR) {
	    fprintf (stderr, "smUtilAttach: shmget failed, key=0x%x. '%s'.\n", 
		memKey, strerror(errno));
	    smTrap ();
	    *create = ERR;
	    return retAddr;
	}


	/*  Attach to shared memory
	 */
	if (SHM_DEBUG) {
	    fprintf (stderr, 
		"smUtilAttach:  id=0x%ld key=0x%x size=%ld\n",
		    (long) shmId, (int) memKey, (long) size);
	}
	if ((retAddr = (void *) shmat(shmId, 0, 0)) == (void *)ERR) {
	    fprintf (stderr, "smUtilAttach: shmat failed. '%s'.\n", 
		strerror(errno));
	    *create = ERR;
	    return (void *) NULL;
	}


	/* Update the lockfile.
	 */
	if ((lockFile = fopen (lockfile, "a")) == (FILE *) NULL) {
	    fprintf (stderr, "shmAttach:  lockfile fopen failed. '%s'.\n", 
		strerror(errno));
	    *create = ERR;
	    return (void *) NULL;

	} else {
            fprintf (lockFile, "cache_addr = %ld\n", (long) retAddr);
            fprintf (lockFile, "E_O_C = true\n");	     /* End of Config */
	    fprintf (lockFile, "# attach pid=%ld (%d)\t",
		(long) getpid (), (int) time ((time_t *)NULL));
	    fprintf (lockFile, "shmid=%ld  size=%ld\n",
		(long) shmId, (long) size);
	    fflush (lockFile);
	    fclose (lockFile);
	}


	/* Clear the first long word of the memory if we're creating the
 	 * segment.
	 */
	if (*create)
	    memset((void *) retAddr, 0, size);

	/* Save the sm identifier in the 'create' flag and return the
	 * pointer to the space.
	 */
	*create = shmId;
	return ((void *) retAddr);
}


/*  SMUTILDETACH --  Detach the current process from the segment
 *  specified by the memKey.  Clear the lock file and completely free
 *  the segment if we're the last process using it.
 */

void 
smUtilDetach (memKey, lockfile, address, clearLock)
key_t 	memKey;				/* shared memory segment key	*/
char	*lockfile;			/* segment lock file		*/
void 	*address;			/* the address to detach from	*/
int	clearLock;			/* clear lockfile if last proc? */
{
	char   lfile[128];
	struct shmid_ds _detBuf;
	long retVal 	= OK;
	int shmFlag 	= 0x00001B6;	/* mode = 666			*/
	int shmId 	= 0, 
	    lockFile 	= -1,
	    size 	= 123;


	/*  Clear the buffer. 
	 */
	(void) memset((void *) &_detBuf, 0, sizeof(struct shmid_ds));
	sprintf (lfile, "%s", lockfile);


	/*  Get the identifier for the segment belonging to the key.
	 */
	if ((shmId = shmget ((key_t) memKey, size, shmFlag)) == ERR) {
	    fprintf (stderr, "shmDetach: shmget call failed, key=0x%x. '%s'.\n",
		memKey, strerror(errno));
	    return;
	}

	if ((retVal = (long) shmctl (shmId, IPC_STAT, &_detBuf)) != OK) {
	    fprintf (stderr, "shmDetach: shmctl call failed. '%s'.\n", 
		strerror(errno));
	    return;
	}

	/*  Detach from the shared memory segment.
	 */
	if ((retVal = (long) shmdt (address)) != OK)
	    fprintf (stderr, "shmDetach: shmdt call at 0x%x failed. '%s'.\n", 
		(int) address, strerror(errno));

	/*  If we're the only process left using it and asked to clean up,
	 *  mark it for deletion.
	 */
	if (clearLock && _detBuf.shm_nattch == 1) {
	    if ((retVal = (long) shmctl (shmId, IPC_RMID, &_detBuf)) != OK) {
	        fprintf (stderr, "shmDetach: shmctl call 2 failed, '%s'.\n",
		    strerror(errno));
	    }

	    if (clearLock) {
	        if ((lockFile = unlink (lfile)) == ERR)
	            fprintf (stderr, "shmDetach: unlink failed. '%s'\n", 
			strerror(errno));
	    }
	}
}


/*  SMUTILSHMSTAT -- Debug print utility procedure.
 */
void
smUtilShmState (key_t  memKey)
{
    struct shmid_ds sp;
    long   retVal = OK;
    int    shmFlag = 0x00001B6, shmId = 0;


    if ((shmId = shmget ((key_t) memKey, 0, shmFlag)) == ERR) {
        fprintf (stderr, "shmState: shmget call failed. '%s'.\n", 
    	    strerror(errno));
        return;
    }

    if ((retVal = (long) shmctl (shmId, IPC_STAT, &sp)) != OK) {
        fprintf (stderr, "shmState: shmctl call failed. '%s'.\n", 
    	    strerror(errno));
        return;
    }

    printf ("\n");
    printf (" shm_segsz = %d\n", (int) sp.shm_segsz);
    printf (" shm_atime = %d\n", (int) sp.shm_atime);
    printf (" shm_dtime = %d\n", (int) sp.shm_dtime);
    printf (" shm_ctime = %d\n", (int) sp.shm_ctime);
    printf ("  shm_cpid = %d\n", (int) sp.shm_cpid);
    printf ("  shm_lpid = %d\n", (int) sp.shm_lpid);
    printf ("shm_nattch = %d\n", (int) sp.shm_nattch);
}



/*  SMUTILSIZE -- Re-size a segment into an even multiple of some unit.
 */

XLONG 
smUtilSize (
  XLONG  *inSize,			/* requested size 		*/
  XLONG   inUnit 			/* size of base unit 		*/
)
{
	/* Check input parameters.
	 */
	if (inSize == (XLONG *) NULL || inUnit == 0L) {
	    fprintf (stderr, "shmSize:  bad parameter, %s\n",
		((inUnit == 0L) ? "inUnit==0L" : "inSize==NULL"));
	    return 0L;
	}

	/* Re-size if it doesn't divide evenly. 
	 */
	if (inUnit % (*inSize))
	    *inSize = ((*inSize / inUnit + 1L) * inUnit);

	return *inSize;
}


/* SMUTILKEY2ID -- Map a memKey to an shmId.
 */
int
smUtilKey2ID (key_t memKey)
{
    return (shmget((key_t) memKey, 0, 0));
}


/*  SMUTILEXISTS -- Determine whether the key points to an existing segment.
 *  accessible with the mode specified.
 */
int 
smUtilExists (
  key_t  memKey,			/* shared memory segment key	*/
  int	mode 				/* access mode			*/
)
{
	key_t  smID = (key_t) NULL;
       
        return ((smID = shmget (memKey, 0, mode)) != -1);
}


/*  SMUTILTIME -- Get the time expressed as a double to include the 
 *  microseconds field of the epoch.
 */
double smUtilTime ()
{
        struct timeval tv;
        struct timezone tz;

        gettimeofday (&tv, &tz);
	return ((double)(tv.tv_sec + ((double)tv.tv_usec / 1000000.)));
}


/*  SMUTILTIMESTR -- Convert the time to a standard date string for humans.
 */
char *smUtilTimeStr (double t)
{
	time_t  tt = (int)t;
	return (ctime (&tt));
}

void smTrap () { int i; i = 1; }
