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




/* Update the pixel status display in the Supervisor GUI.
*/
void
rtdUpdateStats (pixStat *stats)
{
    char   buf[256], msg[1024];

	
    bzero (msg, 1024); 
    strcat (msg, "rtdStat stat {");

    if (stat_enable) {
	memset (buf, 0, 256);
        sprintf (buf, "%-7.7s  %9.3f  %-9.2f  %9.2f  %9.2f  %7.2f  %7.2f\n",
	    stats->detName, 
	    stats->mean, stats->sigma, 
	    stats->min, stats->max,
	    stats->z1, stats->z2);

	/*
        strncat (msg, buf, strlen(buf));
	memset (buf, 0, 256);
        sprintf (buf, "%-7.7s  %9.3f  %-9.2f  %9.2f  %9.2f\n\n",
	    "  (ref)",
	    stats->rmean, stats->rsigma, stats->rmin, stats->rmax);
	*/

    } else {
	memset (buf, 0, 256);
        sprintf (buf, "%-7.7s  %7.2f  %7.2f\n",
	    stats->detName, stats->z1, stats->z2);
    }

    strncat (msg, buf, strlen(buf));
    strcat (msg, "}\0");

    /* Send it to the Supervisor.
    */
    if (use_mbus)
        mbusSend (SUPERVISOR, ANY, MB_SET, msg);
}
