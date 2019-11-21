#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stddef.h>

#include "smCache.h"

/*
 *  SMCOP -- Utility task for commandline operations on the Shared Memory
 *  Cache.  This allows us to script SMC operations by chaining together
 *  any number of command flags.  The task will process these in order, 
 *  so for example
 *
 * 	smcop -pan -list -init -list
 *
 *  will simulate a readout from the PAN (i.e. populate the SMC), list
 *  the managed pages, initialize the SMC and then list what remains (which
 *  should simply be the SMC segment itself).
 *
 *  Usage:
 *         smcop [-cache <file>] [-debug] [-verbose] [-help] <cmd_flags>

 *  where <cmd_flags> is and combination of:
 * 
 *     -clean              cleanup orphaned pages
 *     -create             create an SMC
 *     -destroy            destroy the SMC
 *     -init               initialize the SMC
 *     -list               list managed pages
 *     -pan                simulate PAN readout
 *     -dca                simulate DCA readout
 * 
 *  Command abbreviations are supported.
 */


#ifdef MACOSX

/* ipcs ctl commands */
# define SHM_STAT       13
# define SHM_INFO       14

/* shm_mode upper byte flags */
# define SHM_DEST       01000   /* segment will be destroyed on last detach
# */
# define SHM_LOCKED     02000   /* segment will not be swapped */
# define SHM_HUGETLB    04000   /* segment is mapped via hugetlb */

struct  shminfo
  {
    unsigned long int shmmax;
    unsigned long int shmmin;
    unsigned long int shmmni;
    unsigned long int shmseg;
    unsigned long int shmall;
    unsigned long int __unused1;
    unsigned long int __unused2;
    unsigned long int __unused3;
    unsigned long int __unused4;
  };

struct shm_info
  {
    int used_ids;
    unsigned long int shm_tot;  /* total allocated shm */
    unsigned long int shm_rss;  /* total resident shm */
    unsigned long int shm_swp;  /* total swapped shm */
    unsigned long int swap_attempts;
    unsigned long int swap_successes;
  };

#endif /* __USE_MISC */



extern smCache_t *smcOpen();
extern smcPage_t *smcGetPage();

smCache_t *smc  = (smCache_t *)NULL;
smcPage_t *page = (smcPage_t *)NULL;
smcSegment_t *seg = (smcSegment_t *)NULL;
    
int  imnum	= 0, 
     color_mode	= 1, 
     im_nx	= 2048, 
     im_ny	= 2048, 
     im_bitpix	= 32;

int  verbose	= FALSE;


main (int argc, char **argv) 
{
    char  fname[SZ_FNAME], config[SZ_LINE];
    int   init=FALSE, verbose=FALSE, debug=FALSE, clean=FALSE;
    int   list=FALSE, all=FALSE, seq=FALSE;
    register int  i, j, stat;



    /* Process command-line arguments.  Make a first pass looking only for
     * flags particular to the cache, e.g. the lockfile name.
     */
    bzero (fname, SZ_FNAME);
    for (i=1; i < argc; i++) {
        if (strncmp (argv[i], "-cache", 3) == 0) {
            strcpy (fname, argv[++i]); 			/* set cache file  */
        } else if (strncmp (argv[i], "-all", 2) == 0) {
            all++;					/* list-all flag   */
        } else if (strncmp (argv[i], "-debug", 4) == 0) {
            debug++;					/* debug flag      */
        } else if (strncmp (argv[i], "-verbose", 2) == 0) {
            verbose++;					/* verbose  flag   */
        } else if ((strncmp (argv[i], "-help", 2) == 0)) {
            Usage ();
        }
    }

    /* Initialize the config string and Open/Attach to the cache.
     */
    if (verbose)
	fprintf (stderr, "Opening cache....\n");
    sprintf (config, "debug=%d,cache_file=%s", debug, fname);
    if ((smc = smcOpen (config)) == (smCache_t *)NULL) {
        fprintf (stderr, "Error opening cache, invalid file?.\n");
	exit (1);
    }

    /* Now loop through the args again and process in order, this allows us
     * to create a pipeline of operations easily.
     */
    for (i=1; i < argc; i++) {

        if (strncmp (argv[i], "-clean", 2) == 0) {
            clean++;					/* clean flag      */

        } else if (strncmp (argv[i], "-init", 2) == 0) {
            /* Initialize the SMC.
	     */
	    if (smcInitialize (smc) != OK) {
	        fprintf (stderr, "Error initializing SMC.\n");
	        exit (1);
	    }
	    if (verbose) 
		smcPrintCacheInfo (smc, (char *)NULL);

        } else if (strncmp (argv[i], "-create", 2) == 0) {
	    /* essentially a no-op, cache was created above....
	     */

        } else if (strncmp (argv[i], "-destroy", 4) == 0) {
	    if (smcInitialize (smc) == OK) 
    		smcClose (smc, TRUE);
	    exit (0);

        } else if (strncmp (argv[i], "-dir", 4) == 0) {
	    if (argc > (i+1)) {
		if (argv[i+1][0] != '-') 
		    smcSetDir (smc, argv[++i]);
	    }

        } else if (strncmp (argv[i], "-list", 2) == 0) {
            /* List the current SMC segments.
	     */
	    if (verbose) 
		smcPrintCacheInfo (smc, (char *)NULL);
	    smcopListPages (smc, smc->top);

        } else if (strncmp (argv[i], "-LIST", 2) == 0) {
            /* List the current SMC segments in a continuous loop.
	     */
	    int interval = 1;

	    if (argc > (i+1))
		if (argv[i+1][0] != '-')
		    interval = atoi(argv[++i]);
	    while (1) {
		system ("clear");
	        if (verbose) 
		    smcPrintCacheInfo (smc, (char *)NULL);
	        smcopListPages (smc, smc->top);
		system ("date");

		sleep (interval);
	    }

        } else if (strncmp (argv[i], "-pan", 3) == 0) {
            /* Generate and artificial readout sequence.
	     */
            if (verbose)
                printf ("Generating artificial readout %d (%d/%d) ....", 
                        imnum++, smc->npages, smc->top);
	    smcIncrSeqNo (smc);
            simMetaData ();
                simData ();
            simMetaData ();
            if (verbose)
                printf ("done.\n");

        } else if (strncmp (argv[i], "-root", 5) == 0) {
	    if (argc > (i+1)) {
		if (argv[i+1][0] != '-') 
		    smcSetFRoot (smc, argv[++i]);
	    }

        } else if (strncmp (argv[i], "-seqno", 4) == 0) {
	    int seqno = 1;

	    if (argc > (i+1))
		seqno = (argv[i+1][0] != '-') ? atoi(argv[++i]) : 1;
	    smcSetSeqNo (smc, seqno);

        } else if (strncmp (argv[i], "-dca", 4) == 0) {
            /* Generate and artificial readout sequence.
             */

	    for (j=0; j < 3 && (page = smcNextPage (smc, 0)); j++) {
	        smcAttach (smc, page);      	/*  Attach to the page.  */
	        switch (page->type) {       	/*  Process the data.  */
	        case TY_VOID:     processData (page); break;
	        case TY_DATA:     processData (page); break;
	        case TY_META: processMetaData (page); break;
	             default: 			      break;
	        }
	        smcDetach (smc, page, TRUE);    /*  Detach from the page.  */
	    }

        } else if (!smcopStdFlag (argv[i])) 	/* eat the flag  */
            printf ("Skipping unrecognized option: %s\n", argv[i]);
    }


    /* Now simply close the cache, the 'clean' flag will indicate whether 
     * we mark everything for deletion.
     */
    if (smc)
	smcClose (smc, clean);
}


Usage ()
{
  printf("\n");
  printf("   Usage:\n");
  printf("\tsmcop [-cache <file>] [-debug] [-verbose] [-help] <cmd_flags>");
  printf("\n\n   where <cmd_flags> is and combination of:\n\n");
  printf("    -clean\t\tcleanup orphaned pages\n");
  printf("    -create\t\tcreate an SMC\n");
  printf("    -destroy\t\tdestroy the SMC\n");
  printf("    -init\t\tinitialize the SMC\n");
  printf("    -list\t\tlist managed pages\n");
  printf("    -dir <path>\t\tset cwd\n");
  printf("    -root <str>\t\tset file root\n");
  printf("    -seqno <num>\tset file sequence num\n");
  printf("    \n");
  printf("    -pan\t\tsimulate PAN readout\n");
  printf("    -dca\t\tsimulate DCA readout\n\n");
  printf("   Command abbreviations are supported. The task will process \n");
  printf("   these in order, so for example\n");
  printf("    \n");
  printf("   \tsmcop -pan -list -init -list\n");
  printf("    \n");
  printf("   will simulate a readout from the PAN (i.e. populate the SMC),\n");
  printf("   list the managed pages, initialize the SMC and then list what\n");
  printf("   remains (which should simply be the SMC segment itself).\n");
  printf("\n");

  exit (0);
}


smcopStdFlag (char *opt) 
{
   if (strncmp (opt, "-cache", 3) == 0 || 	/* eat the flag  */
       strncmp (opt, "-all", 2) == 0 ||
       strncmp (opt, "-debug", 2) == 0 ||
       strncmp (opt, "-verbose", 2) == 0 ||
       strncmp (opt, "-help", 2) == 0 ||
       strncmp (opt, "--help", 3) == 0)
	    return TRUE;
    else
       return FALSE;
}


/****************************************************************************
 * SIMMETADATA -- Simulate a MetaData transfer to the SMC.
 ****************************************************************************/

simMetaData ()
{
    fpConfig_t fp;
    mdConfig_t md;
    int i, nkeyw = 256;


    /* Get a Page for the metadata.  */
    page = smcGetPage (smc, TY_META, (80*nkeyw), TRUE, TRUE);
    if (page == (smcPage_t *) NULL) {
	fprintf (stderr, "Error getting metadata page.\n");
	return;
    }

    md.metaType = 1;			/* Set the mdConfig struct.	*/
    md.numFields = nkeyw;
    smcSetMDConfig (page, &md);

    fp.xSize = im_nx;			/* Set the fpConfig struct.	*/
    fp.ySize = im_ny;
    fp.xStart = 0;
    fp.yStart = 0;
    fp.dataType = 1;
    smcSetFPConfig (page, &fp);

    smcSetWho (page, 2);		/* Set the 'who' field		*/
    smcSetExpID (page, 3.141592654);	/* Set the 'expid' field	*/
    smcSetObsetID (page, "3.14159");	/* Set the 'obsetid' field	*/

    /* Create a dummy header in the shared area 
     */
    make_header ((char *)smcGetPageData(page), nkeyw);

    /* Unlock and Detach from the page. 
     */
    smcUnlock (page);
    smcDetach (smc, page, FALSE);
}


/****************************************************************************
 * SIMDATA -- Simulate a Data transfer to the SMC.
 ****************************************************************************/

simData ()
{
    fpConfig_t fp;

    /* Get a Page for the pixel data.  */
    page = smcGetPage (smc, TY_DATA, (im_nx*im_ny*(im_bitpix/8)), TRUE, TRUE);
    if (page == (smcPage_t *) NULL) {
	fprintf (stderr, "Error getting data page.\n");
	return;
    }

    fp.xSize = im_nx;			/* Set the fpConfig struct.	*/
    fp.ySize = im_ny;
    fp.xStart = 0;
    fp.yStart = 0;
    fp.dataType = 1;
    smcSetFPConfig (page, &fp);

    smcSetWho (page, 2);		/* Set the 'who' field		*/
    smcSetExpID (page, 3.141592654);	/* Set the 'expid' field	*/
    smcSetObsetID (page, "3.14159");	/* Set the 'obsetid' field	*/

    /* Create a raster in the shared area 
     */
    make_raster ((int *)smcGetPageData(page), im_nx, im_ny, color_mode++);

    /* Unlock and Detach from the page. 
     */
    smcUnlock (page);
    smcDetach (smc, page, FALSE);
}


/****************************************************************************
 *  MAKE_RASTER -- Create a ramped array of pixel.
 ****************************************************************************/

make_raster (raster, nx, ny, color)
int     *raster;
int     nx, ny, color;
{
        register unsigned int pix;
        register int i, j;

        if (color == 0) {
            /* Build a solid color */
            for (i = 0; i < nx; i++) {
                for (j = 0; j < ny; j++) {
                    raster[i * nx + j] = (unsigned char) color;
                }
            } 
        } else {
            /* Make a test changing pattern. */
            for (i = 0; i < ny; i++) {
               for (j = 0; j < nx; j++) {
		  switch ((color % 3)) {
		  case 0: 				/* Diagonal ramp     */
                    raster[i * nx + j] = i + j;
		    break;
		  case 1: 				/* Horizontal ramp   */
                    raster[i * nx + j] = j;
		    break;
		  case 2: 				/* Vertical ramp     */
                    raster[i * nx + j] = i;
		    break;
                  }
               }
            } 
        }
}


/****************************************************************************
 * MAKE_HEADER -- Create an artificial header.
 ****************************************************************************/

make_header (hdr, nkeys)
char	*hdr;
int	nkeys;
{
	int i;
	char *op, buf[80];

	op = hdr;
	for (i=1; i <= nkeys; i++) {
	    memset (buf, ' ', 80);
	    sprintf (buf, "FOO_%03d = 'value_%04d'", i, i);
	    strncpy (&op[40], "/ comment string", 16);
	    strncpy (op, buf, 80);
	    op += 80; 
	}
}


/****************************************************************************
 * PROCESSDATA -- Read a data (or void) segment.
 ****************************************************************************/

processData (smcPage_t *p)
{
    int   i, data_size, *idata;
    smcSegment_t *seg = (smcSegment_t *) NULL;
    fpConfig_t *fp = smcGetFPConfig (p);


    stdHdrOutput (p);			  /* print standard header	*/

    seg = SEG_ADDR(p);
    data_size = seg->dsize;

    idata = (int *) smcGetPageData (p);

    printf ("    Data Statistics:\n\t"); 
    {	double sum=0.0, sum2=0.0;
	double mean=0.0, sigma=0.0, min=0.0, max=0.0;
	int npix = (fp->xSize * fp->ySize);

	for (i=0, sum=0.0; i < npix; i++) {
	    sum += idata[i];
	    min = (idata[i] < min ? idata[i] : min);
	    max = (idata[i] > max ? idata[i] : max);
	}
	mean = (sum / (double)npix);

	for (i=0, sum=0.0; i < npix; i++)
	    sum += ( (idata[i] - mean) * (idata[i] - mean) );
	sigma = sqrt ( (sum / (double)npix) );

	printf ("Mean: %.4f  StdDev: %.4f  Min: %.1f  Max: %.1f\n", 
	    mean, sigma, min, max);
    }
}


/****************************************************************************
 * PROCESSMETADATA -- Read a header segment.
 ****************************************************************************/

processMetaData (smcPage_t *p)
{
    int   i, data_size;
    char *cdata, *ip, *edata;
    smcSegment_t *seg = (smcSegment_t *) NULL;
    mdConfig_t *md = smcGetMDConfig (p);


    stdHdrOutput (p);			  /* print standard header	*/

    if (!verbose) {
        printf ("\tNum Keywords = %d:\n", md->numFields);
	return;
    }

    seg = SEG_ADDR(p);
    data_size = seg->dsize;
    cdata = ip = (char *) smcGetPageData (p);
    edata = cdata + data_size;

    ip = cdata;				  /* initialize			*/
    edata = (cdata + data_size);	  /* find addr of end of data	*/
    printf ("    MetaData Fields:\n"); 
    for (i=0; i < 3 && ip < edata; i++) { /* print first 3 keywords	*/
        printf ("\t%-70.70s\n", ip); 
	ip += 80;
    }
    if (ip < edata)			  /* print '...' if there's more*/
        printf ("\t....."); 
    printf ("\n"); 
}


stdHdrOutput (smcPage_t *p)
{
    mdConfig_t *md = smcGetMDConfig (p);
    fpConfig_t *fp = smcGetFPConfig (p);
    smcSegment_t *seg = (smcSegment_t *) NULL;


    seg = SEG_ADDR(p);
    printf ("  LockFile: %s  size = %-10d  dsize = %d\n", 
	p->lockFile, p->size, seg->dsize);
    printf ("  Dir:  '%s'  Root: '%s'  SeqNum: %d\n",
        smcGetPageDir (p), smcGetPageFRoot(p), smcGetPageSeqNo(p));

    if (verbose) {
        printf ("  MD Config:  metaType = %d\tnumFields = %d:\n", 
	    md->metaType, md->numFields);
        printf ("  FP Config:  %d x %d  at (%d,%d) type = %d\n", 
	    fp->xSize, fp->ySize, fp->xStart, fp->yStart, fp->dataType);
    }
}






/****************************************************************************
 * SMCOPLISTPAGES -- Print a summary list of pages.
 ****************************************************************************/

smcopListPages (smCache_t *smc, int npages)
{
    smcPage_t *p = (smcPage_t *) NULL;
    smcSegment_t *s = (smcSegment_t *) NULL;
    int   i, np = ((npages < 0) ? max(smc->top+2,smc->npages) : npages);
    char  ptype[12], *ctime;

    int maxid, shmid, id;
    struct shmid_ds shmseg;
    struct shm_info shm_info;
    struct shminfo shminfo;
    struct ipc_perm *ipcp = &shmseg.shm_perm;


    maxid = shmctl (0, SHM_INFO, (struct shmid_ds *) &shm_info);

    printf ("\t\t------ Shared Memory Segments --------\n\n");

    printf ("    Pages allocated:  %ld  resident:  %ld  swapped:  %ld\n", 
	shm_info.shm_tot, shm_info.shm_rss, shm_info.shm_swp);
    printf ("         Cache File:  %s\n\n", smc->sysConfig.cache_path);

    printf ("    memKey      shmId    size       ");
    printf ("type     finalr  nattch  status   ctime\n");
    printf ("    ------      -----    ----       ");
    printf ("----     ------  ------  ------   -----\n");


    /*  Print some info on the cache segment itself.
     */
    printf("    0x%08x  %-8d %-10d SMCache  ",
        smc->memKey, smc->shmid, smc->cache_size);
    printf("00%d0%d0  ", smc->nattached, smc->vm_locked);

    shmid = shmctl (smUtilKey2ID(smc->memKey), IPC_STAT, &shmseg);
    printf("%-6ld %s %s ", 
	(unsigned long) shmseg.shm_nattch,
	ipcp->mode & SHM_DEST ? "dest" : "    ",
	ipcp->mode & SHM_LOCKED ? "lock" : "    ");

    ctime = smUtilTimeStr ((double)smc->ctime);
    printf("%8.8s\n", &ctime[11]);


    /*  Now print each of the managed segments.
     */
    for (i=0; i < np; i++) {
      p = &smc->pdata[i];
      s = SEG_PTR(p);

      if (p->memKey) {
	extern  char *smcType2Str (int type);


	id = smUtilKey2ID(p->memKey);
 	shmid = shmctl (id, IPC_STAT, &shmseg);

	/* Make a readable string of the segment type.
	*/
	strcpy ((char *)&ptype[0], (char *)smcType2Str(p->type));
	bcopy (&ptype[3], &ptype, 7);

        printf("%3d 0x%08x  %-8d %-10d %-7s  ", 
	    i, p->memKey, id, p->size, ptype);
        printf("%d%d%d%d%d%d  ",
            p->free, p->initialized, p->nattached, 
            SEG_ATTACHED(p), p->ac_locked, p->nreaders);


        printf("%-6ld %s %s ", 
	    (unsigned long) shmseg.shm_nattch,
	    ipcp->mode & SHM_DEST ? "dest" : "    ",
            ipcp->mode & SHM_LOCKED ? "lock" : "    ");

	ctime = smUtilTimeStr ((double)shmseg.shm_ctime);
        printf("%8.8s\n", &ctime[11]);
      }
    }
    printf ("\n");
}

