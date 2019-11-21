#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

#include "cdl.h"
#include "mbus.h"
#include "smCache.h"
#include "smcmgr.h"


/*  SMCPROCESS -- Process a series of SMC pages given the ExpID.  At the
**  moment this consists only of rectifying the orientation of the image
**  pixels and sending the image for display.  Later, we add more monitorin
**  capabilities.
*/


time_t	t1, t2;
int     nthread = 0;
int     disp_done = 0;

extern 	int trim_ref;			/* trim reference pix flag 	*/
extern  int rotate_enable;
extern  int disp_frame, disp_stdimg, disp_enable;
extern  int verbose, debug;
extern	int use_mbus, use_disp, use_threads, console, verbose;
extern	int use_pxf, seqno;
extern  char *procPages;
extern  char *imtdev;
extern  CDLPtr  cdl;


#define BITPIX			32



    
/*  smcProcAll -- Process all remaining exposures.
*/
int
smcProcAll (smCache_t *smc)
{
    return (OK);
}


/*  smcProcNext -- Process the next most recent exposure.
*/
int
smcProcNext (smCache_t *smc)
{
    smcPage_t *page;
    double expID;

    if ((page = smcNextByExpID(smc, expID))) {	/* Find the next page.	*/
        smcAttach (smc, page);
        expID = smcGetExpID (page);
        smcDetach (smc, page, FALSE);

        smcProcess (smc, expID);		/* Do it.		*/
    }

    return (OK);
}


/*  smcProcess -- Process all the rasters of an exposure given an expID.
*/
void
smcProcess (smCache_t *smc, double expID)
{
    int	         i, rc;
    smcPage_t    *page;
    rtdArg	 args[128];

    pthread_t      tid;
    pthread_attr_t attr;

    /* Initialize the service processing thread attribute.
    */
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);



    if (console || verbose)
	fprintf (stderr, "Processing ExpID: %lf\n", expID);

    /* Display the SMC data pages.  We'll rectify the rasters at this 
    ** so we can 1) get the image display as quickly as possible and 2)
    ** only require we attach to the page once during the processing.
    */
    if (disp_enable) {
	if (!use_threads) {
            if (imtdev == (char *) NULL)
                imtdev = (char *) getenv ("IMTDEV");
            if (! (cdl = cdl_open (imtdev)) ) {
                fprintf (stderr, "ERROR: cannot connect to display server.");
                use_disp = disp_enable = 0;
            }
            cdl_setFBConfig (cdl, disp_stdimg);     /* imt4400 by default */
            cdl_setFrame (cdl, disp_frame);
            cdl_clearFrame (cdl);
	}

        if (use_mbus) {
            mbusSend (SUPERVISOR, ANY, MB_STATUS, "active");
            mbusSend (SUPERVISOR, ANY, MB_SET, "rtdStat clear");
        }
    }

    /*  Find the data pages to process.
    */
    nthread = 0;
    for (i=0; i < smc->top; i++) {
        if (console || verbose)
	    fprintf (stderr, "Processing page %d of %d\n", i+1, smc->top);

	page = &smc->pdata[i];

        if (page->memKey == (key_t)NULL)
	  continue;

        /* See whether we're supposed to process the page or not.
        */
        if (procPages && strstr(procPages, page->colID) == NULL)
            continue;

	if (smcEqualExpID (page->expID, expID)) {

            smcAttach (smc, page);

	    smcSetPageSeqNo (page, seqno);
            if (page->type == TY_DATA || page->type == TY_VOID) {

   	        if (console || verbose)
        	    fprintf (stderr, "Rectifying data page....\n");
                smcRectifyPage (page);

	        if (disp_enable) {
   	            if (console || verbose)
        	        fprintf (stderr, "Displaying pixels....\n");

		    if (use_threads) {
			args[nthread].nthread = nthread;
			args[nthread].smc     = smc;
			args[nthread].page    = &smc->pdata[i];

        		if ((rc = pthread_create (&tid, &attr,
                	    (void *)rtdDisplayThread,(void *)&args[nthread]))) {
                    		fprintf (stderr,
                        	    "ERROR: pthread_create() fails, code: %d\n",				    rc);
                    		continue;
        		}
        		nthread++;
		    } else
                        rtdDisplayPixels ((void *)cdl, page);
	        }

   	        if (console || verbose)
        	    fprintf (stderr, "done.\n");

	    } else if (page->type == TY_META) {
		smcSegment_t *seg = SEG_ADDR(page);
		char kwdb[80], *avp, *typ, *ip, *op, val[SZ_FNAME];
		char msg[SZ_LINE], line[SZ_LINE];
		int  i, nkeyw;
	 	extern char *imtypeDB, *imtypeKeyw;
		

    		bzero (kwdb, 80); 	/* Generate the keyword db name       */
		sprintf (kwdb, 
		    ((seg->expPageNum == 0) ? "%s_Pre" :"%s_Post"), seg->colID);

		if (strcasecmp (imtypeDB, kwdb) == 0) {
		    avp = (char *) smcGetPageData (page);
		    nkeyw = seg->dsize / 128;	    /* 128 bytes/card	*/

		    bzero (val, SZ_FNAME);
		    bzero (msg, SZ_LINE);

		    ip = avp;
		    for (i=0; i < nkeyw; i++) {
		  	strncpy (line, ip, 128);
		        if ((typ = strstr (line, imtypeKeyw))) {
			    /* Found the image type keyword, skip to value.
			    */
			    ip = &line[32];
			    while (*ip && isspace(*ip)) ip++;
			    for (op=val; *ip && !isspace(*ip); )
			        *op++ = *ip++;
			    *op++ = '\0';
			    break;
			}
			ip += 128;
		    }

		    /* Tell the Supervisor what kind of image we have.
		    */
		    if (val[0])
			sprintf (msg, "imagetype %.6lf %s\n", expID, val);
		    else
			sprintf (msg, "imagetype %.6lf image\n", expID);
		    mbusSend (SUPERVISOR, ANY, MB_SET, msg);
		}
	    }

	    if (!use_threads) {
	        if (use_pxf == 0) {
		    smcMutexOn ();
                    smcDetach (smc, page, TRUE);
	            smcMutexOff ();
	        } else if (!use_threads)
                    smcDetach (smc, page, FALSE);
	    }
	}
    }
    sleep (5);		/* give display a chance to finish */

    /*  Notify the Supervisor we're done with this image.
    */
    if (use_mbus) {
	char buf[128];

	memset (buf, 0, 128);
	sprintf (buf, "process smc done %.6lf", expID);
	if (console || verbose )
	    fprintf (stderr, "SMC Processing done ExpID: %.6lf\n", expID);
        mbusSend (SUPERVISOR, ANY, MB_SET, buf);
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "inactive");
    }

    if (disp_enable && !use_threads)
	if (cdl) cdl_close (cdl);

    mbusSend (SUPERVISOR, ANY, MB_STATUS, "inactive");
}


/* Control access to the pages associated with the given exposure ID.
*/
void
smcLockExpID (smCache_t *smc, double expID, char *procPages)
{
    int   i;
    smcPage_t *p = (smcPage_t *) NULL;

    for (i=0; i < smc->top; i++) {
        p = &smc->pdata[i]; 

        if (procPages && strstr(procPages, p->colID) == NULL)
            continue;

        if (smcEqualExpID(p->expID,expID))/* look for a match     */
            smcLock (p);
    }
}


void
smcUnlockExpID (smCache_t *smc, double expID, char *procPages)
{
    int   i;
    smcPage_t *p = (smcPage_t *) NULL;

    for (i=0; i < smc->top; i++) {
        p = &smc->pdata[i];

        if (procPages && strstr(procPages, p->colID) == NULL)
            continue;

        if (smcEqualExpID(p->expID,expID))/* look for a match             */
            smcUnlock (p);
    }
}   


void
smcDelExpID (smCache_t *smc, double expID, char *procPages)
{   
    int   i, np = 0;
    smcPage_t *p = (smcPage_t *) NULL;


    if (smc->debug) printf ("Deleting ExpID: %.6lf\n", expID);
    
    for (i=0; i < smc->top; i++) {
        p = &smc->pdata[i];

        if (procPages && strstr(procPages, p->colID) == NULL)
            continue;

        if (smcEqualExpID(p->expID,expID)) {/* look for a match           */
	    smcMutexOn ();
            smcUnlock (p);
            smcDetach (smc, p, TRUE);
	    smcMutexOff ();
            np++;
        }
    }

    if (smc->debug) 
	fprintf (stderr, "Removed %d pages for ExpID: %.6lf\n", np, expID);
}
