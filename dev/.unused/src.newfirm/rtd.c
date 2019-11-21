#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#include "cdl.h"
#include "mbus.h"
#include "smCache.h"
#include "location.h"
#include "instrument.h"
#include "smcmgr.h"


/*  SMCRTD -- Real-Time Display Client code for the SMCMGR.
 */


extern 	int trim_ref;			/* trim reference pix flag */
extern  int disp_enable;
extern  int stat_enable;
extern  int rotate_enable;
extern  int verbose, debug;
extern  int use_mbus;
    
extern  CDLPtr  cdl;

#define DISP_GAP		32
#define REFERENCE_WIDTH		64


void rtdPixelStats (XLONG *pix, int nx, int ny, pixStat *stats);
void rtdUpdateStats (pixStat *stat);



/*  rtdDisplayPixels -- Display the pixels of a SMC page.
*/
void
rtdDisplayPixels (smcPage_t *page)
{
    int    i, nx=0, ny=0, xs=0, ys=0, bitpix=32;
    int    fb_w, fb_h, fbconfig, nf, nb, rowlen, lx, ly;
    XLONG   *pix, *dpix, *ip, *op;
    float  z1, z2;
    char   *det = NULL;

    static XLONG *dpix_buf=(XLONG *)NULL, sv_nx=0, sv_ny=0, first_time=1;

    fpConfig_t *fp;
    pixStat stats;


    if (!disp_enable)			/* sanity checks		*/
	return;
    if (!cdl)
	return;

    fp = smcGetFPConfig (page);		/* get the focal plane config	*/
    nx = fp->xSize;
    ny = fp->ySize;
    xs = fp->xStart;
    ys = fp->yStart;

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
    if (first_time) {
        dpix = dpix_buf = (XLONG *) calloc ((nx*ny), sizeof (XLONG));
        first_time = 0;
    } else
        dpix = (XLONG *)dpix_buf;

    /* Select a frame buffer large enough for the image.  
    */
    cdl_selectFB (cdl, (2*nx+3*DISP_GAP), (2*ny+3*DISP_GAP),
	&fbconfig, &fb_w, &fb_h, &nf, 1);
	
    if (verbose && debug) {
	printf ("Using Frame Buffer #%d: %d x %d (%d frames)\n",
	    fbconfig, fb_w, fb_h, nf);
    }


    /* Now copy the page pixels to the display raster.  We will trim the
    ** the 64 reference pixels at the end of each row if needed and reset
    ** our origin based on the location of the chip.  For NEWFIRM the 4
    ** detectors are assumed to tbe layed out as follows:
    **
    **	    +----+----+	    where   A1,A2  is Array 1 or 2
    **      | A2 | A1 |		    Pa	   is Pan A
    **      | Pb | Pb |		    Pb	   is Pan B
    **	    +----+----+
    **      | A1 | A2 |	    The SMC page contains the data such that the
    **      | Pa | Pa |	    starting address represents the corners of
    **	    +----+----+	    the detector area, w/ readout moving to the
    **			    centers.  We'll trim the reference pixels and
    ** reset our ogin for the display as needed.
    */
    if (trim_ref) {			/* extract display area	*/
        ip = pix;
    	op = dpix;
	rowlen = nx - REFERENCE_WIDTH;
	nb = rowlen * (bitpix / 8);
	for (i=0; i < ny; i++) {
            bcopy (ip, op, nb);
	    ip += (rowlen + REFERENCE_WIDTH);
	    op += rowlen;
/*
	    ip += (rowlen + REFERENCE_WIDTH) * sizeof(XLONG);
	    op += (rowlen) * sizeof(XLONG);
*/
	}

    } else {
	rowlen = nx;
        bcopy (pix, dpix, (nx*ny*(bitpix/8)));
    }

    /* Compute the pixel statistics.  Note the reference pixels are still on
    ** the right side of the array at this stage.
    */
    if (stat_enable)
        rtdPixelStats (pix, nx, ny, &stats);

#ifdef KPNO

#ifdef DISPLAY_ORIGINAL_DETECTOR

    if (xs == 0 && ys == 0) {			/* A1/Pa 	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan A/1";

    } else if (xs != 0 && ys == 0) {		/* A2/Pa 	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan A/2";

    } else if (xs == 0 && ys != 0) {		/* A2/Pb 	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan B/2";

    } else if (xs != 0 && ys != 0) {		/* A1/Pb 	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan B/1";
    }

#else
#ifdef DISPLAY_FALL07_LAYOUT

    /*  Note that the conditionals are based on the original detector
    **  layout, we define the (lx,ly) so they are moved to the proper
    **  quadrant for the display.
    */
    if (xs == 0 && ys == 0) {			/* A1	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan A/1";

    } else if (xs != 0 && ys == 0) {		/* A2	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan A/2";

    } else if (xs == 0 && ys != 0) {		/* B2	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan B/2";

    } else if (xs != 0 && ys != 0) {		/* B1	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan B/1";
    }

#else
    /*  Note that the conditionals are based on the original detector
    **  layout, we define the (lx,ly) so they are moved to the proper
    **  quadrant for the display.
    **  
    **  This is the configuration used for Feb08  
    */
    if (xs == 0 && ys == 0) {			/* A1	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan A/1";

    } else if (xs != 0 && ys == 0) {		/* B2	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan B/2";

    } else if (xs == 0 && ys != 0) {		/* A2	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan A/2";

    } else if (xs != 0 && ys != 0) {		/* B1	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan B/1";
    }
#endif
#endif

#else

#ifdef CTIO
    /*  Note that the conditionals are based on the original detector
    **  layout, we define the (lx,ly) so they are moved to the proper
    **  quadrant for the display.
    **  
    **  This is the configuration used for Feb08  
    */
    if (xs == 0 && ys == 0) {			/* B1	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan B/1";

    } else if (xs != 0 && ys == 0) {		/* B2	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) + (DISP_GAP/2);
	det = "Pan B/2";

    } else if (xs == 0 && ys != 0) {		/* A1	*/
	lx = (fb_w / 2) - (DISP_GAP/2) - rowlen;
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan A/1";

    } else if (xs != 0 && ys != 0) {		/* S2	*/
	lx = (fb_w / 2) + (DISP_GAP/2);
	ly = (fb_h / 2) - (DISP_GAP/2) - ny;
	det = "Pan A/2";
    }
#endif

#endif

    /* Z-scale the raster for display.
    */
    cdl_computeZscale  (cdl, dpix, rowlen, ny, bitpix, &z1, &z2);
    cdl_zscaleImage (cdl, &dpix, rowlen, ny, bitpix, z1, z2);
    stats.z1 = z1;
    stats.z2 = z2;
    stats.detName = det;

    if (verbose)
	printf ("%s:  lx=%4d ly=%4d nx=%4d ny=%4d  z1=%.3f z2=%.3f\n", 
	    det, lx, ly, rowlen, ny, z1, z2);

    /*  Make a guess at the WCS, need this to set the fbconfig.
    */
    cdl_setWCS (cdl, det, det, 1.0, 0.0, 0.0, -1.0, 
	(float)lx, (float)(fb_h-ly), z1, z2, 1);

    /*  Write the subraster to the display.
    */
    cdl_writeSubRaster (cdl, lx, ly, rowlen, ny, dpix);


    /* Update the status display in the Supervisor GUI.
    */
    rtdUpdateStats (&stats);

    /*free (dpix);*/ 				/* clean up.	*/
}
