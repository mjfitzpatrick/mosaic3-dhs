#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <pthread.h>

#include "cdl.h"
#include "mbus.h"
#include "smCache.h"
#include "location.h"
#include "instrument.h"
#include "smcmgr.h"


#define	DEBUG		0


/*  SMCRTD -- Real-Time Display Client code for the SMCMGR.
 */


pthread_mutex_t rtd_mut = PTHREAD_MUTEX_INITIALIZER;

extern 	int trim_ref;			/* trim reference pix flag */
extern  int disp_enable;
extern  int stat_enable;
extern  int rotate_enable;
extern  int verbose, debug;
extern  int use_mbus, use_threads;
extern  int lo_gain, otf_enable;
extern  int procDebug;
    
extern  CDLPtr  cdl;

#define DISP_GAP		32
#define REFERENCE_WIDTH		47
#define BIAS_WIDTH		47 


static char  *rtdGets (char *s, int len, FILE *fp);
static void   rtdLoadOTF (void);
static void   rtdStats (XLONG *pix, int nx, int ny, int ccd, float z1,float z2);
static XLONG *rtd_ampLeft (XLONG *data, int nx, int ny);
static XLONG *rtd_ampRight (XLONG *data, int nx, int ny);

void rtdPixelStats (XLONG *pix, int nx, int ny, pixStat *stats);
void rtdUpdateStats (pixStat *stat);

typedef struct {
   int   extn;				/* extension number		*/
   int   nzero;				/* normal mode bias		*/
   float nscale;			/* normal mode scale		*/
   int   lzero;				/* lo gain mode bias		*/
   float lscale;			/* lo gain mode scale		*/
} otfScale, *otfScaleP;

otfScale otf[16];



/*  RTDDISPLAYPIXELS -- Display the pixels of a SMC page.
*/
void
rtdDisplayPixels (void *cdlP, smcPage_t *page)
{
    int    i, j, nx=0, ny=0, xs=0, ys=0, bitpix=32, skip = 0, ccd = 0;
    int    fb_w, fb_h, fbconfig, nf, nb, rowlen, collen, lx, ly;
    XLONG   *pix, *dpix, *ip, *op, *row, *pv;
    float  z1, z2;
    CDLPtr cdl = (CDLPtr) cdlP;
    char   *det = NULL;
    /*static*/ XLONG *dpix_buf=(XLONG *)NULL, sv_nx=0, sv_ny=0, first_time=1;
    fpConfig_t *fp;

    extern smCache_t *smc;
    extern int  use_threads;


    if (!disp_enable)			/* sanity checks		*/
	return;
    if (!cdl)
	return;

    rtdLoadOTF ();

    fp = smcGetFPConfig (page);		/* get the focal plane config	*/
    nx = fp->xSize;
    ny = fp->ySize;
    xs = fp->xStart;
    ys = fp->yStart;


    
    if(debug)fprintf(stderr,"rtdDisplayPixels: nx = %d\n", nx);
    if(debug)fprintf(stderr,"rtdDisplayPixels: ny = %d\n", ny);
    if(debug)fprintf(stderr,"rtdDisplayPixels: xs = %d\n", xs);
    if(debug)fprintf(stderr,"rtdDisplayPixels: ys = %d\n", ys);

    /* Get the pixel pointer and raster dimensions.  Allocate space for a
    ** buffer we'll reuse as long as the size remains the same to cut down
    ** on memory allocation in a long-running process,
    */
    pix  = (XLONG *) smcGetPageData (page);
    if ((nx*ny) != (sv_nx*sv_ny)) {
	if (dpix_buf) free ((XLONG *)dpix_buf);
	sv_nx = nx;
	sv_ny = ny;
	first_time = 1;
    }
    if (first_time || use_threads) {
        dpix = dpix_buf = (XLONG *) calloc ((nx*ny), sizeof (XLONG));
        first_time = 0;
    } else
        dpix = (XLONG *)dpix_buf;


    /* Select a frame buffer large enough for the image.  
    */
    cdl_selectFB (cdl, (4*nx+3*DISP_GAP), (2*ny+3*DISP_GAP),
	&fbconfig, &fb_w, &fb_h, &nf, 1);
	
    // hard code bias column
    rowlen = XBIAS_COL;
    if (xs > 6000) {
	lx = (fb_w / 2) + 3 * (DISP_GAP/2) + rowlen; 		ccd = 3;
    } else if (xs > 4000) {
	lx = (fb_w / 2) +     (DISP_GAP/2); 			ccd = 2;
    } else if (xs > 2000) {
	lx = (fb_w / 2) - 1 * (DISP_GAP/2) - (1 * rowlen); 	ccd = 1;
    } else {
	lx = (fb_w / 2) - 3 * (DISP_GAP/2) - (2 * rowlen); 	ccd = 0;
    }
	
    /* assume no overscan rows */
    collen = ny ;
    if (ys > 6000) {
	ly = (fb_h / 2) + 3 * (DISP_GAP/2) + collen; 		ccd = 3;
    } else if (ys > 4000) {
	ly = (fb_h / 2) +     (DISP_GAP/2); 			ccd = 2;
    } else if (ys > 2000) {
	ly = (fb_h / 2) - 1 * (DISP_GAP/2) - (1 * collen); 	ccd = 1;
    } else {
	ly = (fb_h / 2) - 3 * (DISP_GAP/2) - (2 * collen); 	ccd = 0;
    }


    if(debug)fprintf(stderr,"rtdDisplayPixels: lx = %d\n", lx);
    if(debug)fprintf(stderr,"rtdDisplayPixels: ly = %d\n", ly);

    if (verbose && debug) {
	printf ("Using Frame Buffer #%d: %d x %d (%d frames)\n",
	    fbconfig, fb_w, fb_h, nf);
    }

	
    /* Now copy the page pixels to the display raster.  We will trim the
    ** the 64 reference pixels at the end of each row if needed and reset
    ** our origin based on the location of the chip.  For NEWFIRM the 4
    ** detectors are assumed to tbe layed out as follows:
    **
    **	    +----+----+----+----+
    **      | A8 | A7 | A6 | A5 |
    **      |    |    |    |    |
    **	    +----+----+----+----+
    **      | A4 | A3 | A2 | A1 |
    **      |    |    |    |    |
    **	    +----+----+----+----+
    */
    if (trim_ref) {			/* extract display area	*/
	skip = 0;
        ip = pix + skip;
    	op = dpix;
	//rowlen = nx - 100;
        rowlen = XBIAS_COL;
	nb = rowlen * (bitpix / 8);

	row = (XLONG *) malloc (nb);

	for (i=0; i < ny; i++) {
            if (! otf_enable) {
	        bcopy (ip, op, nb); 
	    } else {
		int   zero  = 0;
		float scale = 1.0;

	        pv = (XLONG *) ip;

		zero = (lo_gain ? otf[ccd].lzero : otf[ccd].nzero);
		scale = (lo_gain ? otf[ccd].lscale : otf[ccd].nscale);
	        for (j=0; j < (rowlen / 2); j++)
		    row[j] = (*pv++ - zero) * scale;

		zero = (lo_gain ? otf[ccd+8].lzero : otf[ccd+8].nzero);
		scale = (lo_gain ? otf[ccd+8].lscale : otf[ccd+8].nscale);
	        for (   ; j <  rowlen     ; j++)
		    row[j] = (*pv++ - zero) * scale;

                bcopy (row, op, nb);
	    }

	    ip += nx;
	    op += rowlen;
	}
	free (row);

    } else {
	rowlen = nx;
        bcopy (pix, dpix, (nx * ny * (bitpix / 8)));
    }

    if ((det = smcGetFRoot (smc)) != NULL) {
	static char name[SZ_PATH];

	memset (name, 0, SZ_PATH);
	sprintf (name, "%s%05d", det, smcGetPageSeqNo (page));
	det = name;
    }

    /* Z-scale the raster for display.
    */
    cdl_computeZscale  (cdl, dpix, rowlen, ny, bitpix, &z1, &z2);
    cdl_zscaleImage (cdl, &dpix, rowlen, ny, bitpix, z1, z2);
/*
    stats.z1 = z1;
    stats.z2 = z2;
    stats.detName = det;
*/

    /* Compute the pixel statistics.  Note the reference pixels are still on
    ** the right side of the array at this stage.
    */
    if (stat_enable) 
        rtdStats (pix, nx, ny, ccd, z1, z2);

    if (verbose)
        printf ("%s:  lx=%4d ly=%4d nx=%4d ny=%4d  z1=%.3f z2=%.3f\n", 
	    det, lx, ly, rowlen, ny, z1, z2);
	    
    /*  Make a guess at the WCS, need this to set the fbconfig.
    */
    cdl_setWCS (cdl, det, "RTD", 1.0, 0.0, 0.0, -1.0, 
	(float)lx, (float)(fb_h-ly), z1, z2, 1);

    /*  Write the subraster to the display.
    */
    cdl_writeSubRaster (cdl, lx, ly, rowlen, ny, dpix);

    free (dpix);
}


/*  RTDPIXELSTATS -- Compute the pixel raster statistics.  Stats are
**  accumulated for both the display area as well as the reference pixels.
**  We assume we're being passed the raw readout array that includes the
**  reference strip at the right side of the array.
*/
static void
rtdStats (XLONG *pix, int nx, int ny, int ccd, float z1, float z2)
{
    pixStat stats;
    XLONG  *left, *right;
    static  char ldet[32], rdet[32];


    memset (ldet, 0, 32);
    memset (rdet, 0, 32);

    left = rtd_ampLeft (pix, nx, ny);
    rtdPixelStats (left, nx/2, ny, &stats);
    sprintf (ldet, "%d", ccd);
    stats.detName = ldet;
    stats.z1 = z1;
    stats.z2 = z2;
    rtdUpdateStats (&stats);
    free (left);

    right = rtd_ampRight (pix, nx, ny);
    rtdPixelStats (right, nx/2, ny, &stats);
    sprintf (rdet, "%d", (ccd+8));
    stats.detName = rdet;
    stats.z1 = z1;
    stats.z2 = z2;
    rtdUpdateStats (&stats);
    free (right);
}


/*  RTDPIXELSTATS -- Compute the pixel raster statistics.  Stats are
**  accumulated for both the display area as well as the reference pixels.
**  We assume we're being passed the raw readout array that includes the
**  reference strip at the right side of the array.
*/
void
rtdPixelStats (XLONG *pix, int nx, int ny, pixStat *stats)
{
    int   i, j, val;
    int   rowlen=(nx - REFERENCE_WIDTH);
    int   npix = (rowlen * ny);
    double sum=0.0, sum2=0.0, mean=0.0, sigma=0.0, min=100000, max = -100000;


    rowlen = (nx - REFERENCE_WIDTH);
    npix = rowlen * ny;

    for (i=0; i < ny; i++) {                    /* accumulate sums      */
        for (j=0; j < rowlen; j++) {
            val = pix[(i*rowlen) + j];		
						/* active pixels	*/
            sum += val;
            if (val && val < min) min = val;
            if (val && val > max) max = val;
        }
    }
    mean = (sum / (double)npix);

    for (i=0; i < ny; i++) {
        for (j=0; j < rowlen; j++) {
            val = pix[(i*rowlen) + j];
            sum2 += ( (val - mean) * (val - mean) );
        }
    }
    sigma = sqrt ( (sum2 / (double)npix) );

    stats->mean   = mean;
    stats->sigma  = sigma;
    stats->min    = min;
    stats->max    = max;

    if (verbose || DEBUG)
        printf ("mean=%.4f +- %.4f  min/max=%f/%f \n", mean, sigma, min, max);
}


/**
 *  RTDDISPLAYTHREAD -- Thread worker function for RTD.
 */
void
rtdDisplayThread (void *data)
{
    rtdArg *arg = (rtdArg *) data;
    int    tnum = arg->nthread;
    smCache_t *smc  = (smCache_t *)arg->smc;
    smcPage_t *page = (smcPage_t *)arg->page;

    CDLPtr  cdl;
    int    stat = 0, base = 0, port = 0;
    char   dev[SZ_LINE];

    extern  int disp_frame, disp_stdimg, disp_done;
    extern  char *imtdev;


    sscanf (imtdev, "fast:inet:%d", &base);
    memset (dev, 0, SZ_LINE);
    sprintf (dev, "fast:inet:%d", (port = base + tnum));

    if ((cdl = cdl_open (dev)) == (CDLPtr) NULL)
        fprintf (stderr, "ERROR: cannot connect to display server.");
   
    if (tnum == 0) {
        cdl_setFBConfig (cdl, disp_stdimg);
        cdl_setFrame (cdl, disp_frame);
	cdl_clearFrame (cdl);
    } 

    rtdDisplayPixels (cdl, page);

    cdl_close (cdl);

    smcDetach (smc, page, FALSE);

    if (tnum == 8)
	disp_done = 1;

    pthread_exit (&stat);
}


/**
 *  RTDLOADOTF -- Load the OTF scale values
 */
#define	OTF_FILE	"/dhs/lib/mosaic3.otf"

static void 
rtdLoadOTF ()
{
    FILE  *fd;
    static char line[SZ_LINE], *lp;
    int	   lnum = 0;


    if ((fd = fopen (OTF_FILE, "r")) == (FILE *) NULL)
	return;

    lnum = 0;
    while (rtdGets (line, SZ_LINE, fd)) {
        /*  Skip comments and empty lines.
        */
        for (lp=line; *lp && isspace(*lp); lp++)
            ;
        if (*lp == '#' || *lp == '\0')
            continue;

	sscanf (lp, "%d %d %g %d %g", &otf[lnum].extn, 
	    &otf[lnum].nzero, &otf[lnum].nscale,
	    &otf[lnum].lzero, &otf[lnum].lscale);
	lnum++;

	if (lnum == 16)
	    break;
    }

    fclose (fd);
}


/**
 *  RTDGETS   A smart fgets() function
 */
static char *
rtdGets (char *s, int len, FILE *fp)
{
    char  *ptr, *end;
    int    ch;


    if (!s)
        return ((char *)NULL);

    ptr = s;
    end = s + len - 1;

    memset (ptr, 0, len);
    while ((ch = getc(fp)) != EOF) {
        if (ch == '\n')
            break;
        else if (ch == '\r') {
            /* See if a LF follows...
            */
            ch = getc(fp);
            if (ch != '\n')
                ungetc(ch, fp);
            break;

        } else if (ptr < end)
            *ptr++ = ch;
    }
    *ptr = '\0';

    if (ch == EOF && ptr == s)
        return (NULL);
    else {
        int len = strlen (s);

        /* Trim trailing whitespace.
        */
        end = (s + len - 1);
        while ( end > s && *end && isspace(*end) )
            *end-- = '\0';

        return (s);
    }
}



/**
 *  
 */

static XLONG *
rtd_ampLeft (XLONG *data, int nx, int ny)
{
    int  *amp, *ip, *op, i, j;
    int   npix = (nx - BIAS_WIDTH) / 2;

    ip = data;
    op = amp = calloc ((nx * ny), sizeof(XLONG));

    for (j=0; j < ny; j++) {
        for (i=0; i < npix; i++)                /* copy pixels  	*/
            *op++ = *ip++;

        ip += npix + BIAS_WIDTH;                /* skip amp & bias 	*/
    }

    return (amp);
}

static XLONG *
rtd_ampRight (XLONG *data, int nx, int ny)
{
    int  *amp, *ip, *op, i, j;
    int   npix = (nx - BIAS_WIDTH) / 2;

    ip = data + npix;
    op = amp = calloc ((nx * ny), sizeof(XLONG));

    for (j=0; j < ny; j++) {
        for (i=0; i < npix; i++)                /* copy pixels  */
            *op++ = *ip++;
        ip += npix + BIAS_WIDTH;                /* skip amp & bias 	*/
    }

    return (amp);
}

