/*
 *  SMCACHE -- Shared Memory Cache Interface.
 *
 *
 *	       smc = smcOpen  (config)
 *	       smcInitialize  (smc)
 *	      smc = smcClose  (smc, free_flag)
 *        page = smcFindPage  (smc, memKey)
 *        page = smcNextPage  (smc, timeout)
 *	            smcPrune  (smc)
 *	            smcReset  (smc)
 *
 *     page = smcNextByExpID  (smc, expID)
 *              smcLockExpID  (smc, expID)
 *            smcUnlockExpID  (smc, expID)
 *
 *	   page = smcGetPage  (smc, type, size, attach_flag, lock_flag)
 *	           smcAttach  (smc, page)
 *	           smcDetach  (smc, page, free_flag)
 *
 *	  	   smcSetDir  (smc, dirpath)	// Filename Creation attrs
 *	     dir = smcGetDir  (smc)
 *	         smcSetFRoot  (smc, froot)
 *       froot = smcGetFRoot  (smc)
 *	         smcSetSeqNo  (smc, seqnum)
 *      seqnum = smcGetSeqNo  (smc)
 *	        smcIncrSeqNo  (smc)
 *	        smcDecrSeqNo  (smc)
 *	       smcResetSeqNo  (smc)
 *
 *	          smcMutexOn  ()		// Mutual exclusion locks
 *	         smcMutexOff  ()
 *
 *	 dir = smcGetPageDir  (page)
 *   froot = smcGetPageFRoot  (page)
 *  seqnum = smcGetPageSeqNo  (page)
 *
 *     data = smcGetPageData  (page)		// Managed Page Interface
 *	         smcFreePage  (page)
 *	             smcLock  (page)
 *	           smcUnlock  (page)
 *
 *           who = smcGetWho  (page)		// Page Attributes
 *                 smcSetWho  (page, who)
 *       expID = smcGetExpID  (page)
 *               smcSetExpID  (page, expID)
 *       colID = smcGetColID  (page)
 *               smcSetColID  (page, colID)
 *   obsetID = smcGetObsetID  (page)
 *             smcSetObsetID  (page, obsetID)
 *       colID = smcGetColID  (page)
 *               smcSetColID  (page, colID)
 *    num = smcGetExpPageNum  (page)
 *          smcSetExpPageNum  (page, expPageNum)
 *       md = smcGetMDConfig  (page)
 *            smcSetMDConfig  (page, mdConfig)
 *       fp = smcGetFPConfig  (page)
 *            smcSetFPConfig  (page, fpConfig)
 *
 *
 * Callbacks:
 *
 *       id = smcAddCallback  (smc, callback_type, fcn, client_data)
 *         smcRemoveCallback  (smc, id)
 *
 *             read_callback  (client_data)
 *            write_callback  (client_data)
 *
 *	   
 *  Example:
 *
 *  main (int argc, char **argv) {
 *	// Open a cache managing 64 pages.
 *	smc = smcOpen ("npages=64");
 *
 *	// Get a page with a 10K user data allocation, attach, don't lock
 *	page = smcGetPage (smc, TY_DATA, 10240, TRUE, FALSE)
 *
 *	memset (page->data, 1, 10240);    // fill the data area with a '1'
 *
 *	// Close the cache, detach but don't free.  Another client should
 *	// be able to connect to it to see the data. 
 *  	(void) smcClose (smc, FALSE);
 *    }
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>

#include "smCache.h"

#define SMC_SEM_ID	250

static int    smc_sem_id;
static int    smc_sem_initialized = 0;


/*  Local definitions.
 */
char *smcType2Str ();

smCache_t 	empty_cache_p;



/*  SMCOPEN --  Open the Shared Memory Cache for the current process.  If
 *  the cache already exists we simply attach to it and return the structre
 *  pointer for the current process, otherwise we create it.  If the 'config'
 *  parameter is NULL we use hardwired defaults for the cache, otherwise it
 *  may point to a filename containing the configuration of the cache on this
 *  machine, or it can be a whitespace/comma-delimited string of kw=value pairs
 *  specifying some or all of the configuration options.  Options other than
 *  the cache_file are ignored when we attach to an existing cache, the
 *  current values are used instead.
 *
 *  Configuration options currently include:
 *
 *	cache_file	- cache file to open
 *	lock_file	- lock file to open
 *	nsegs		- no of segments to be managed
 *	min_seg_size	- default page size (Mb)
 *	lock_cache	- specifies whether cache is locked in memory
 *	lock_segs	- specifies whether segments are locked in memory
 */
smCache_t *
smcOpen  (char *config)
{
    void  	*addr = NULL;
    int    	 i, new = 0, reader = TRUE;
    key_t  	 memKey = (key_t) NULL;
    smCache_t 	*smc = (smCache_t *) NULL;
    smcPage_t 	*page = (smcPage_t *) NULL;
    sysConfig_t *cfg = (sysConfig_t *) NULL;
    	

    /*  Allocate and initialize the sysConfig structure.  Then process the 
     *  configuration options.  These will be overwritten if we are attaching 
     *  to an existing cache file.
     */
    cfg = (sysConfig_t *) calloc (1, sizeof(sysConfig_t));
    smParseConfig (config, cfg, &reader);

    /* Open and/or initialize the SMC area.
     */
    memKey = smUtilInitCache (&new, cfg);
    if (!memKey) {
	fprintf (stderr, "Error initializing cache: '%s'\n", cfg->cache_path);
	return ((smCache_t *)NULL);
    }

    if (!new) {
	XLONG  size = cfg->cache_size;
	key_t  memKey = cfg->cache_memKey;

        /*  If this is not a new cache we're attaching to an existing one.
	 *  The config file gave us the memKey for the segment, so simply
	 *  attach to it here and set the SMC struct pointer to the running
	 *  cache.
         */

	if (cfg->debug)
	    fprintf (stderr,"Attaching to cache at addr=0x%x key=0x%x sz=%d\n",
		(int)cfg->cache_addr, (int)cfg->cache_memKey, size);

        addr = (void *)smUtilAttach (memKey, cfg->cache_path, 
	    cfg->cache_size, &new);
        if (!addr) {
            fprintf (stderr, "Error attaching to cache: file='%s' key=%ld\n", 
	        cfg->cache_path, (long) memKey);
	    return ((smCache_t *)NULL);
        }
        smc = (smCache_t *)addr;
        smc->fd = smc->shmid = new;	/* fd is returned from the attach */

	/*  Add us to the state struct so we know who's using the cache.
	 */
	smStateAdd (smc->pstate, smUtilTime (), (void *)addr, TRUE);

	if (smc->debug)
	    fprintf (stderr, "attached  cache_addr=0x%x  addr=smc=0x%x\n",
		(int)cfg->cache_addr, (int)smc);

    } else {
	/*  If this is a new cache we'll need to open the SMC structure.
	 *  Once we initialize we copy this struct to the addr of the shmem
	 *  who then becomes the runtime struct seen by all processes.
	 */
	memset (&empty_cache_p, 0, sizeof (smCache_t));
        smc = &empty_cache_p;
	bcopy (cfg, &smc->sysConfig, sizeof (sysConfig_t));
        smc->new = new;
        smc->memKey = memKey;
        smc->debug = SMC_DEBUG;


        /*  Compute the size of the SMC public object so that it is large enough
         *  for the number of requested cache
         */
        smc->cache_size = cfg->cache_size;

        smc->np_hiwater = 0;
        smc->np_max = cfg->max_segs;

        smc->mem_allocated = smc->cache_size;
        smc->mem_avail = (cfg->sys_avphys_pages * cfg->sys_page_size);

	smc->top = 0;
	smc->npages = 0;
	smc->ctime = smUtilTime ();

        smc->seqno = 0;
        strcpy (smc->dirpath, DEF_DIRPATH);
        strcpy (smc->froot, DEF_FROOT);

        /*  Attach to the segment containing the "head" of the cache.  We attach
         *  to the various pages during the initialization phase.
         */
        smc->cache_addr = (void *) smUtilAttach (smc->memKey, 
	    cfg->cache_path, smc->cache_size, &new);
        if (!smc->cache_addr) {
            fprintf (stderr, "Error attaching to cache: file='%s' key=%ld\n", 
	        cfg->cache_path, (long) smc->memKey);
	    (void) smcClose (smc, TRUE);
	    return ((smCache_t *)NULL);
        }
        smc->fd = smc->shmid = new;	/* fd is returned from the attach */
	cfg->cache_addr = (void *)smc->cache_addr;


	/*  Add us to the state struct so we know who's using the cache.
	 */
	smStateAdd (smc->pstate, smUtilTime (), (void *)smc->cache_addr, TRUE);

	/*  Clear and copy the SMC struct to the shared memory area. 
	 */
	bcopy (cfg, &smc->sysConfig, sizeof(sysConfig_t));
	memset ((addr=(void *)smc->cache_addr), 0, smc->cache_size);
	bcopy (smc, addr, smc->cache_size);

	/* Initialize the page segment array.
	*/
	for (i=0; i < MAX_SEGS; i++) {
	    page = &smc->pdata[i];
	    memset (page, 0, sizeof(smcPage_t));
	    page->free = TRUE;
	}

	/*  Lastly, free the local SMC pointer and then set it to the shmem
	 *  area we've just "allocated".
	 */
        smc = (smCache_t *)addr;
    } 
    cfg->smc = smc;	 		/* initialize structure pointer	  */


    /*  We are called whenever a process opens the cache.  Each process can
     *  register an interest in a particular segment type in the config 
     *  string, or by default is interested in everything in the cache.  Each
     *  page will be initialized with the number of processes reading that 
     *  type of segment, this refcount is decremented each time a process 
     *  detaches and when it reaches zero we assume the page is 'free' to be
     *  recycled.  The smcGetPage() method is used to get a new/recycled page
     *  for writing, all other processes are assumed to be read-only and will
     *  simply attach/detach from the segment.  The 'nattached' attribute on
     *  the cache is the max number of readers we'll need to be concerned
     *  with.
     */

    smc->nattached++;
    smStateSetReader (smc->pstate, reader);

    /*  Lock the SMC in memory so it won't be paged.  Do this only once.
     *  Check the effective userid of the process since only root can lock
     *  pages.
     */
    if (smc->nattached == 1 && !smc->vm_locked && geteuid() == 0) {
	if (mlock (smc->cache_addr, smc->cache_size) < 0) {
	    fprintf (stderr, "Warning: Locking failed during startup (%s).\n",
		strerror(errno));
	} else
	    smc->vm_locked = TRUE;
    }

    /* Initialize the mutex semaphore.
    */
    smcMutexInit ( ( SMC_SEM_ID + getuid() ) );


    /*  Debug output.
     */
    if ((smc->debug = cfg->debug)) {
        smcPrintCacheInfo (smc, (char *)NULL);
        smcPrintCfgInfo (&smc->sysConfig, (char *)NULL);
    }

    return (smc);
}


/*  SMCINITIALIZE -- Initialize the Shared Memory Cache.   This is normally
 *  only called as a cleanup method to free pages that may be left around
 *  because of a process crash.
 */
int
smcInitialize  (smCache_t *smc)
{
    int    i = 0, create = 0;
    smcPage_t *page = (smcPage_t *)NULL;

 
    if (!smc) {
	fprintf (stderr, "smcInit: Null cache pointer, use smcOpen() first.\n");
	return (ERR);
    }
	
    /* Cleanup the state pid array. 
     */
    smStateClean (smc->pstate);


    /* Detach from each of the pages.
     */
    for (i=0; i < smc->np_max; i++) {
        page = &smc->pdata[i];
        create = 0;

	if (page->memKey == (key_t)NULL)
	    continue;

	if (smc->debug)
	    fprintf (stderr, "Initializing page %d: key:0x%x seg:0x%x...\n",
		    i, (int)page->memKey, (int)SEG_ADDR(page));

	/*  Detach from the segment.
	 */
        smcDetach (smc, page, TRUE);

	/*  Reinitialize the state struct.
	 */
	smStateRemove (page->pstate, (pid_t)NULL);

	/*  Mark the page as free.
         */ 
	smcFreePage (page);

	if (smc->debug) 
	    fprintf (stderr, "done.\n");
    }
    smcPrune (smc); 				/* compact the cache */

    smc->seqno = 0;				/* init filename attributes */
    strcpy (smc->dirpath, DEF_DIRPATH);
    strcpy (smc->froot, DEF_FROOT);

    return (OK);
}


/*  SMCCLOSE -- Close this instance of the smCache by detaching from the
 *  cache and optionally freeing the space.  The 'free_flag' is used to 
 *  release the shared memory of the cache itself or the associated segments
 *  provided there are no other processes attached to them.  The last process
 *  to exit turns off all the lights.
 */
smCache_t *
smcClose  (smCache_t *smc, int free_flag)
{
    sysConfig_t *cfg  = &smc->sysConfig;
    smcPage_t   *page = (smcPage_t *) NULL;
    register int i;


    if (!smc)
	return ((smCache_t *)NULL);

    if (smc->debug) {
	fprintf (stderr, "smcClose: free=%d nattach=%d  np=%d  top=%d\n", 
	    free_flag, smc->nattached, smc->npages, smc->top);
    }

    /*  Detach from the segments in the cache.
    */
    for (i=0; i < smc->top; i++) {
        page = &smc->pdata[i];
	
        if (smc->debug && smc->sysConfig.verbose)
	    smcPrintPageInfo (page, i);

        if (free_flag && page->nattached < 1 && page->nreaders < 1) {
	    if (smc->debug)
		fprintf (stderr, "Cleaning page %d: key:0x%x id=%d size:%d...",
		    i, (int)page->memKey, (int)page->seg_addr, page->size);

	    if (page->memKey) {
	        /* Attach to segment so we can properly detach and clean up. 
		 */
	        smcAttach (smc, page);
                smcDetach (smc, page, free_flag);

	    /*  The following code doesn't appear to be used any more.  The
	    **  seg_addr is used in place of the shmId to clean up the buffer
	    **  but this is not set anywhere in the code.  For now, just comment
	    **  out the block.
	    **
	    **  MJF  9/11/07
	    
	    } else if (page->memKey == (key_t)NULL && page->seg_addr) {
        	long retVal     = OK;
		int  shmId 	= (int)page->seg_addr;
        	struct shmid_ds _detBuf;

	        if (smc->debug)
		    fprintf (stderr, "Cleaning shmId=%d size:%d...",
		        i, page->seg_addr, page->size);

                if ((retVal = (long)shmctl (shmId, IPC_RMID, &_detBuf)) != OK) {
                    fprintf (stderr, "shmDetach: shmctl call 2 failed, '%s'.\n",
                        strerror(errno));
                }
	    */

	    }	

	    if (smc->debug) fprintf (stderr, "done.\n");

	} else if (free_flag || page->free) {
	    if (page->memKey) {
	        if (smc->debug)
		    fprintf (stderr, "Detaching page %d: key:0x%x seg:0x%x...",
		        i, (int)page->memKey, (int)SEG_ADDR(page));

	        /* Attach to segment so we can properly detach and clean up. */
	        if (!SEG_ATTACHED(page))
		    smcAttach (smc, page);
                smcDetach (smc, page, free_flag);
	    }
	    if (smc->debug) fprintf (stderr, "done.\n");
        }
    }

    /*  Release the memory lock on the SMC if we're the last process using
     *  it.  The lock will be re-established once another process connects.
     */
    smc->nattached--;
    if (smc->nattached == 0 && smc->vm_locked && geteuid() == 0) {
	if (munlock (smc->cache_addr, smc->cache_size) < 0)
	    fprintf (stderr, "Warning: Unlocking failed during shutdown.\n");
	else
	    smc->vm_locked = FALSE;
    }


    /*  Detach from the cache itself.  Removes the lock file if we're 
     *  the only process left and freeing the cache.
     */
    smUtilDetach (smc->memKey, cfg->cache_path, SEG_ADDR(smc), free_flag);
    if (free_flag) 
        smc = (smCache_t *) NULL;

    return ((smCache_t *) smc);
}


/*  SMCGETPAGE -- Get an empty page slot from the cache.
 */
smcPage_t *
smcGetPage (smCache_t *smc, int type, long size, int attach_flag, int lock_flag)
{
    int     i, pnum=0, new=0, attach=0, matched=0;
    int     pgsize, npages, nsize;
    smcPage_t *p = (smcPage_t *) NULL;
    smcSegment_t *sp = (smcSegment_t *) NULL;
    smcSegment_t *seg = (smcSegment_t *) NULL;


    if (!smc) {
	fprintf (stderr, "getPage: Null cache pointer.\n");
	return ((smcPage_t *)NULL);
    }
    if (size > MAX_SEG_SIZE) {
	fprintf (stderr, "getPage: Max segment size of %d bytes exceeded.\n",
	    MAX_SEG_SIZE);
	return ((smcPage_t *)NULL);
    }


    /*  See whether we need to throttle the process because we have exceeded
     *  (or are about to) the max number of pages we can handle.   Suspend
     *  the task for a fixed period of time and try again, punt if we have to
     *  do this too many times and let the caller handle it.
     */
    for (i=0; smc->npages >= smc->np_max; i++) {
        int throttle_time = smc->sysConfig.throttle_time;
        int throttle_ntry = smc->sysConfig.throttle_ntry;
        int verbose = smc->sysConfig.verbose;

	if (verbose) {
	    fprintf (stderr, 
		"Warning: max pages (%d) exceeded, throttling try %d...\n",
	    	smc->npages, i);
	}
	usleep ((unsigned long)(throttle_time * 1000));
	if (i == throttle_ntry) {
	    if (verbose) {
	        fprintf (stderr, "Warning: Throttle max-tries (%d) exceeded.\n",
	            throttle_ntry);
	    }
	    return ((smcPage_t *)NULL);
	}
    }


    if (smc->debug)
	fprintf (stderr, "getPage: npages=%d top=%d size=%d type=%d\n", 
	    smc->npages, smc->top, (int)size, type);


    /*  Look for an existing unused page that meets the requirements.
     *  An "unused" page is one already allocated but with nobody currently
     *  attached and who's 'free' flag is set (i.e. no pending attachments).
    pnum = smc->top;
    for (i=0; i < smc->npages; i++) {
     */
    pnum = smc->top;			/* default to top of stack	*/
    for (i=0; i < smc->top; i++) {
        p = &smc->pdata[i];

        if (smc->debug) {
	    fprintf (stderr, 
	      "%3d: key=0x%07x saddr=0x%08x free=%d typ/rq=%d/%d sz/rq=%d/%d ",
		i, p->memKey, (int) SEG_ADDR(p), p->free, p->type, type, 
		p->size, (int) size);
	}

	/*  Check for an available page slot.
	 */
	if (!matched) {
	    if (p->memKey == (key_t)NULL || (p->nattached == 0 && p->free)) {
	        pnum = i;
                if (smc->debug) fprintf (stderr, "    ***\n");
	        matched++;

	    }
	}
        if (smc->debug) fprintf (stderr, "\n");
    }


    /*  FIXME -- Page Recycling doesn't work quite yet .... force 'new'
    */

    /* Open and/or initialize the segment. */
    if ((new = TRUE)) {
	/*  Get space for the page struct. 
	 */
        p = &smc->pdata[pnum];
	memset (p, 0, sizeof (smcPage_t));

	sprintf (p->lockFile,"%s_%d",smc->sysConfig.cache_path, pnum);
	/*
	p->size   = (sizeof (smcSegment_t) + 1) + (size + 1);
	*/
	p->size	  = (sizeof (smcSegment_t) + 1) + (size + 1) + 1;
	p->type	  = type;
	p->ctime  = smUtilTime ();
	p->cpid   = smPid ();

	/*  Update the memory usage counters. */
	smc->mem_allocated += p->size;
	smc->mem_avail     -= p->size;

	if (smc->debug)
	    fprintf (stderr, "getPage: p=0x%x top=%d pnum=%d\n",
		(int) p, smc->top, pnum);

	/*  Allocate the segment, we'll copy over the structure once we've
	 *  attached to the segment.
	seg = (smcSegment_t *) calloc (1, (sizeof(smcSegment_t)+(size+1)) );
	 */
	seg = (smcSegment_t *) calloc (1, (sizeof(smcSegment_t)+1) );
	if (seg == (smcSegment_t *)NULL) {
            fprintf (stderr, "Error allocating segment: '%s'\n", p->lockFile);
            return ((smcPage_t *)NULL);
	}

        p->memKey = smUtilInitSegment (&new, p->lockFile);
        if (!p->memKey) {
            fprintf (stderr, "Error initializing segment: '%s'\n", p->lockFile);
            return ((smcPage_t *)NULL);
        }

        smc->npages++;
	if (pnum == smc->top)
            smc->top++;

    } else {
	/* Page exists, but let's initialize it.
	p->size	 	= (sizeof (smcSegment_t) + 1) + (size + 1);
	 */
	p->size	 	= (sizeof (smcSegment_t) + 1) + (size + 1) + 1;
	p->type	 	= type;
	p->ctime 	= smUtilTime ();
	p->nattached	= 0;
	p->refcount	= 0;
	p->initialized	= 0;
	memset (p->lockFile, 0, SZ_PATH);
	sprintf (p->lockFile,"%s_%d",smc->sysConfig.cache_path, pnum);

	if (smc->debug) {
	    fprintf (stderr, "getPage recycling: page %d  top=%d  key=0x%x\n",
		pnum, smc->top, (int)p->memKey);
	}
    }

    /*  Create an aligned page size larger than what we actually need.
     */
    pgsize  = sysconf (_SC_PAGESIZE);
    npages  = (p->size / pgsize);
    p->size = (npages + 1) * pgsize;

    /*  Attach to the segment?  If so we'll attach to create the shmem 
     *  segment, then fill out the struct and copy it over.  The attach()
     *  routine here returns the pointer to the shmem.
     */
    if (attach_flag) {
	sp = (smcSegment_t *) smUtilAttach (p->memKey, 
	    p->lockFile, p->size, &new);

	/*  Initialize a new segment header.
         */
	if (new) {
	    seg->smc     = smc;
	    seg->pagenum = pnum;
	    seg->memKey  = p->memKey;
	    seg->shmid   = new;
	    seg->size    = p->size;		/* entire segment	*/
	    seg->dsize   = size;		/* requested size only	*/
	    seg->type    = type;
	    seg->ctime   = smUtilTime ();
fprintf (stderr, "size=%d  p->size=%d  segSize=%d  [%d] \n", 
    (int)size, (int)p->size, (int)sizeof (smcSegment_t), (int)(size + sizeof(smcSegment_t)) );

    	    /*  Inherit from the SMC fields we'll be using.
     	     */
    	    seg->seqno = smc->seqno;
    	    strcpy (seg->dirpath, smc->dirpath);
    	    strcpy (seg->froot, smc->froot);

	    /*  TODO: Need to make sure this pointer is word-aligned!!
    	    seg->data = (void *) (SEG_ADDR(p) + sizeof (smcSegment_t) + 1);
    	    seg->data = (void *) (sp + sizeof (smcSegment_t) + 1);
	     */
    	    seg->data  = (void *) (sp + sizeof (smcSegment_t));
    	    seg->edata = (char *) (seg->data + size);


	    /*  Zero and copy the data to the new segment.  Note we clear the
	     *  entire segment, but copy only the header data.
	     */
	    memset (sp, 0, p->size);
	    bcopy (seg, sp, sizeof(smcSegment_t));

	    free (seg);				/* free local storage	*/

	}
	p->initialized = TRUE;

	/*  Add us to the state struct so we know who's using the cache.
	 */
	smStateAdd (p->pstate, smUtilTime (), (void *)sp, attach=TRUE);
        smStateSetAddr (p->pstate, sp);
        smStateSetAttached (p->pstate, TRUE);

    } else {
	p->initialized = FALSE;
	smStateSetAttached (p->pstate, attach=FALSE);

	if (smc->debug)
	    fprintf (stderr, "getPage not attaching to seg=0x%x\n", (int)seg);
    }
    p->free = FALSE;
    smStateSetSMC (p->pstate, smc);	/* save local address of SMC */


    /* Lock the segment?  
     */
    if (lock_flag)
	smcLock (p);


    if (smc->debug) 
	fprintf (stderr, "getPage:  npages=%d top=%d new=shmid=0x%x\n\n", 
	    smc->npages, smc->top, new);

    return ((smcPage_t *) p);
}


void
smcValidatePage (char *where, smcPage_t *page)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(page);
#ifdef USE_32BIT
    int edata = (int)(smcGetPageData (page) + seg->dsize + 1);

    if (*(char *)edata == '\0') {
#else
    char *edata = (char *)(smcGetPageData (page) + seg->dsize + 1);

    if (*edata == '\0') {
#endif
	fprintf (stderr, "Valid page: 0x%x  : %s\n", (int)page, where);
        fflush (stderr);
    } else
	smcValidateTrap();
}
	

void
smcValidateTrap() { int debug; debug = 1; }


/*  Mutual exclusion locks.  Because we have mutiple processes using the 
 *  SMC simultaneously, we must be able to provide a lock for exclusive 
 *  access during critical parts of the code.   This is done using a simple
 *  semaphore system that blocks the calling process until the SMC is "freed"
 *  by the competing process.
 *	Typical usage would be something like:
 *
 *      	smcMutexOn ();
 *		  <....some critical SMC code....>
 *      	smcMutexOff ();
 */
void
smcMutexInit (int id)
{
    char  err[SZ_LINE];


    if (smc_sem_initialized)
	return;

    /* Create a semaphore set with the specified id and access
    ** only by the owner.
    */
    if ((smc_sem_id = semget (id, 1, IPC_CREAT | 0600)) < 0) {
	sprintf (err, "smcMutexInit: semget fails on id=%d", id);
	perror (err);
	exit (1);
    }

    /* Initialize the value to to '1'.
    */
    if (semctl (smc_sem_id, 0, SETVAL, 1) < 0) {
	sprintf (err, "smcMutexInit: semctl fails on sem_id=%d", smc_sem_id);
	perror (err);
	exit (1);
    }
        
    smc_sem_initialized = 1;
}


void
smcMutexOn ()
{
    struct sembuf sem_op;

    if (smc_sem_initialized == 0)
	smcMutexInit ( ( SMC_SEM_ID + getuid() ) );

    /* Wait on the semaphore unless it is non-negative.
    */
    sem_op.sem_num = 0;
    sem_op.sem_op  = -1;
    sem_op.sem_flg = 0;
    semop (smc_sem_id, &sem_op, 1);
}


void
smcMutexOff ()
{
    struct sembuf sem_op;

    if (smc_sem_initialized == 0)
	smcMutexInit ( ( SMC_SEM_ID + getuid() ) );

    /* Signal the semaphore and increase its value by one.
    */
    sem_op.sem_num = 0;
    sem_op.sem_op  = 1;
    sem_op.sem_flg = 0;
    semop (smc_sem_id, &sem_op, 1);
}




/*  Filename Attribute Sub-Interface
 *
 *	  	       smcSetDir  (smc, dirpath)   // Filename Creation attrs
 *	         dir = smcGetDir  (smc)
 *	             smcSetFRoot  (smc, froot)
 *           froot = smcGetFRoot  (smc)
 *	             smcSetSeqNo  (smc, seqnum)
 *          seqnum = smcGetSeqNo  (smc)
 *	            smcIncrSeqNo  (smc)
 *	            smcDecrSeqNo  (smc)
 *	           smcResetSeqNo  (smc)
 *
 *	     dir = smcGetPageDir  (page)
 *       froot = smcGetPageFRoot  (page)
 *      seqnum = smcGetPageSeqNo  (page)
 *
 *      The sequence number is a running counter that can be used to create 
 *  unique filenames.  Client tasks are responsible for deciding when/how 
 *  to use this value or manupulate it, however each new Page created will 
 *  inherit the current value from the SMC.
 *
 *  FIXME:  Locking of the SMC may be required here......
 */

void smcSetDir   (smCache_t *smc, char *dirpath) {
    			strncpy (smc->dirpath, dirpath, SZ_PATH); 	   }
void smcSetFRoot (smCache_t *smc, char *froot) {
    			strncpy (smc->froot, froot, SZ_FNAME); 		   }
void smcSetSeqNo (smCache_t *smc, int seqnum) { smc->seqno = seqnum; 	   }

char *smcGetDir   (smCache_t *smc) 	{ return (smc->dirpath); 	   }
char *smcGetFRoot (smCache_t *smc) 	{ return (smc->froot); 		   }
int   smcGetSeqNo (smCache_t *smc) 	{ return (smc->seqno); 		   }


char *smcGetPageDir   (smcPage_t *p)
{ 
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(p);
    return (seg->dirpath); 
}

char *smcGetPageFRoot (smcPage_t *p)
{ 
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(p);
    return (seg->froot);   
}

int   smcGetPageSeqNo (smcPage_t *p)
{ 
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(p);
    return (seg->seqno);   
}

int   smcSetPageSeqNo (smcPage_t *p, int seqno)
{ 
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(p);
    return (seg->seqno = seqno);   
}

void  smcIncrSeqNo (smCache_t *smc)	{ smc->seqno++;   		   }
void  smcDecrSeqNo (smCache_t *smc)     { smc->seqno--; 		   }
void  smcResetSeqNo (smCache_t *smc)    { smc->seqno = 0; 		   }





/*  Page Management Interface
 *
 *     data = smcGetPageData  (page)		// Managed Page Interface
 *	         smcFreePage  (page)
 *	             smcLock  (page)
 *	           smcUnlock  (page)
 *
 *           who = smcGetWho  (page)		// Page Attributes
 *                 smcSetWho  (page, who)
 *       expID = smcGetExpID  (page)
 *               smcSetExpID  (page, expID)
 *       colID = smcGetColID  (page)
 *               smcSetColID  (page, colID)
 *   obsetID = smcGetObsetID  (page)
 *             smcSetObsetID  (page, obsetID)
 *       colID = smcGetColID  (page)
 *               smcSetColID  (page, colID)
 *    num = smcGetExpPageNum  (page)
 *          smcSetExpPageNum  (page, expPageNum)
 *       md = smcGetMDConfig  (page)
 *            smcSetMDConfig  (page, mdConfig)
 *       fp = smcGetFPConfig  (page)
 *            smcSetFPConfig  (page, fpConfig)
 */


/*  SMCGETPAGEDATA -- Return the pointer to the start of the data area for
 *  the requested page.  The pointer returned is the address in the local
 *  process space.
 */
void *
smcGetPageData (smcPage_t *page)
{
    void *addr;


    if (page == (smcPage_t *)NULL) {
	fprintf (stderr, "Null page pointer.\n");
	return ((void *) NULL);

    } else if (page->initialized == FALSE) {
	fprintf (stderr, "Attempt to access uninitialized page.\n");
	return ((void *) NULL);
    }

    /*  Return an address relative to the page segment since the segment
     *  addr refers to the current process space and not an absolute addr.
     */
    addr = (void *) (SEG_ADDR(page) + sizeof (smcSegment_t) + 1);

    return (addr);
}


/*  SMCFINDPAGE -- Return the page pointer for the given memKey.  A NULL
 *  pointer is returned if no page is found.
 */
smcPage_t *
smcFindPage (smCache_t *smc, key_t memKey)
{
    int   i;
    smcPage_t *p = (smcPage_t *) NULL;


    /*  Loop through the pages looking for given key.
     */
    for (i=0; i < smc->npages; i++) {
        p = &smc->pdata[i];
	if (p->memKey == memKey)
	    break;
    }

    return (p);
}


/*  SMCNEXTPAGE -- Return a pointer to the 'next' page to be processed.  Pages
 *  are stamped with a creation time and each pid using the cache carries
 *  with it the time it last checked for an update.  Simply search the list
 *  of pages and return the "oldest" page.   
 *
 *  If the 'timeout' is non-zero we pause the specified number of milliseconds
 *  before looking for an update, otherwise we return immediately with a
 *  single page.  This allows the function to be used in a non-blocking call
 *  or as an iterating poll.
 */
smcPage_t *
smcNextPage (smCache_t *smc, int timeout)
{
    register int    i;
    smcPage_t *p = (smcPage_t *) NULL;
    smcPage_t *oldest = (smcPage_t *) NULL;
    XLONG interval = smc->sysConfig.interval;


    while (1) {

        /*  Loop through the pages looking for "new" ones.
         */
        for (i=0; i < smc->top; i++) {
            p = &smc->pdata[i];

	    if (smStateDefined (p->pstate) == FALSE) {
		void *addr;

		/* Compute the local-process address of the segment. 
		 */
		addr = (void *)(p + sizeof (smcSegment_t) + 1);
	        smStateAdd (p->pstate, p->ctime, addr, FALSE);
	    }

	    /* Skip locked pages and empty slots.
	     */
	    if (p->memKey == (key_t)NULL || p->ac_locked == TRUE) 
	        continue;			

	    else {
	        /*  Page is updated more recently than the last time we
	         *  checked it.
	         */
		if (smc->debug > 1) {
		    fprintf (stderr, "page%d(0x%x):  utime=%f   ltime=%f   ",
		        i, p->memKey, SEG_UTIME(p), SEG_LTIME(p));
		    if (oldest) 
		        fprintf (stderr, "oldest=%d\n", oldest->memKey); 
		    else 
		        fprintf (stderr, "\n");
		}

	        if (SEG_UTIME(p) == 0.0) {
	            if (oldest == (smcPage_t *)NULL) oldest = p;
		    if (SEG_LTIME(p) < SEG_LTIME(oldest))
	                oldest = p;

	        } else if (SEG_UTIME(p) < SEG_LTIME(p)) {
	            if (oldest == (smcPage_t *)NULL) oldest = p;
		    if (SEG_UTIME(p) < SEG_UTIME(oldest))
	                oldest = p;

	        } else if (SEG_UTIME(p) >= SEG_LTIME(p)) {
		    continue;
	        } 
	    }
        }

	if (oldest != (smcPage_t *) NULL) {
	    if (smc->debug)
		fprintf (stderr, "nextPage:  memKey=0x%x\n", oldest->memKey);
    
	    /* Update the UTime (last-checked-for-update) for this page.
	     */
    	    smStateSetUTime (oldest->pstate, smUtilTime ());

            return (oldest);
	}

	if (timeout > 0) {
	   int  stime = min (interval, timeout);
	   if (stime < 1000)
	       usleep ((unsigned long)(stime * 1000));
	   else
	       sleep (1);
	} else 
	   break;
    }
            
    return ((smcPage_t *) oldest);
}


/*  SMCNEXTBYEXPID -- Return a pointer to the next page to be processed given
 *  an ExpID as the identifier.  This procedure acts both as a search
 *  function, collecting all pages marked with the expID when a new expID is
 *  seen, and as an iterator for returning the next page in the list.  A NULL
 *  page pointer is returned at the end of the list.
 * 
 *  NOTE: The page list created here not sorted in time.  It isn't clear this
 *  is needed at this point but the caller cannot assume a particular order
 *  to the pages returned here.
 */

#define MAX_ID_PAGES	128

smcPage_t *
smcNextByExpID (smCache_t *smc, double expID)
{
    int   i;
    smcPage_t *p = (smcPage_t *) NULL;

    static double listID = 0.0;
    static int npages = 0, curPage = 0;
    static smcPage_t *plist[MAX_ID_PAGES];


    /*  A negative expID means simply return the next page in the SMC.  This
     *  is useful when the caller might not know the expID.
     */
    if (expID < 0.0)
	return (smcNextPage (smc, 0));	


    /* Begin a new search.
    **
    printf ("expID=%.9lf  listID=%.9lf  curPage=%d  npages=%d\n",
	expID, listID, curPage, npages);
    */
    if (expID != listID) { 			
	npages = curPage = 0; 		/* Initialize */
	listID  = expID;
	memset (plist, 0, (MAX_ID_PAGES * sizeof(smcPage_t *)));

        /*  Loop through the pages looking for ones with the requested ID.
         */
        for (i=0; i < smc->top; i++) {
            p = &smc->pdata[i];

	    if (p->memKey == (key_t)NULL || p->ac_locked == TRUE) 
	        continue;			

	    if (smcEqualExpID(p->expID,expID))	/* look for a match	  */
		plist[npages++] = p;
	}

	return (plist[curPage] );	/* return the first found or NULL */

    } else { 
	/* return the next page in the list. */
	if (plist[curPage+1]) {
	    return (plist[++curPage] );
	} else  {
	    listID = 0.0;
	    return ((smcPage_t *) NULL);
	}
    }
}


/*  SMCPRUNE -- Scan the page data array, free any unused segments and 
 *  re-order the array to remove any voids.  Note that the cache must be
 *  locked during this procedure.
 */
void
smcPrune  (smCache_t *smc)
{
    int i=0, j=0, top = smc->top;
    smcPage_t *p = (smcPage_t *) NULL;
    smcSegment_t *seg = (smcSegment_t *) NULL;


    /* Update the high-water mark. 
     */
    if (top > smc->np_hiwater)
	smc->np_hiwater = top;

    /*  Compact the page array.
     */
    if (smc->debug)
	printf ("B4 pruning:  npages = %d  top = %d\n", smc->npages, smc->top);

    for (i=0; i < top; i++) {
        p = &smc->pdata[i];
	seg = (void *)SEG_ADDR(p);

	if (p->memKey == (key_t)NULL) {
	    if (smc->debug) 
		printf ("pruning page %d... \n", i);

            for (j=i; j < (top-1); j++)
		memcpy ((void *)&smc->pdata[j], (void *)&smc->pdata[j+1],
		    sizeof(smcPage_t));

	    /*  Everything has been shifted 'down', clear the top.
	     */
    	    memset (&smc->pdata[j], 0, sizeof(smcPage_t));
	    top--;
	}
    }
    smc->top = top;

    if (smc->debug) {
	smcListPages (smc, -1);
	printf ("After pruning:  npages = %d  top = %d\n", 
	    smc->npages, smc->top);
    }
}


/*  SMCRESET -- Reset the SMC by deleting all active pages and freeing
 *  their resources.
 */
void
smcReset  (smCache_t *smc)
{
    int    i=0, top = smc->top;
    smcPage_t *p = (smcPage_t *) NULL;
    smcSegment_t *seg = (smcSegment_t *) NULL;


    /*  Compact the page array.
     */
    if (smc->debug)
	printf ("Before reset: npages = %d  top = %d\n", smc->npages, smc->top);

    for (i=0; i < top; i++) {
        p = &smc->pdata[i];
	seg = (void *)SEG_ADDR(p);

	smcDetach (smc, p, TRUE);
    }

    if (smc->debug) {
	smcListPages (smc, -1);
	printf ("After reset: npages = %d  top = %d\n", smc->npages, smc->top);
    }
}


/*  SMCFREEPAGE -- Free the named page.  By 'free' we mean we remove the
 *  lock file assuming all processes have been detached.
 */
void
smcFreePage (smcPage_t *page)
{
    memset (page, 0, sizeof (smcPage_t));
    page->free = TRUE;
}


/*  SMCATTACH -- Attach to the named page.  We assume the page already exists
 *  and simply attach the current process here.
 */
void
smcAttach (smCache_t *smc, smcPage_t *page)
{
    void *addr = (void *)NULL;
    int  create = FALSE;
 

    /*
    if (smc->debug && smc->sysConfig.verbose)
	fprintf (stderr, "Attaching page %s, key=0x%x...", 
	    page->lockFile, page->memKey);
    */
	    
    if (page->memKey == (key_t) NULL) {
	fprintf (stderr, "smcAttach: Invalid key on page 0x%x\n", (int)page);
	return;
    }

    addr = smUtilAttach (page->memKey, page->lockFile, page->size, &create);

    if (smc->debug)
	fprintf (stderr, "Attaching page %s, key=0x%x size=%d addr=0x%x...\n", 
	    page->lockFile, (int)page->memKey, page->size, (int)addr);

    /*
    if (smc->debug && smc->sysConfig.verbose)
	fprintf (stderr, "addr=0x%x\n", addr);
    */
	    

    /* Add us to the state for the segment if we're not already there,
     * this will also force a cleanup of the state array.
     */
    if (!smStateDefined (page->pstate))
	smStateAdd (page->pstate, smUtilTime (), (void *)addr, TRUE);

    smStateSetAddr (page->pstate, addr);
    smStateSetSMC (page->pstate, smc);
    smStateSetAttached (page->pstate, TRUE);
}


/*  SMCDETACH -- Detach from the named page.
 */
void
smcDetach (smCache_t *smc, smcPage_t *page, int free)
{
    /*  Attach to segment so we can properly clean it up. 
     */
    if (free && SEG_ATTACHED(page) <= 0)
        smcAttach (smc, page);

    /*  If we're leaving the page, be sure to unlock it first.
     */
    if (page->cpid == smPid())
        smcUnlock (page);

    /*  Detach from the segment.
     */
    smUtilDetach (page->memKey, page->lockFile, (void *)SEG_ADDR(page), free);
    smStateSetAttached (page->pstate, FALSE);

    /*  Clear the page associated with the cache.
     */ 
    if (free) {
	long sz = page->size;

	smc->npages--;
	memset (page, 0, sizeof (smcPage_t));
        page->free = TRUE;
	if (page == &smc->pdata[smc->top-1])
	    smc->top--;

	/*  Update the memory usage counters. */
	smc->mem_allocated -= sz;
	smc->mem_avail += sz;
    }
}


/*  SMCLOCK -- Lock a particular page.  This is simply a "soft lock" on 
 *  the page where the flag indicates the process which created the page
 *  isn't done writing to it yet.  Writer processes are responsible for
 *  explicitly releasing the lock, this is done by default when the creating
 *  process detaches.
 */
void
smcLock (smcPage_t *page)
{
    page->ac_locked = TRUE;
}


/*  SMCUNLOCK -- Unlock a particular page.  We also update the LTime to
 *  flag the (assumed) modification to the page.
 */
void
smcUnlock (smcPage_t *page)
{
    page->ac_locked = FALSE;
    if (SEG_ATTACHED(page) > 0)
        smStateSetLTime (page->pstate, smUtilTime ());
}



/*****************************************************************************
**
**  GET/SET methods for the NEWFIRM segment attributes.
**
*****************************************************************************/

/*  SMCGETWHO -- Return the 'who' attribute from the segment header.
 */
XLONG
smcGetWho (smcPage_t *page)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    return ((seg ? (XLONG)seg->who : -1));
}


/*  SMCSETWHO -- Set the 'who' attribute in the segment header.
 */
void
smcSetWho (smcPage_t *page, XLONG who)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    seg->who = who;
}


/*  SMCGETEXPID -- Return the 'expID' attribute from the segment header.
 */
double
smcGetExpID (smcPage_t *page)
{
    return ((double)page->expID);
}


/*  SMCSETEXPID -- Set the 'expID' attribute in the segment header.
 */
void
smcSetExpID (smcPage_t *page, double expid)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    seg->expID = page->expID = expid;
}


/*  SMCGETOBSETID -- Return the 'obsetID' attribute from the segment header.
 */
char *
smcGetObsetID (smcPage_t *page)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    return ((seg ? (char *)seg->obsetID : (char *)NULL));
}


/*  SMCSETOBSETID -- Set the 'obsetID' attribute in the segment header.
 */
void
smcSetObsetID (smcPage_t *page, char *obsetID)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    strncpy (seg->obsetID, obsetID, strlen (obsetID));
}


/*  SMCGETCOLID -- Return the 'colID' attribute from the segment header.
 */
char *
smcGetColID (smcPage_t *page)
{
    return ((char *)page->colID);
}


/*  SMCSETCOLID -- Set the 'colID' attribute in the segment header.
 */
void
smcSetColID (smcPage_t *page, char *colID)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    if (seg)
        strncpy (seg->colID, colID, strlen (colID));
    strncpy (page->colID, colID, strlen (colID));
}


/*  SMCGETEXPPAGENUM -- Return the 'expPageNum' attribute from the segment.
 */
int
smcGetExpPageNum (smcPage_t *page)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    return ((seg ? seg->expPageNum : -1));
}


/*  SMCSETEXPPAGENUM -- Set the 'expPageNum' attribute in the segment header.
 */
void
smcSetExpPageNum (smcPage_t *page, int expPageNum)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_PTR(page);
    seg->expPageNum = expPageNum;
}


/*  SMCGETMDCONFIG -- Return the mdConfig struct from the segment header.
 */
mdConfig_t *
smcGetMDConfig (smcPage_t *page)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(page);
    return ((mdConfig_t *)&seg->mdCfg);
}


/*  SMCSETMDCONFIG -- Set the mdConfig struct in the segment header.
 */
void
smcSetMDConfig (smcPage_t *page, mdConfig_t *mdConfig)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(page);
    bcopy (mdConfig, &seg->mdCfg, sizeof(mdConfig_t));
}


/*  SMCGETFPCONFIG -- Return the fpConfig struct from the segment header.
 */
fpConfig_t *
smcGetFPConfig (smcPage_t *page)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(page);
    return ((fpConfig_t *)&seg->fpCfg);
}


/*  SMCSETFPCONFIG -- Set the fpConfig struct in the segment header.
 */
void
smcSetFPConfig (smcPage_t *page, fpConfig_t *fpConfig)
{
    smcSegment_t *seg = (smcSegment_t *)SEG_ADDR(page);
    bcopy (fpConfig, &seg->fpCfg, sizeof(fpConfig_t));
}



/*******************************************************************************
**  UTILITY/DEBUG ROUTINES
*******************************************************************************/

/*  SMCTOGGLEDEBUG -- Toggle the debug flag
 */
void
smcToggleDebug (smCache_t *smc) 
{
    if (smc) 
	smc->debug = (smc->debug ? 0 : 1); 
}


/*   SMCTYPE2STR -- Convert a type code to a string for easy viewing.
 */
char *
smcType2Str (int type)
{
    switch (type) {
    case TY_VOID: 	
	return ("TY_VOID");
    case TY_DATA: 	
	return ("TY_DATA");
    case TY_META: 	
	return ("TY_META");
    default: 		
	return ("INDEF");
    }
}


/* Print the cache runtime structure.
 */
void
smcPrintCacheInfo (smCache_t *smc, char *title)
{
    FILE *fd = stderr;
    int   i;

    if (!smc)
	return;

    fprintf (fd, "%s\n", (title ? title : "") );
    fprintf (fd, "   smCache {\t\t\t0x%x\n", (int) smc);
    fprintf (fd, "            memKey = 0x%x\n", (int) smc->memKey);
    fprintf (fd, "             shmid = %ld\n",  smc->shmid);
    fprintf (fd, "        cache_addr = 0x%x\n", (int) smc->cache_addr);
    fprintf (fd, "        cache_size = %d\n",   smc->cache_size);
    fprintf (fd, "         vm_locked = %d\n",   smc->vm_locked);
    fprintf (fd, "             ctime = %s",     smUtilTimeStr(smc->ctime));
    fprintf (fd, "               new = %d\n",   smc->new);
    fprintf (fd, "            np_max = %d\n",   smc->np_max);
    fprintf (fd, "        np_hiwater = %d\n",   smc->np_hiwater);
    fprintf (fd, "     mem_allocated = %d\n",   smc->mem_allocated);
    fprintf (fd, "         mem_avail = %d\n",   smc->mem_avail);
    fprintf (fd, "            npages = %d\n",   smc->npages);
    fprintf (fd, "               top = %d\n",   smc->top);
    fprintf (fd, "             pdata = 0x%x\n", (int) &smc->pdata);
    for (i=0; i < MAX_PROCS && smc->pstate[i].pid; i++) {
        fprintf (fd, "            %s pid:%d  addr:0x%x  %s",
				(i ? "        " : "pstate ="),
				(int) smc->pstate[i].pid,
				(int) smc->pstate[i].addr,
				smUtilTimeStr (smc->pstate[i].atime));
    }
    fprintf (fd, "             debug = %d\n",   smc->debug);
    fprintf (fd, "   }\n");

}


/* Print the cache configuration info.
 */
void
smcPrintCfgInfo (sysConfig_t *cfg, char *title)
{
    FILE *fd = stderr;

    if (!cfg)
	return;

    fprintf (fd,"%s\n",                    (title ? title : ""));
    fprintf (fd, "   sysConfig {\t\t\t(0x%x)\n", (int) cfg);
    fprintf (fd,"       cache_path = '%s'\n", 
			(cfg->cache_path ? cfg->cache_path : ""));
    fprintf (fd,"           config = '%s'\n", (cfg->config ? cfg->config : ""));
    fprintf (fd,"       cache_size = %d\n",   cfg->cache_size);
    fprintf (fd,"     cache_memKey = 0x%x\n", (int) cfg->cache_memKey);
    fprintf (fd,"            nsegs = %d\n",   cfg->nsegs);
    fprintf (fd,"     min_seg_size = %d (bytes)\n", cfg->min_seg_size);
    fprintf (fd,"       lock_cache = %d\n",   cfg->lock_cache);
    fprintf (fd,"        lock_segs = %d\n",   cfg->lock_segs);
    fprintf (fd,"\t   sys_page_size = %d\n", cfg->sys_page_size);
    fprintf (fd,"\t  sys_phys_pages = %d\n", cfg->sys_phys_pages);
    fprintf (fd,"\tsys_avphys_pages = %d\n", cfg->sys_avphys_pages);
    fprintf (fd,"\t       sys_pmask = %d\n", cfg->sys_pmask);
    fprintf (fd, "   }\n\n");
}


/* Print Page list info.
 */
void
smcPrintPageList (smCache_t *smc, char *title)
{
    register int  i=0;

    if (!smc)
	return;

    for (i=0 ; i < smc->npages; i++)
	smcPrintPageInfo (&smc->pdata[i], i);
}


/* Print Page list info.
 */
void
smcPrintPageInfo (smcPage_t *page, int pagenum)
{
    FILE *fd = stderr;
    int   i;

    if (!page)
	return;

    fprintf (fd,"   smcPage (%2d) {\t\t0x%x\n", pagenum, (int)page);
    fprintf (fd,"           smc = 0x%x\n", (int) SEG_SMC(page));
    fprintf (fd,"      lockFile = '%s'\n", page->lockFile);
    fprintf (fd,"        memKey = 0x%x\n", (int) page->memKey);
    fprintf (fd,"          size = %d\n", page->size);
    fprintf (fd,"          free = %d\n", page->free);
    fprintf (fd,"          time = %s",   smUtilTimeStr (page->ctime));
    fprintf (fd,"          type = %s\n", smcType2Str (page->type));
    fprintf (fd,"     nattached = %d\n", page->nattached);
    fprintf (fd,"      refcount = %d\n", page->refcount);

    fprintf (fd,"     ac_locked = %d\n", page->ac_locked);
    fprintf (fd,"     vm_locked = %d\n", page->vm_locked);

    for (i=0; i < MAX_PROCS && page->pstate[i].pid; i++) {
        fprintf (fd, "        %s %c pid:%d  addr:0x%x  att:%d  %s",
				(i ? "        " : "pstate ="),
				(getpid() == page->pstate[i].pid ? '*' : ' '),
				(int) page->pstate[i].pid,
				(int) page->pstate[i].addr,
				(int) page->pstate[i].attached,
				smUtilTimeStr (page->pstate[i].atime));
    }

    if (SEG_ADDR(page))	
	smcPrintSegmentInfo (SEG_PTR(page), pagenum, NULL);
    else
        fprintf (fd,"      segment = { NULL }\n");

    fprintf (fd, "   }\n\n");
}


/* Print segment list info.
 */
void
smcPrintSegmentInfo (smcSegment_t *seg, int segno, char *title)
{
    FILE *fd = stderr;
    register int  i=0;

    if (!seg)
	return;

    if (title)
        fprintf (fd,"%s\n", title);

    fprintf (fd,"        smcSegment (%2d) {\t0x%x\n", segno, (int) seg);
    fprintf (fd,"           memKey = 0x%x\n", (int) seg->memKey);
    fprintf (fd,"             size = %ld\n", (long) seg->size);
    fprintf (fd,"             type = %d\n", seg->type);
    fprintf (fd,"             time = %s",   smUtilTimeStr (seg->ctime));

    fprintf (fd,"              who = %ld\n", (long) seg->who);
    fprintf (fd,"            expID = %g\n", seg->expID);
    fprintf (fd,"          obsetID = '%s'\n", (seg->obsetID?seg->obsetID:""));
    fprintf (fd,"            dsize = %ld\n", (long) seg->dsize);
    fprintf (fd,"             data = 0x%x\n", (int) seg->data);

    fprintf (fd,"	 fpConfig = {\t\t0x%x\n", (int) &seg->fpCfg);
    fprintf (fd,"             [xy]Size = %ld  %ld\n", 
				    (long) seg->fpCfg.xSize, 
				    (long) seg->fpCfg.ySize);
    fprintf (fd,"            [xy]Start = %ld  %ld\n", 
				    (long) seg->fpCfg.xStart, 
				    (long) seg->fpCfg.yStart);
    fprintf (fd,"             dataType = %ld\n", (long) seg->fpCfg.dataType);
    fprintf (fd,"	 }\n");

    fprintf (fd,"	 mdConfig = {\t\t0x%x\n", (int) &seg->mdCfg);
    fprintf (fd,"             metaType = %ld\n", (long) seg->mdCfg.metaType);
    fprintf (fd,"            numFields = %ld\n", (long) seg->mdCfg.numFields);
    for (i=0; i < seg->mdCfg.numFields; i++)
      fprintf (fd,"            %d: fieldSize = %ld    dataType = %ld\n",
	    i, (long) seg->mdCfg.fieldSize[i], (long) seg->mdCfg.dataType[i]);
    fprintf (fd,"	 }\n");

    fprintf (fd, "       }\n\n");
}


/* Print a summary list of pages.
 */
void
smcListPages (smCache_t *smc, int npages)
{
    int   i, np = ((npages < 0) ? max(smc->top+2,smc->npages) : npages);
    smcPage_t *p = (smcPage_t *) NULL;
    smcSegment_t *s = (smcSegment_t *) NULL;

    for (i=0; i < np; i++) {
	p = &smc->pdata[i];
	s = SEG_PTR(p);

	fprintf(stderr, "(%d) key=0x%08x addr=0x%08x sz=%8d ",
	        i, p->memKey, (int) &smc->pdata[i], p->size);
	fprintf(stderr, "type=%d final=%d%d%d%d%d seg=0x%x\n",
	        p->type, p->free, p->initialized, p->nattached, 
		SEG_ATTACHED(p), p->ac_locked, (int) s);
    }
}


/* Test two ExpID for equality within some precision.

#define equal_expid(a,b) ((abs(a-b)<0.008))

 */
int
smcEqualExpID (double a, double b)
{
    double tol = 0.000001;

    if (a < b) {
	return ( ((b-a) < tol) ? 1 : 0 );
    } else { 
	return ( ((a-b) < tol) ? 1 : 0 );
    }
}
