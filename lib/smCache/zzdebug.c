#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stddef.h>

#include "smCache.h"

extern smCache_t *smcOpen();
extern smcPage_t *smcGetPage();

int	im_nx = 100;
int	im_ny = 100;
int	im_bitpix = 32;
int	color_mode = 1;

smCache_t *smc  = (smCache_t *)NULL;
smcPage_t *page = (smcPage_t *)NULL;
smcSegment_t *seg = (smcSegment_t *)NULL;


main (int argc, char **argv) 
{
    char  c, fname[64], buf[64], resp[32], val[12], config[128];
    int   attach=FALSE, lock=FALSE, ival, nvals, segtyp;
    int	  i, j, stat=0, nsegs=0, pagenum=0, segnum=0, segsz=0, debug=0;
    void  *data;
    int   *idata;
    char  *cdata;


    ZZINIT (argc, argv);


    /* Command loop */
    printf ("zzdebug (%d)>  ", getpid());
    while ((c = getchar())) {

	switch (c) {
	case 'a':			/*   attach to segment		*/
	    smcListPages (smc, -1);
	    printf ("which page? ");  scanf ("%d", &pagenum);
	    page = &smc->pdata[pagenum];
	    smcAttach (smc, page);
	    if (!SEG_ATTACHED(page))
		printf ("\nERR: Page is not attached to this process.\n");
	    break;

	case 'c':			/*   cleanup (prune) cache	*/
	    if (smc) {
	        printf ("Pruning cache....\n");
    	        smcPrune (smc);
    	        if (debug) 
		    smcPrintCacheInfo (smc, "After Pruning:");
	    } else
		printf ("Null cache ptr, try opening a cache first.\n");

	    break;

	case 'd':			/*   detach from segment	*/
	    smcListPages (smc, -1);
	    printf ("which page? ");  scanf ("%d", &pagenum);
	    page = &smc->pdata[pagenum];
	    if (page->ac_locked)
		printf ("Page is locked for access.\n");
	    else if (!SEG_ATTACHED(page))
		printf ("\nPage is not attached to this process.\n");
	    else
	        smcDetach (smc, page, FALSE);
	    break;

	case 'f':			/*   fill segment with data	*/
	    smcListPages (smc, -1);
	    printf ("which page? ");  scanf ("%d", &pagenum);
	    page = &smc->pdata[pagenum];
	    if (page->ac_locked)
		printf ("Page is locked for access.\n");
	    else if (!SEG_ATTACHED(page))
		printf ("Page is not attached to this process.\n");
	    else {
	        printf ("data value? ");  scanf ("%d", &ival);
		seg = SEG_ADDR(page);
		if (page->type == TY_META) {
		    cdata = (char *) smcGetPageData (page);
		    memset (cdata, ival, seg->dsize);
		} else {
		    idata = (int *) smcGetPageData (page);
		    for (i=0; i < seg->dsize; i++)
		       idata[i] = ival;
		}
	    }
	    break;

	case 'g':			/*   print (get) segment data	*/
	    smcListPages (smc, -1);
	    printf ("which page? ");  scanf ("%d", &pagenum);
	    page = &smc->pdata[pagenum];
	    if (page->ac_locked)
		printf ("Page is locked for access.\n");
	    else if (!SEG_ATTACHED(page))
		printf ("Page is not attached to this process.\n");
	    else {
	        printf ("how many values? ");  scanf ("%d", &nvals);
		if (page->type == TY_META) {
		    cdata = (char *)smcGetPageData (page);
		    printf ("%.80s", cdata);
		} else {
		    idata = (int *)smcGetPageData (page);
		    for (i=0; i < nvals; i++)
		        printf ("%2d(%d) %s", i,idata[i],(i&&(i%4==0))?"\n":"");
		}
		printf ("\n");
	    }
	    break;


	case 'l':			/*   list segments		*/
	    nsegs = optargi ();
	    if (nsegs < 0 && smc->npages == 0) {
		printf ("No pages yet allocated, showing first 8.\n");
	        smcListPages (smc, 8);
	    } else
	        smcListPages (smc, nsegs);
	    break;

	case 'n':			/*   new segment 		*/
	    printf ("nsegs? "); scanf ("%d", &nsegs);
	    printf ("seg_type (1=void,2=data,3=meta): "); scanf ("%d", &segtyp);
	    attach = TRUE;
	    lock = FALSE;
	    for (i=1; i <= nsegs; i++)  {
/*
                page = smcGetPage (smc, segtyp, (i*1024), attach, lock);
*/
		if (segtyp == TY_META)
		    simMetaData ();
		else
		    simData ();
	    }
	    break;

	case 'p':			/*   print segment info 	*/
	    if (smc) {
	        smcListPages (smc, -1);
	        printf ("which page? ");  scanf ("%d", &pagenum);
	        page = &smc->pdata[pagenum];
	        if (page)
		    smcPrintPageInfo (page, pagenum);
	        else
		    printf ("Null page ptr, try again.\n");
	    } else
		printf ("Null cache ptr, try opening a cache first.\n");
	    break;

	case 'r':			/*   remove segment 		*/
	    smcListPages (smc, -1);
	    printf ("which page? ");  scanf ("%d", &pagenum);
	    page = &smc->pdata[pagenum];
	    if (page->ac_locked)
		printf ("Page is locked for access.\n");
	    else
	        smcDetach (smc, page, TRUE);
	    break;

	/****************************************************************/

	case 'q':			/*   quit			*/
	case 'C':			/*   close cache		*/
	    printf ("free cache? ");    scanf ("%s", val);
    	    smc = smcClose (smc, is_true(val[0]));
	    printf ("After smcClose(): smc = 0x%x\n", smc);
	    break;

	case 'I':			/*   initialize cache		*/
	    if (smc) {
	        printf ("Initializing cache....\n");
    	        stat = smcInitialize (smc);
    	        if (stat && debug) 
		    smcPrintCacheInfo (smc, "After Initialize:");
	    } else
		printf ("Cannot initialize() before open()\n");

	    break;

	case 'O':			/*   open cache			*/
	    printf ("cache_file: ");    scanf ("%s", fname);
    	    sprintf (config,"debug=%d,cache_file=%s\0", debug,fname);

	    printf ("Opening cache....'%s'\n", config);
    	    if ((smc = smcOpen (config)) == (smCache_t *)NULL)
		fprintf (stderr, "Error opening cache, invalid file?.\n");

	    break;

	case 'P':			/*   print cache info		*/
	    if (smc) {
	        smcPrintCacheInfo (smc, (char *)NULL);
	        smcPrintCfgInfo (&smc->sysConfig, (char *)NULL);
	    } else
		printf ("Null cache ptr, try opening a cache first.\n");
	    break;

	/****************************************************************/

	case 'L':			/*   dump cache lock file	*/
	    if (smc) {
	        sprintf (buf, "cat %s\0", smc->sysConfig.cache_path);
	        system (buf);
	    } else
		printf ("Null cache ptr, try again.\n");
	    break;

	case 'S':			/*   dump shmem info		*/
	    /*
	    system("cat /proc/sysvipc/shm");
	    */
	    /*
	    if (smc) {
	        sprintf (buf, "ipcs -m -i %d\0", smc->shmid);
	        system (buf);
	    } else 
	    */
	        system("ipcs -m");
	    break;

	case 'T':			/*   toggle debug flag		*/
	    debug != debug;
	    smcToggleDebug (smc);
	    break;

	case '?':
	    Usage ();
	    break;

	case '\n':			/*   no-op			*/
            printf ("zzdebug (%d)>  ", getpid());
	    continue;

	default:
	    printf ("Unknown command: '%c'\n", c);
	}
	

	if (c == 'q' || c == 'C')
	    break;
    }

    printf ("Done\n");
}


ZZINIT (int argc, char **argv)
{
    int i, j, nsegs=4;

    if (argc <= 1)
	return;

    printf ("Opening cache....\n");
    if ((smc = smcOpen ("debug=1")) == (smCache_t *)NULL)
	fprintf (stderr, "Error opening cache, invalid file?.\n");

for (j = 0; j < 100; j++) {
printf ("Iteration %d\n", j);

    /* Create four new pages. */
    for (i=1; i <= j; i++) 
   	page = smcGetPage (smc, TY_DATA, (i*256), TRUE, FALSE);
	        
    /* Detach but don't release them. */
    for (i=0; i < j; i++) 
        smcDetach (smc, &smc->pdata[i], FALSE);
	        
    /* Release the second page. */
    smcDetach (smc, &smc->pdata[1], TRUE);

    /* Detach and release them. */
    for (i=0; i < j; i++) 
        smcDetach (smc, &smc->pdata[i], TRUE);
	        
    smcListPages (smc, -1);

}
}


optargi ()
{
    int  i, ival;
    char c, val[32];

    bzero (val, 32);
    for (i=0; i<32 && c != '\n'; i++)
	val[i] = (c = getchar());
        
    if (c == '\n')
	ungetc (c,stdin);
    
    return ( ((ival= atoi(val)) ? ival : -1) );
}


Usage ()
{
    printf ("Command Summary:\n");
    printf ("   a   attach to segment\t   I   initialize cache\n");
    printf ("   d   detach from segment\t   D   detach from cache\n");
    printf ("   f   fill segment w/ data\t   C   close cache\n");
    printf ("   p   print segment info\t   O   open cache\n");
    printf ("   n   new segment\t\t   P   print cache info\n");
    printf ("   l   list segments\t\t   c   cleanup segments\n");
    printf ("\n");
    printf ("   S   dump shmem info\t\t   T   toggle debug\n");
    printf ("   L   dump cache lock file\n");
    printf ("   q   quit\t\t\t   ?   help\n");
    printf ("\n");
}



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
