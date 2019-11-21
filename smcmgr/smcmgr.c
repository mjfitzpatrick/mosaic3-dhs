#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stddef.h>

#include "smCache.h"
#include "mbus.h"
#include "cdl.h"
#include "smcmgr.h"

/*****************************************************************************
**  SMCMGR -- Utility task for commandline management and operations of
**  the Shared Memory Cache.  This allows us to script SMC operations by
**  chaining together any number of command flags.  The task will process
**  these in order, so for example
**
** 	smcmgr -pan -list -init -list
**
**  will simulate a readout from the PAN (i.e. populate the SMC), list
**  the managed pages, initialize the SMC and then list what remains (which
**  should simply be the SMC segment itself).
**
**  Usage:
**
**    smcmgr [-cache <file>] [-mon] [-debug] [-verbose] [-help] <cmd_flags>
**
**  where <cmd_flags> is and combination of:
** 
**     -clean              cleanup orphaned pages
**     -create             create an SMC
**     -destroy            destroy the SMC
**     -init               initialize the SMC
**     -list               list managed pages
**     -pan                simulate PAN readout
**     -dca                simulate DCA readout
** 
**  Command abbreviations are supported.
** 
**	An alternative mode is triggered with the '-mon' flag where the 
**  task will connect to the message but and act as a monitor for the 
**  DHS Supervisor.  In this mode the SMCMGR also acts as a Real-Time Display
**  (RTD) client application since we have direct access to the pixel memory
**  pages.  Metadata headers and statistics for pixel arrays can also be
**  computed and sent to the Supervisor for display.  Minimal command support
**  is available via messages from the Supervisor itself.
** 
*****************************************************************************/


                                        /* Function prototypes          */
extern smCache_t *smcOpen();
extern smcPage_t *smcGetPage();
                                        /* I/O Handlers                 */
int   mbusMsgHandler (int fd, void *client_data);



smCache_t *smc    = (smCache_t *)NULL;
smcPage_t *page   = (smcPage_t *)NULL;
smcSegment_t *seg = (smcSegment_t *)NULL;

double	expID	  = 3.141592654;	/* Exposure ID variable		*/

CDLPtr cdl        = (CDLPtr) NULL;	/* RTD variables		*/

int    fbconfig   = 41,			/* Frame buffer			*/
       fb_width   = 8800,
       fb_height  = 8800;
int    disp_stdimg= 41;			/* display frame		*/

int    disp_frame = 1;			/* display frame		*/
int    trim_ref   = FALSE;		/* trim reference pixels	*/
char  *imtdev     = NULL;		/* Display server device	*/
int    seqno 	  = 0;			/* sequence number		*/

int  imnum	  = 0, 			/* for artificial image data	*/
     color_mode	  = -1,			/* diagonal ramp w/ ref pixels	*/
     im_nx	  = 2148, 
     im_ny	  = 4096, 
     im_bitpix	  = 32,
     nkeyw        = 128;

int  use_mbus	  = FALSE;		/* enable monitor mode		*/
int  use_disp	  = FALSE;		/* enable RTD mode		*/
int  use_threads  = FALSE ;		/* parallelize display		*/

int  console	  = FALSE;		/* enable console output	*/
int  verbose	  = FALSE;		/* pipeline options		*/
int  init	  = FALSE;		/*     "      "			*/
int  debug	  = FALSE;		/*     "      "			*/
int  clean	  = FALSE;		/*     "      "			*/
int  list	  = FALSE;		/*     "      "			*/
int  all	  = FALSE;		/*     "      "			*/
int  seq	  = FALSE;		/*     "      "			*/
int  lo_gain      = FALSE;		/*     "      "			*/

int  otf_enable   = TRUE;		/*     "      "			*/
int  disp_enable  = TRUE;		/*     "      "			*/
int  stat_enable  = TRUE;		/*     "      "			*/
int  rotate_enable= TRUE;		/*     "      "			*/
int  idListing    = TRUE;		/*     "      "			*/
int  use_pxf      = TRUE;		/*     "      "			*/

char fname[SZ_FNAME];			/* SMC lock file name		*/
char config[SZ_LINE];			/* SMC configuration options	*/

char *procPages	   = (char *)NULL;

char *imtypeKeyw   = "NOCTYP";		/* For getting the image type	*/
char *imtypeDB     = "NOCS_Pre";


int
main (int argc, char **argv) 
{
    register int  i;



    /* Process command-line arguments.  Make a first pass looking only for
     * flags particular to the cache, e.g. the lockfile name or mbus mode.
     */
    bzero (fname, SZ_FNAME);
    for (i=1; i < argc; i++) {
        if (strncmp (argv[i], "-cache", 3) == 0) {
            strcpy (fname, argv[++i]); 			/* set cache file  */

        } else if (strncmp (argv[i], "-mbus", 2) == 0) {
            use_mbus=1;					/* monitor mode    */
        } else if (strncmp (argv[i], "-nombus", 5) == 0) {
            use_mbus=0;					/* monitor mode    */

        } else if (strncmp (argv[i], "-mon", 2) == 0) {
            use_mbus=1;					/* monitor mode    */
        } else if (strncmp (argv[i], "-nomon", 5) == 0) {
            use_mbus=0;					/* monitor mode    */

        } else if (strncmp(argv[i], "-host", 3) == 0) {
            mbInitMBHost();
            if ((argc - i) > 1 && (argv[i+1][0] != '-'))
                mbSetSimHost (argv[++i], 1);
            else {
                fprintf (stderr, "Error: '-host' requires an argument\n");
                exit (1);
            }

        } else if (strncmp (argv[i], "-dev", 5) == 0) {
            imtdev = argv[++i];				/* display server  */
	    use_disp++;

        } else if (strncmp (argv[i], "-disp", 5) == 0) {
	    use_disp++;

        } else if (strncmp (argv[i], "-no_disp", 5) == 0) {
	    use_disp = 0;

        } else if (strncmp (argv[i], "-trim", 5) == 0) {
	    trim_ref++;

        } else if (strncmp (argv[i], "-no_stat", 5) == 0) {
	    stat_enable = 0;

        } else if (strncmp (argv[i], "-no_pxf", 5) == 0) {
	    use_pxf = 0;

        } else if (strncmp (argv[i], "-raw", 5) == 0) {
	    rotate_enable = 0;

        } else if (strncmp (argv[i], "-proc", 5) == 0) {
	    procPages = argv[++i];

        } else if (strncmp (argv[i], "-frame", 5) == 0) {
            disp_frame = atoi (argv[++i]);	    /* display frame   	*/

        } else if (strncmp (argv[i], "-fbconfig", 5) == 0) {
            fbconfig = atoi (argv[++i]);	    /* display buffer  	*/

        } else if (strncmp (argv[i], "-console", 5) == 0) {
            console++;				    /* console output  	*/

        } else if (strncmp (argv[i], "-all", 2) == 0) {
            all++;				    /* list-all flag   	*/

        } else if (strncmp (argv[i], "-debug", 4) == 0) {
            debug++;				    /* debug flag      	*/

        } else if (strncmp (argv[i], "-verbose", 2) == 0) {
            verbose++;				    /* verbose  flag   	*/

        } else if ((strncmp (argv[i], "-help", 2) == 0))
            smUsage ();
    }

    if (fname[0] == (char) NULL)
        sprintf (config, "debug=%d", debug);
    else
        sprintf (config, "debug=%d,cache_file=%s", debug, fname);


    /* Initialize the config string and Open/Attach to the cache.
     */
    if (verbose)
	fprintf (stderr, "Opening cache....%s\n", config);
    if ((smc = smcOpen (config)) == (smCache_t *)NULL) {
        fprintf (stderr, "Error opening cache, invalid file?.\n");
	exit (1);
    }


    /*  If we're acting as a Supervisor monitor or explicitly displaying an
    **  image, open a connection to the display server.
    if (use_mbus || use_disp) {
	if (imtdev == (char *) NULL)
            imtdev = (char *) getenv ("IMTDEV");
        if (! (cdl = cdl_open (imtdev)) ) {
            fprintf (stderr, "ERROR: cannot connect to display server.");
	    use_disp = disp_enable = 0;
        }
    }
    */


    /* Begin processing either as a monitor for the DHS or as a commandline
    ** tool.
    */
    if (use_mbus) 
	smMonitor ();
    else
	smPipeline (argc, argv);


    /* Now simply close the cache, the 'clean' flag will indicate whether 
     * we mark everything for deletion.
     */
    if (smc)
	smcClose (smc, clean);

/*
    if (cdl)
	cdl_close (cdl);

*/

    return (0);
}


/*************************************************************************
**  SMMONITOR -- Operate as a message bus monitor application.
*/
void
smMonitor ()
{
    int  mb_tid, mb_fd;


    /* Open a connection on the message bus.
    */
    if ((mb_tid = mbusConnect("SMCMgr", "SMCMgr", FALSE)) <= 0) {
        fprintf (stderr, "ERROR: Can't connect to message bus.\n");
        exit (1);
    }

    if ((mb_fd = mbGetMBusFD()) >= 0) {     /* Add the input handlers. */
        mbusAddInputHandler (mb_fd, mbusMsgHandler, NULL);

    } else {
        fprintf (stderr, "ERROR: Can't install MBUS input handler.\n");
        exit (1);
    }

    /*  Send initial status to the Supervisor for display.
    */
    if (console)
	fprintf (stderr, "Waiting for input....\n");
    mbusSend (SUPERVISOR, ANY, MB_STATUS, "Waiting for data...");

    /*  Begin processing I/O events.  Note: This never returns....
    */
    mbusSend (SUPERVISOR, ANY, MB_READY, "READY SMCMGR");
    mbusAppMainLoop (NULL);
}


/**************************************************************************
**  SMPIPELINE -- Operate as a commandline pipeline task.
*/
void
smPipeline (int argc, char **argv)
{
    register int  i, j, xs, ys;



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

        } else if (strncmp (argv[i], "-list", 4) == 0) {
            /* List the current SMC segments.
	     */
	    if (verbose) 
		smcPrintCacheInfo (smc, (char *)NULL);
	    smListSegments (smc, smc->top);

        } else if (strncmp (argv[i], "-lpages", 4) == 0) {
            /* List the current SMC pages.
	     */
	    if (verbose) 
		smcPrintCacheInfo (smc, (char *)NULL);
	    smListPages (smc, smc->top);


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
	        smListSegments (smc, smc->top);
		system ("date");

		sleep (interval);
	    }

        } else if (strncmp (argv[i], "-expid", 3) == 0) {
	    expID = atof (argv[++i]);

        } else if (strncmp (argv[i], "-nocs", 3) == 0) {
	    smcIncrSeqNo (smc);
            simMetaData (nkeyw=199,im_nx=4160,im_ny=4160);
            simMetaData (nkeyw=199,im_nx=4160,im_ny=4160);

        } else if (strncmp (argv[i], "-pan", 4) == 0) {
            /* Generate an artificial readout sequence of the entire
	    ** focal plan and detector system.
	    */
            if (verbose)
                printf ("Generating artificial readout %d (%d/%d) ....", 
                        imnum++, smc->npages, smc->top);
	    smcIncrSeqNo (smc);
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);	/* Pan A */
                simData (im_nx=im_nx,im_ny=im_ny,xs=0,   ys=0);
                simData (im_nx=im_nx,im_ny=im_ny,xs=4159,ys=0);
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);	/* Pan B */
                simData (im_nx=im_nx,im_ny=im_ny,xs=4159,ys=4159);
                simData (im_nx=im_nx,im_ny=im_ny,xs=0   ,ys=4159);
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);
            if (verbose)
                printf ("done.\n");
	    sleep (1);

        } else if (strncmp (argv[i], "-panA", 5) == 0) {
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);	/* Pan A */
                simData (im_nx=im_nx,im_ny=im_ny,xs=0,   ys=0);
                simData (im_nx=im_nx,im_ny=im_ny,xs=4159,ys=0);
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);
	    sleep (1);

        } else if (strncmp (argv[i], "-panB", 5) == 0) {
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);	/* Pan B */
                simData (im_nx=im_nx,im_ny=im_ny,xs=4159,ys=4159);
                simData (im_nx=im_nx,im_ny=im_ny,xs=0   ,ys=4159);
            simMetaData (nkeyw=128,im_nx=im_nx,im_ny=im_ny);
	    sleep (1);

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

	    for (j=0; j < 4 && (page = smcNextPage (smc, 0)); j++) {
	        smcAttach (smc, page);      	/*  Attach to the page.  */
	        switch (page->type) {       	/*  Process the data.  */
	        case TY_VOID:     processData (page); break;
	        case TY_DATA:     processData (page); break;
	        case TY_META: processMetaData (page); break;
	             default: 			      break;
	        }
		smcMutexOn ();
	        smcDetach (smc, page, TRUE);    /*  Detach from the page.  */
		smcMutexOff ();
	    }

        } else if (strncmp (argv[i], "-disp", 5) == 0) {
	    double id = -1.0;

	    if (i < argc && argv[i+1][0] != '-') /* look for expid arg */
		id = atof (argv[++i]);

	    /* Display the SMC data pages.
	    */
	    if (imtdev == (char *) NULL)
                imtdev = (char *) getenv ("IMTDEV");
            if ((cdl = cdl_open (imtdev)) ) {
	        cdl_setFBConfig (cdl, 41);  	/* imt8800		*/
	        cdl_setFrame (cdl, disp_frame);  
	        cdl_clearFrame (cdl);  
	        if (use_mbus) 
	            mbusSend (SUPERVISOR, ANY, MB_SET, "rtdStat clear");

	        while ((page = smcNextByExpID (smc, id))) {
	            smcAttach (smc, page);
		    if (page->type == TY_DATA || page->type == TY_VOID) {
		        smcRectifyPage (page);
		        rtdDisplayPixels ((void *)cdl, page);
		    }
		    smcMutexOn ();
	            smcDetach (smc, page, TRUE);
		    smcMutexOff ();
	        }
	        cdl_close (cdl);
            }

        } else if (strncmp (argv[i], "-stat", 5) == 0) {
	    /* Display the SMC data page statistics
	    */

        }
    }
}


/*************************************************************************
*/
void
smUsage ()
{
  printf("\n");
  printf("   Usage:\n");
  printf("\tsmcmgr [-cache <file>] [-debug] [-verbose] [-help] <cmd_flags>");
  printf("\n\n   where <cmd_flags> is and combination of:\n\n");
  printf("    -mon\t\tact as message bus monitor\n");
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
  printf("   \tsmcmgr -pan -list -init -list\n");
  printf("    \n");
  printf("   will simulate a readout from the PAN (i.e. populate the SMC),\n");
  printf("   list the managed pages, initialize the SMC and then list what\n");
  printf("   remains (which should simply be the SMC segment itself).\n");
  printf("\n");

  exit (0);
}


int
smStdFlag (char *opt) 
{
   if (strncmp (opt, "-cache", 3) == 0 || 	/* eat the flag  */
       strncmp (opt, "-all", 2) == 0 ||
       strncmp (opt, "-mon", 2) == 0 ||
       strncmp (opt, "-dev", 2) == 0 ||
       strncmp (opt, "-frame", 2) == 0 ||
       strncmp (opt, "-fbconfig", 2) == 0 ||
       strncmp (opt, "-debug", 2) == 0 ||
       strncmp (opt, "-verbose", 2) == 0 ||
       strncmp (opt, "-mon", 2) == 0 ||
       strncmp (opt, "-help", 2) == 0 ||
       strncmp (opt, "--help", 3) == 0)
	    return TRUE;
    else
       return FALSE;
}


/****************************************************************************
 * SIMMETADATA -- Simulate a MetaData transfer to the SMC.
 ****************************************************************************/
void
simMetaData (int nkeyw, int nx, int ny)
{
    fpConfig_t fp;
    mdConfig_t md;


    /* Get a Page for the metadata.  */
    page = smcGetPage (smc, TY_META, (128*(nkeyw+36)), TRUE, TRUE);
    if (page == (smcPage_t *) NULL) {
	fprintf (stderr, "Error getting metadata page.\n");
	return;
    }

    md.metaType = 2;			/* Set the mdConfig struct.	*/
    md.numFields = nkeyw;
    smcSetMDConfig (page, &md);

    fp.xSize    = nx;			/* Set the fpConfig struct.	*/
    fp.ySize    = ny;
    fp.xStart   = 0;
    fp.yStart   = 0;
    fp.dataType = 1;
    smcSetFPConfig (page, &fp);

    smcSetWho (page, 2);		/* Set the 'who' field		*/
    smcSetExpID (page, expID);		/* Set the 'expid' field	*/
    smcSetObsetID (page, "3.14159");	/* Set the 'obsetid' field	*/

    /* Create a dummy header in the shared area  -- use AVP format
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
void
simData (int nx, int ny, int xs, int y2)
{
    fpConfig_t fp;


    /* Get a Page for the pixel data.  */
    page = smcGetPage (smc, TY_DATA, (nx*ny*(im_bitpix/8)), TRUE, TRUE);
    if (page == (smcPage_t *) NULL) {
	fprintf (stderr, "Error getting data page.\n");
	return;
    }

    fp.xSize    = nx;			/* Set the fpConfig struct.	*/
    fp.ySize    = ny;
    fp.xStart   = xs;
    fp.yStart   = y2;
    fp.dataType = 1;
    smcSetFPConfig (page, &fp);

    smcSetWho (page, 2);		/* Set the 'who' field		*/
    smcSetExpID (page, expID);		/* Set the 'expid' field	*/
    smcSetObsetID (page, "3.14159");	/* Set the 'obsetid' field	*/

    /* Create a raster in the shared area 
     */
    make_raster ((int *)smcGetPageData(page), nx, ny, color_mode);

    /* Unlock and Detach from the page. 
     */
    smcUnlock (page);
    smcDetach (smc, page, FALSE);
}


/****************************************************************************
 *  MAKE_RASTER -- Create a ramped array of pixel.
 ****************************************************************************/
void
make_raster (int *raster, int nx, int ny, int color)
{
        register int i, j;


        if (color < 0) {
            for (i = 0; i < ny; i++) {
               for (j = 0; j < nx; j++) {
		  if (j >= (nx-64))
                      raster[i * nx + j] = 4096;
		  else
                      raster[i * nx + j] = i + j;
               }
            }

        } else if (color == 0) {
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
void
make_header (char *hdr, int nkeys)
{
	int i;
	char *op, keyw[32], val[32], comment[64], buf[128];

	op = hdr;
	for (i=1; i <= nkeys; i++) {
	    memset (buf, ' ', 128);
	    memset (keyw, ' ', 32);
	    memset (val, ' ', 32);
	    memset (comment, ' ', 64);

	    sprintf (keyw, "FOO_%04d", i);
	    sprintf (val, "value_%04d", i);
	    sprintf (comment, "comment string");

	    memmove (op, keyw, 32);
	    memmove (&op[32], val, 32);
	    memmove (&op[64], comment, 64);
	    op += 128; 
	}
}


void
make_fits_header (char *hdr, int nkeys)
{
	int i;
	char *op, buf[80];

	op = hdr;
	for (i=1; i <= nkeys; i++) {
	    memset (buf, ' ', 80);
	    sprintf (buf, "FOO_%04d= 'value_%04d'        / comment string", i, i);
	    memmove (op, buf, 80);
	    op += 80; 
	}
}


/****************************************************************************
 * PROCESSDATA -- Read a data (or void) segment.
 ****************************************************************************/
void
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

    {	double sum=0.0, mean=0.0, sigma=0.0, min=0.0, max=0.0;
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
void
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


void
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


/* SMLISTSEGMENTS -- Print a summary list of managed segments.
*/
void
smListSegments (smCache_t *smc, int nsegs)
{
    smcPage_t *p = (smcPage_t *) NULL;
    smcSegment_t *s = (smcSegment_t *) NULL;
    int   i, np = ((nsegs < 0) ? max(smc->top+2,smc->npages) : nsegs);
    char  ptype[12], *ctime;

    int    maxid, shmid, id;
    struct shmid_ds shmseg;
    struct shm_info shm_info;
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
    printf("    0x%08x  %-8ld %-10d SMCache  ",
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
	    id = smUtilKey2ID(p->memKey);
 	    shmid = shmctl (id, IPC_STAT, &shmseg);

	    /* Make a readable string of the segment type. */
	    strcpy ((char *)&ptype, (char *)smcType2Str(p->type));
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


/* SMLISTPAGES -- Print a summary list of managed pages.
*/
void
smListPages (smCache_t *smc, int npages)
{
    smcPage_t *page = (smcPage_t *) NULL;
    smcSegment_t *seg = (smcSegment_t *) NULL;
    char   ptype[12];
    int    i, maxid, shmid, id;
    struct shmid_ds shmseg;
    struct shm_info shm_info;
    /*struct ipc_perm *ipcp = &shmseg.shm_perm; */


    maxid = shmctl (0, SHM_INFO, (struct shmid_ds *) &shm_info);

    printf ("\t\t------ Shared Memory Pages --------\n\n");

    printf ("    Pages allocated:  %ld  resident:  %ld  swapped:  %ld\n", 
	shm_info.shm_tot, shm_info.shm_rss, shm_info.shm_swp);
    printf ("         Cache File:  %s\n\n", smc->sysConfig.cache_path);

    printf ("    memKey      shmId    size       type     ExpID\n");
    printf ("    ------      -----    ----       ----     -----\n\n");



    /*  Now print each of the managed segments.
     */
    i =0;
    while ((page = smcNextPage (smc, 0))) {
	smcAttach (smc, page);

        seg = SEG_PTR(page);

        if (page->memKey) {
	    id = smUtilKey2ID (page->memKey);
 	    shmid = shmctl (id, IPC_STAT, &shmseg);

	    /* Make a readable string of the segment type. */
	    strcpy ((char *)&ptype, (char *)smcType2Str(page->type));
	    bcopy (&ptype[3], &ptype, 7);

            printf ("%3d 0x%08x  %-8d %-10d %-7s  %.6lf\n", 
	        i, page->memKey, id, page->size, ptype, smcGetExpID(page));

        }
	i++;
	smcDetach (smc, page, FALSE);
    }
    printf ("\n\n");
}
