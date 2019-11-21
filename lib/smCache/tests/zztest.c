#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "smCache.h"


typedef unsigned char uchar;

extern 	smCache_t *smcOpen();
extern 	smcPage_t *smcGetPage();

int	color_mode = 1;
int	im_nx 	  = 512;
int	im_ny 	  = 512;
int	im_bitpix = 32;
int	nc=0, np=0;

smCache_t *smc  = (smCache_t *)NULL;
smcPage_t *page = (smcPage_t *)NULL;
smcSegment_t *seg = (smcSegment_t *)NULL;

smcPage_t *parray[8192];


main (int argc, char **argv) 
{
    char   c, fname[64], buf[64], resp[32], val[12], config[128];
    int	   i, j, k, debug=0, clear = 1, nimages=1;
    int    nsegs, pagenum, nvals, imnum=0;
    void   *data;
    char   *cdata, *cp;
    int    *idata, *ip;
    short  *sdata, *sp;
    double stime, etime;


    /* Process command-line arguments.
     */
    strcpy (fname, "\0");
    for (i=1; i < argc; i++) {
        if (strncmp (argv[i], "-cache", 2) == 0) {
	    strcpy (fname, argv[++i]);
        } else if (strncmp (argv[i], "-x", 2) == 0) {
	    debug++;
        } else if (strncmp (argv[i], "-nimages", 2) == 0) {
	    nimages = atoi (argv[++i]);
	    nimages = (nimages > 1024 ? 1024 : nimages);

        } else if (strncmp (argv[i], "-keep", 2) == 0) {
	    clear = 0;
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



    /*  Simulate the readout sequence>
     */
k = 1;
while (k++) {

    for (j=0; j < 1024; j++)  parray[j] = (smcPage_t *)NULL;

    for (i=0; i < nimages; i+=3) {
	printf ("Generating image %4d ....", i);

        simMetaData (i  , i % (3+k));
            simData (i+1, i % (5+k));
        simMetaData (i+2, i % (7+k));

	printf ("done.\n");

	if ((i % (nimages / 20)) == 0) {
	    printf ("Clearing %d....", np);
	    np = nc = 0;
	    for (j=0; j < 1024; j++) {
		if (parray[j]) {
    	    	    smcAttach (smc, parray[j]);
    		    smcDetach (smc, parray[j], 1);
		    parray[j] = NULL;
		    nc++;
		} 
	    }
	    printf ("found %d \n", nc);
	}
    }

    printf ("Cleaning up %d remaining pages.\n", np);
    for (j=0; j < 1024; j++) {
	if (parray[j]) {
	    printf ("X");
    	    smcAttach (smc, parray[j]);
    	    smcDetach (smc, parray[j], 1);
	} else
	    printf (".");
    }
    printf ("\n");
    smcListPages (smc, -1);			/* List all pages 	*/

    if (k > 6) k = 1;
}


    printf ("Done\n");
    smcClose (smc, 1);
}


simMetaData (int index, int flag)
{
    fpConfig_t fp;
    mdConfig_t md;
    int i, nkeyw = 256, del = (flag == 0);


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
    smcDetach (smc, page, del);

    if (del) {
	printf ("deleting...");
	parray[index] = NULL;
    } else {
	parray[index] = page;
	np++;
    }
}


simData (int index, int flag)
{
    fpConfig_t fp;
    int del = (flag == 0);


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
    smcDetach (smc, page, del);

    if (del) {
	printf ("deleting...");
	parray[index] = NULL;
    } else {
	parray[index] = page;
	np++;
    }
}


Usage ()
{
    printf ("Command Summary:\n");
    printf ("   a   attach to segment\t   I   initialize cache\n");
}

printHelp ()
{
    printf ("   -batch          create image in batch mode and quit\n");
    printf ("   -cache <file>   attach/open <file> cache\n");
    printf ("   -help           this message\n");
    printf ("   -nimages N      create N image sequences\n");
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
    
    return ( ((ival=atoi(val)) ? ival : -1) );
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


