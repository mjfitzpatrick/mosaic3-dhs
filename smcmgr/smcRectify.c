#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#include "cdl.h"
#include "mbus.h"
#include "smCache.h"
#include "location.h"
#include "instrument.h"
#include "detector.h"
#include "smcmgr.h"


/*  SMCRECTIFY -- Rectify the display area of the DATA pages being processed
**  depending on their location in the focal plane.  Write the result back
**  to the SMC page in the proper orientation.
*/


extern 	int trim_ref;			/* trim reference pix flag 	*/
extern  int rotate_enable;
extern  int console, verbose, debug;


int	rectifyDir (int xs, int ys);

    
/*  smcRectify -- Rectify all the rasters of an exposure given an expID.
*/
void
smcRectify (smc, expID)
smCache_t *smc;
double expID;
{
    smcPage_t *page;

    while ((page = smcNextByExpID (smc, expID))) {
        smcAttach (smc, page);
	if (page->type == TY_DATA || page->type == TY_VOID)
	    smcRectifyPage (page);		/* DEVICE SPECIFIC	*/
        smcDetach (smc, page, FALSE);
    }
}
