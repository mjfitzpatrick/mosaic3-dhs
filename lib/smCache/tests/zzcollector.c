#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "smCache.h"


typedef unsigned char uchar;

extern 	smCache_t *smcOpen();
extern 	smcPage_t *smcGetPage();

int	color_mode = 1;
int	im_nx 	  = 2048;
int	im_ny 	  = 2048;
int	im_bitpix = 32;

smCache_t *smc  = (smCache_t *)NULL;
smcPage_t *page = (smcPage_t *)NULL;
smcSegment_t *seg = (smcSegment_t *)NULL;


main (int argc, char **argv) 
{
    char   c, fname[64], buf[64], resp[32], val[12], config[128];
    int	   i, j, debug=0, clear = 1, nimages=1, delay=0, interactive=1;
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
        } else if (strncmp (argv[i], "-delay", 2) == 0) {
	    delay = atoi (argv[++i]);
        } else if (strncmp (argv[i], "-x", 2) == 0) {
	    debug++;
        } else if (strncmp (argv[i], "-nimages", 2) == 0) {
	    nimages = atoi (argv[++i]);
        } else if (strncmp (argv[i], "-keep", 2) == 0) {
	    clear = 0;
        } else if (strncmp (argv[i], "-interactive", 2) == 0) {
	    interactive = 1;
        } else if (strncmp (argv[i], "-batch", 2) == 0) {
	    interactive = 0;
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
    for (i=1; i <= nimages; i++) {
	printf ("Generating image %d ....", i);
        simMetaData ();				/* pre-readout header	*/
        simData ();				/* pixel data		*/
        simMetaData ();				/* post-readout header	*/
	printf ("done.\n");

        sleep (delay); 				/* pause requested time */
    }


    /* Once we're done, see if we want drop into a command mode.
     */
    if (interactive) {
	imnum = nimages;

        /* Command loop */
        printf ("%s (%d)>  ", argv[0], getpid());
        while ((c = getchar())) {

	    switch (c) {
            case 'n':                       /*   new sequence		*/
		stime = smUtilTime();
                nimages = optargi ();
                nimages = max (1, nimages);
		for (i=0; i < nimages; i++) {
		    printf ("Generating readout %d (%d/%d) ....", 
			imnum++, smc->npages, smc->top);
        	    simMetaData ();
		        simData ();
		    simMetaData ();
		    printf ("done.\n");
		}
		etime = smUtilTime();
		printf ("Created %d images in %f sec\n",nimages,(etime-stime));
                break;

            case 'd':                       /*   detach from segment        */
                smcListPages (smc, -1);
                printf ("which page? ");  scanf ("%d", &pagenum);
                page = &smc->pdata[pagenum];
                if (page->ac_locked)
                    printf ("Page is locked for access.\n");
                else
                    smcDetach (smc, page, FALSE);
                break;

            case 'l':                       /*   list segments              */
                nsegs = optargi ();
                if (nsegs < 0 && smc->npages == 0) {
                    printf ("No pages yet allocated, showing first 8.\n");
                    smcListPages (smc, 8);
                } else
                    smcListPages (smc, nsegs);
                break;

            case 'i':                       /*   initialize cache           */
            case 'I':
                if (smc) {
		    int stat;
                    printf ("Initializing cache....\n");
                    stat = smcInitialize (smc);
                    if (stat && debug) 
                        smcPrintCacheInfo (smc, "After Initialize:");
                } else
                    printf ("Cannot initialize() before open()\n");
                break;

	    case '\n':
                printf ("%s (%d)>  ", argv[0], getpid());
	        continue;

            case 'p':                       /*   print cache info           */
                if (smc) {
                    smcPrintCacheInfo (smc, (char *)NULL);
                    smcPrintCfgInfo (&smc->sysConfig, (char *)NULL);
                } else
                    printf ("Null cache ptr, try opening a cache first.\n");
            break;


            case 'q':                       /*   quit                       */
            case 'C':                       /*   close cache                */
                printf ("free cache? ");    scanf ("%s", val);
                smc = smcClose (smc, is_true(val[0]));
                printf ("After smcClose(): smc = 0x%x\n", smc);
                break;

            case 'g':                       /*   print segment data         */
                smcListPages (smc, -1);
                printf ("which page? ");  scanf ("%d", &pagenum);
                page = &smc->pdata[pagenum];
                if (page->ac_locked)
                    printf ("Page is locked for access.\n");
                else {
                    printf ("how many lines? ");  scanf ("%d", &nvals);
                    if (page->type == TY_META) {
                        cdata = (char *)smcGetPageData (page);
                        for (cp=cdata, i=0; i < nvals; i++) {
                            printf ("%.80s", cp);
			    cp += 80;
			}
                    } else {
                        idata = (int *)smcGetPageData (page);
                        for (i=0; i < nvals; i++)
                            printf ("(%d) %s", idata[i],(i&&(i%4==0))?"\n":"");
                    }
                    printf ("\n");
                }
                break;


	    default:
	        printf ("Unknown command: '%c'\n", c);
	    }

            if (c == 'q' || c == 'C')
                break;
        }
    }

    printf ("Done\n");
    if (smc && !interactive) 
	smcClose (smc, clear);
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


Usage ()
{
    printf ("Command Summary:\n");
    printf ("   a   attach to segment\t   I   initialize cache\n");
}

printHelp ()
{
    printf ("   -batch          create image in batch mode and quit\n");
    printf ("   -cache <file>   attach/open <file> cache\n");
    printf ("   -delay N        delay N seconds between images\n");
    printf ("   -help           this message\n");
    printf ("   -interactive    create image and drop into cmd loop\n");
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


