#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stddef.h>


#if !defined(_FITSIO_H)
#include "fitsio.h"
#endif

#include "smCache.h"
#include "mbus.h"
#include "pxf.h"


extern int	use_mbus;		/* Message bus variables	*/
extern int	mb_tid, mb_fd;
extern int	verbose, console, procDebug;
extern int	save_fits, save_mbus;
extern int	noop;


/* These PRINT definitions are by default at /MNSN/soft_dev/inc/debug.h */

#define DPRINT(Level, Arg, Msg) \
 if(Arg >= Level) {fprintf(stderr, "\t*DBG* %s", Msg); fflush(stderr);}

#define DPRINTF(Level, Arg, Msg, Val) if(Arg >= Level) \
{   char __dbgMsg[256],__dbgMsg2[256];\
    sprintf(__dbgMsg, "\t*DBG* %s", Msg);\
    sprintf(__dbgMsg2, __dbgMsg, Val);\
    fputs (__dbgMsg2, stderr); \
    fflush(stderr);\
}




/*  PXFPROCESS -- Process an exposure given the ExpID.
*/
void
pxfProcess (smCache_t *smc, double expID)
{
    char   fname[250], pxfDIR[250], pxfFILENAME[250], resp[SZ_LINE];
    int    i, pcount, fs;
    int    pxfFLAG = 3;  /* indicates that DIR and FILENAME are defined	*/
    XLONG   istat;
    fitsfile *fd;

    smcPage_t *page;
#ifdef DO_TIMINGS
    time_t t1, t2;
#endif


    /*smcSetSeqNo (smc, 0);*/	/* Initialize sequence number		*/

    /* Get Filename and Directory from shared memory.
    */
#ifdef DO_TIMINGS
    t1 = time ((time_t)NULL);
#endif
    bzero (resp, SZ_LINE);
    bzero (fname, 250);
    bzero (pxfDIR, 250);
    bzero (pxfFILENAME, 250);
    strncpy (pxfDIR, (char *)smcGetDir(smc), 250);
    strncpy (pxfFILENAME, (char *)smcGetFRoot(smc), 250);
    strcpy (fname, pxfFILENAME);   		/* save root filename */
    
    if (use_mbus) {
	mbusSend (SUPERVISOR, ANY, MB_STATUS, "active");
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "Processing...");
    }

    /* Open the file */
    if (console)
    	fprintf (stderr, "Opening file '%s'\n", pxfFILENAME);

    if (save_fits) {
        pxfFLAG = 3;
        pxfFileOpen (&istat, resp, &expID, smc, &fd); 
        DPRINTF (10, procDebug, "%s\n", resp);
    }


  for (pcount=0; pcount < 2; pcount++) {
    for (i=0; i < smc->top; i++) {
	page = &smc->pdata[i];

        if (page->memKey == (key_t)NULL)
          continue;

        if (smcEqualExpID(page->expID,expID)) {

	    smcAttach (smc, page); 		/* Attach to the page. */

	    if (console) {
	 	fprintf (stderr, "Processing page %d, %.6lf (%.6lf)\n ",
    		    i, page->expID, expID);
	    }


	    switch (page->type) { 		/* Process the data.   */
	    case TY_VOID:
fprintf (stderr, "Processing page %d, type is TY_VOID\n",i);
	    case TY_DATA:
fprintf (stderr, "Processing page %d, type is TY_DATA\n",i);
		if (!noop && pcount == 1) {
		    if (save_fits){
fprintf (stderr, "Processing page %d, calling procFITSData\n",i);
	                procFITSData (page, fd);
                    }
		    else{
fprintf (stderr, "Processing page %d, calling procDCAData\n",i);
	                procDCAData (page);
		    }
		}
	        break;
	    case TY_META:
fprintf (stderr, "Processing page %d, type is TY_META\n",i);
		if (!noop && pcount == 0) {
		    if (save_fits){
fprintf (stderr, "Processing page %d, calling procFITSMetaData\n",i);
	                procFITSMetaData (page, fd);
	  	    }
		    else{
fprintf (stderr, "Processing page %d, calling procDCAMetaData\n",i);
	                procDCAMetaData (page);
		    }
		}
	        break;
	    default:
		fprintf (stderr, "Warning: Invalid page type: 0x%x\n", 
		    (int)page);
	        break;
	    }

	    smcDetach (smc, page, FALSE); 	/*  Detach from the page.  */

	} else if (console && verbose) {
	    fprintf (stderr, "Skipping page %d, %.6lf (%.6lf) ",
		i, page->expID, expID);
	     fprintf (stderr, "from=%s page=%d\n", smcGetColID(page),
	        smcGetExpPageNum(page));
	}
    }
  }


#ifdef DO_TIMINGS
    t2 = time ((time_t)NULL);
    fprintf (stderr, "Time [%d] pxfProcess()  expID=%.6lf\n", (t2-t1), expID);
#endif

    if (save_fits) {
        if (console)
	    fprintf (stderr, "Closing file '%s'\n", pxfFILENAME);

        if (fits_close_file ((fitsfile *) fd, &fs)) {
    	    DPRINTF (10, procDebug, "pxf: fits close failed, status = %d\n",fs);
        } else {
    	    DPRINTF (10, procDebug, "pxf: fits close succeeded, status = %d\n",
		fs);
        }
    }
}
