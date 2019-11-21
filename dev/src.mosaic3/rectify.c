/**
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#include "../../include/location.h"		/* REQUIRED	*/
#include "../../include/instrument.h"		/* REQUIRED	*/

#include "mbus.h"
#include "smCache.h"
#include "smcmgr.h"


#define	BITPIX			32


extern  int trim_ref;                   /* trim reference pix flag      */
extern  int rotate_enable;
extern  int console, verbose, debug;

extern void rotate(void *iadr, XLONG **oadr, int type, int nx, int ny, int dir);



/*  smcRectifyPage -- Rectify the display pixels of an SMC page.
*/
void
smcRectifyPage (page)
smcPage_t *page;
{
    int    i, j, k, nx=0, ny=0, xbias_start;
    XLONG  *pix, *dpix, *sp, *sp0, *ep, npix, temp, xdir, ydir;
    static XLONG  sv_nx=0, sv_ny=0, first_time=1;
    static XLONG *dpix_buf=(XLONG *)NULL;

    fpConfig_t *fp;


    if (!rotate_enable){			/* sanity checks		*/
	fprintf(stderr,"smcRectify: rotate_enable not set. Skipping rectification\n");
	fflush(stderr);
	return;
    }

    // hardwire start of x and y bias for now

    xbias_start = XBIAS_COL;
    fp = smcGetFPConfig (page);		/* get the focal plane config	*/
    nx = fp->xSize;
    ny = fp->ySize;
    xdir = fp->xDir;
    ydir = fp->yDir;
    npix = nx * ny;

    if(verbose){
	fprintf(stderr,"smcRectify: nx = %d ny = %d xdir = %d ydir = %d  xbias = %d\n",nx,ny,xdir,ydir,xbias_start);
    }

    if( ( xdir != -1 && xdir != 1 ) || ( ydir != -1 && ydir != 1 ) ) {
	fprintf(stderr,"smcRectify: unexpected values for xdir and ydir %d %d\n",xdir,ydir);
	fflush(stderr);
        return;
    }

    if ( xbias_start > nx ){
	fprintf(stderr,"smcRectify: unexpected value for nx %d \n",nx);
	fflush(stderr);
        return;
    }

    /* Get the pixel pointer and raster dimensions.  Allocate space that 
    ** includes the reference in case we use it later.
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
    } else {
        dpix = (XLONG *) dpix_buf;
    }

    if (xdir == 1 && ydir == 1 ) return;

    
    // assume no overscan rows, only overscan cols at end of each line
    // put the rectified image into the destination buffer (dpix_buf)

    for(i=0;i<ny;i++){

	 ep = dpix + i*nx;

	 if(ydir==-1)
	   sp0 = pix + (ny-i-1) * nx;
         else
	   sp0 = pix + i*nx;

         if(xdir==-1)
	   sp = sp0 + xbias_start-1;
         else
	   sp = sp0;
	  
	 // first copy image pixels     
         for(j=0;j<xbias_start;j++){
	    *ep++  = *sp;
	    sp = sp + xdir;
         }
	
	 // now copy bias pixels
	 if(xdir==-1)
	    sp = sp0 + nx - 1 ;
         else
            sp = sp0 + xbias_start -1;

	 for(j=xbias_start;j<nx;j++){
	    *ep++  = *sp;
	    sp = sp + xdir;
	 }

	 
    }

    /*  copy the rectified pixels from dpix_bug back into the source buffer (pix)*/
    for(i=0;i<npix;i++)*(pix+i)=*(dpix+i);

}
