#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

#include "smCache.h"

extern smCache_t *smcOpen();
extern smcPage_t *smcGetPage();

smCache_t *smc  = (smCache_t *)NULL;
smcPage_t *page = (smcPage_t *)NULL;
smcSegment_t *seg = (smcSegment_t *)NULL;


main (int argc, char **argv) 
{
    char  c, fname[64], root[64], buf[64], resp[32], val[12], config[128];
    char  dir[128];
    int	  i, j, debug=0, clear=1, nimages=1, delay=0, interactive=1;
    int   nsegs, pagenum, nvals, free, seqno=0;
    void  *data;
    char  *cdata, *ip;
    int   *idata;
    short *sdata;


    /* Process command-line arguments.
     */
    strcpy (dir, "./\0");
    strcpy (fname, "\0");
    strcpy (root, "img\0");
    for (i=1; i < argc; i++) {
        if (strncmp (argv[i], "-batch", 2) == 0) {
	    interactive = 0;
        } else if (strncmp (argv[i], "-cache", 2) == 0) {
	    strcpy (fname, argv[++i]);
        } else if (strncmp (argv[i], "-delay", 3) == 0) {
	    delay = atoi (argv[++i]);
        } else if (strncmp (argv[i], "-dir", 3) == 0) {
	    strcpy (dir, argv[++i]);
        } else if (strncmp (argv[i], "-interactive", 2) == 0) {
	    interactive = 1;
        } else if (strncmp (argv[i], "-root", 2) == 0) {
	    strcpy (root, argv[++i]);
        } else if (strncmp (argv[i], "-seqno", 3) == 0) {
	    seqno = atoi (argv[++i]);
        } else if (strncmp (argv[i], "-x", 2) == 0) {
	    debug++;

        } else if (strncmp (argv[i], "-help", 2) == 0) {
	    printHelp ();
	    exit (0);

	} else
	    printf ("Unrecognized option: %s\n", argv[i]);
    }


    /* Initialize the config string.
     */
    sprintf (config, "debug=%d,cache_file=%s", debug, fname);


    /*  Open/Attach to the cache.
     */
    printf ("Opening cache....\n");
    if ((smc = smcOpen (config)) == (smCache_t *)NULL)
	fprintf (stderr, "Error opening cache, invalid file?.\n");



    /* Initialize the SMC values. */
    smcSetDir (smc, dir);
    smcSetFRoot (smc, dir);
    smcSetSeqNo (smc, seqno);


    /*  Simulate the readout sequence
     */
    i = 0;
    while ((page = smcNextPage (smc, 1000))) {
	smcAttach (smc, page); 			/*  Attach to the page.  */

	switch (page->type) { 			/*  Process the data.  */
	case TY_VOID:
	    printf ("VOID: seq=%d  key=0x%x  size=%d\n", 
		i, page->memKey, page->size);
	    processData (page);
	    break;
	case TY_DATA:
	    printf ("DATA: seq=%d  key=0x%x  size=%d\n", 
		i, page->memKey, page->size);
	    processData (page);
	    break;
	case TY_META:
	    printf ("META: seq=%d  key=0x%x  size=%d\n", 
		i, page->memKey, page->size);
	    processMetaData (page);
	    break;
	default:
	    break;
	}

	i++;
	smcDetach (smc, page, (free=TRUE));	/*  Detach from the page.  */
    }


    printf ("Done\n");
    if (smc && !interactive) 
	smcClose (smc, clear);
}


/*  NOTE:  Assumes an integer data array.
 */
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

	printf ("Mean: %.4f  StdDev: %.4f  Min: %.1f  Max: %.1f\n\n", 
	    mean, sigma, min, max);
    }
}


processMetaData (smcPage_t *p)
{
    int   i, data_size;
    char *cdata, *ip, *edata;
    smcSegment_t *seg = (smcSegment_t *) NULL;


    stdHdrOutput (p);			  /* print standard header	*/

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
    printf ("\n\n"); 
}


stdHdrOutput (smcPage_t *p)
{
    mdConfig_t *md = smcGetMDConfig (p);
    fpConfig_t *fp = smcGetFPConfig (p);
    smcSegment_t *seg = (smcSegment_t *) NULL;
    smCache_t *smc  = p->smc;


    char  *dir, *root;
    int   seqno;

    seg = SEG_ADDR(p);
    printf ("  Lock: %s  size = %d   dsize = %d\n", 
	p->lockFile, p->size, seg->dsize);
    printf ("  MD Config:  metaType = %d\tnumFields = %d:\n", 
	md->metaType, md->numFields);
    printf ("  FP Config:  %d x %d  at (%d,%d) type = %d\n", 
	fp->xSize, fp->ySize, fp->xStart, fp->yStart, fp->dataType);

    printf ("  Dir:  '%s'  Root: '%s'  SeqNum: %d\n",
	smcGetDir (smc), smcGetFRoot(smc), smcGetSeqNo(smc));

}


Usage ()
{
}

printHelp ()
{
    printf ("   -cache <file>   attach/open <file> cache\n");
    printf ("   -root <file>    root name of output files\n");
    printf ("   -delay N        delay N seconds between images\n");
    printf ("   -help           this message\n");
    printf ("   -nimages N      read N image sequences (-1 => infinite)\n");
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
