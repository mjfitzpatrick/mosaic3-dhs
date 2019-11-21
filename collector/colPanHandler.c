/**
 *
 */

#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

#include "dcaDhs.h"
#include "mbus.h"


#include <stdio.h>		/* for printf() and fprintf()           */
#include <sys/types.h>		/* for SO_REUSEADDR                     */
#include <sys/socket.h>		/* for socket(), bind(), and connect()  */
#include <arpa/inet.h>		/* for sockaddr_in and inet_ntoa()      */
#include <stdlib.h>		/* for atoi() and exit()                */
#include <string.h>		/* for memset()                         */
#include <unistd.h>		/* for close()                          */

#include "smCache.h"
#include "imageCfg.h"


#define SZMSG    	120	/* Size of a Supervisor message         */
#define RCVBUFSIZE 	1024	/* Size of receive buffer               */

#define COL_DEBUG  (getenv("COL_DEBUG")!=NULL||access("/tmp/COL_DEBUG",F_OK)==0)


extern int procDebug;			/* main process debug flag      */
extern int console;			/* display console messages?    */
extern int use_mbus;			/* Use the msgbus for status?   */
extern int mb_tid;			/* message bus tid              */

					/* Function prototypes          */
char *dcaTyp2Str (int type);
int   colPanHandler (int fd, void *client_data);
void  col_disconnectClient (int source);

extern smcPage_t *smcGetPage();
extern char colID[];
extern int expPageNum, sim_mode;

static int connected = 0;

#define	MAX_MESSAGE_SZ	262144			/* 2048 AVP 		*/



/*************************************************************************
**  COLPANHANDLER -- Handle input on the (PAN) client I/O channel.
*/
int
colPanHandler (int socket, void *client_data) 
{
    char   buffer[SZMSG], line[180], obSetID[80];
    char   dirname[SZMSG], filename[SZMSG], msg[SZMSG];
    char  *buff, *hdr;
    int    recvMsgSize, blksize, mtype, size;
    int    totRowsPDet, totColsPDet, bytesPxl, imSize;
    int    szt, attach = FALSE, lock = FALSE, arrayNum = 0;
    int   *idata;
    XLONG  istat, who, transferTotal = 0;;
    double expID = 0.0;
    extern smCache_t *smc;

    struct msgHeader *msgh;	/* msgHeader msgh               */
    fpConfig_t fpCfg;		/* focal plane config struct    */
    mdConfig_t mdCfg;		/* configuration of meta data   */
    smcPage_t *page;		/* shared memory page           */



    /* Receives message code from the PAN Client via the DHSlibrary.
     */
    if (console)
	fprintf (stderr,  "Waiting for message: socket=%d\n", socket);
    if ((size = dcaRecv(socket, &buffer[0], SZMSG)) != SZMSG) {
	if (size == EOF) {
	    if (console)
	        fprintf (stderr, "colHandleClient: EOF seen on %d\n", socket);
	    col_disconnectClient (socket);
	    return (ERR);

	} else {
	    if (console)
	        fprintf (stderr, "colHandleClient: Error receiving message\n");
	    return (ERR);
	}
    }


    recvMsgSize = SZMSG;
    while (recvMsgSize > 0) {

	msgh = (struct msgHeader *) buffer;

	mtype = msgh->type;
	who = msgh->whoami;

	if (COL_DEBUG || procDebug > 99) {
	    fprintf (stderr,"colPanHandler:  mtype=%d  %s\n",
		mtype, dcaTyp2Str(mtype));
	}

	if (mtype & DCA_SET) {
	    if (mtype & DCA_FP_CONFIG) {
		size = sizeof(fpCfg);
		if (dcaRecv(socket, (char *) &fpCfg, size) != size)
		    fprintf (stderr, "DCA_FP_CONFIG error \n");

		DPRINTF (30, procDebug,
		    "DCA_SET | DCA_FP_CONFIG received: %d bytes\n", size);
		sprintf(line, "fpCfg.(x,y)Start: (%d,%d)\n",
			(int) fpCfg.xStart, (int) fpCfg.yStart);
		DPRINT (20, procDebug, line);
		sprintf(line, "fpCfg.(x,y)Size: (%d,%d)\n",
			(int) fpCfg.xSize, (int) fpCfg.ySize);
		DPRINT (20, procDebug, line);
		sprintf(line, "ExpID: %.6lf\n", msgh->expnum);
		DPRINT (20, procDebug, line);
	    }
	} else if (mtype & DCA_NOOP) {
	    /* A NOOP is a request for an ACK.
	     */
	    msgh->type = DCA_ACK;
	    msgh->whoami = DCA_COLLECTOR;
	    if (dcaSend(socket, (char *) msgh, SZMSG) != SZMSG)
		fprintf (stderr, "DCA_NOOP error\n");

	} else if (mtype & DCA_DIRECTORY) {

	    /*  Set the current directory path.
	     */
	    szt = 256;
	    if (dcaRecv(socket, dirname, szt) != szt)
		fprintf (stderr, "DCA_DIRECTORY error\n");

	    sprintf(line, "DCA_DIRECTORY: '%s'\n", dirname);
	    DPRINT (20, procDebug, line);

	    smcSetDir (smc, dirname);

	} else if (mtype & DCA_FNAME) {

	    /*  Set the current filename root.
	     */
	    szt = 256;
	    if (dcaRecv(socket, filename, szt) != szt)
		fprintf (stderr, "DCA_FNAME error\n");

	    sprintf(line, "DCA_FNAME: '%s'\n", filename);
	    DPRINT (20, procDebug, line);

	    smcSetFRoot(smc, filename);

	} else if (mtype & DCA_EXP_OBSID) {

	    /*  Set the exposure ObsID.
	     */
	    expID = msgh->expnum;
	    memcpy(obSetID, msgh->obset, 80);

	    DPRINT (30, procDebug, "DCA_EXP_OBSID received\n");
	    DPRINTF (10, procDebug, "ExpID: %.6lf\n", expID);
	    DPRINTF (10, procDebug, "ObsSetID: %sf\n", obSetID);

	} else if (mtype & DCA_META) {

	    /*  Receives meta data. We need to know how many bytes to read
	    **  first.
	    */
	    size = sizeof(blksize);
	    size = SZMSG;
	    DPRINT (10, procDebug, "Handling DCA_META \n");
	    if (dcaRecv(socket, buffer, size) != size) {
		fprintf (stderr, "DCA_META blksize error\n");
		DPRINT (10, procDebug, "DCA_META blksize error\n");
	    }
	    memmove ((char *) &blksize, buffer, 4);

	    buff = calloc (1, blksize);

	        DPRINTF (30, procDebug, "   DCA_META msg size: %d\n", size);
	        DPRINTF (30, procDebug, "   DCA_META blk size: %d\n", blksize);

	    /* Read meta data.
	     */
            mbusSend (SUPERVISOR, ANY, MB_STATUS, "Reading metadata...");

	    if (dcaRecv(socket, &buff[0], blksize) != blksize) {
		DPRINT (10, procDebug, "DCA_META buff error\n");
	    } else {
		DPRINTF (10, procDebug, "DCA_META read %d bytes\n", blksize);
	    }

	    size = sizeof(mdCfg);
	    DPRINTF (30, procDebug, "   DCA_META mdCfg size: %d\n", size);
	    if (dcaRecv(socket, (char *) &mdCfg, size) != size) {
		DPRINT (10, procDebug, "DCA_META bufffer error\n");
	    } else {
		DPRINTF (10, procDebug, "DCA_META read %d bytes\n", size);
	    }

	    /* Update the Supervisor with the transfer.  */
	    transferTotal += blksize;

	    /*  Load the SMC page with the metadata.
	     */
	    smcMutexOn ();

	    page = smcGetPage(smc, TY_META, (XLONG)blksize, TRUE, TRUE);
	    if (page == (smcPage_t *) NULL) {
		fprintf (stderr, "Error getting SMC metadata page.\n");
		return (OK);
	    }
	    smcSetMDConfig (page, &mdCfg);
	    smcSetFPConfig (page, &fpCfg);
	    smcSetExpID (page, expID);
	    smcSetObsetID (page, obSetID);
	    smcSetColID (page, colID);
	    smcSetExpPageNum (page, expPageNum++);

	    hdr = (char *) smcGetPageData (page);

	    memcpy(hdr, buff, blksize);

	    smcUnlock (page);
	    smcDetach (smc, page, FALSE);

	    smcMutexOff ();

	    if (buff) free (buff);


	} else if (mtype & DCA_PIXEL) {

	    /* Read focal plane configuration first.
	     */
	    size = sizeof(fpCfg);
	    if (dcaRecv(socket, (char *) &fpCfg, size) != size)
		fprintf (stderr, "DCA_PIXEL error reading fpCfg\n");
	    else
	        DPRINT (40, procDebug, "DCA_PIXEL: DCA_FP_CONFIG received\n");

	    totRowsPDet = (int) fpCfg.ySize;
	    totColsPDet = (int) fpCfg.xSize;

	    sprintf(line,
		"DCA_PIXEL:  %d x %d   xs=%d ys=%d\n",
  		fpCfg.xSize, fpCfg.ySize, fpCfg.xStart, fpCfg.yStart); 
	    DPRINT (10, procDebug, line);

	    bytesPxl = 4;
	    imSize = totRowsPDet * totColsPDet * bytesPxl;
	    transferTotal += imSize;

	    sprintf(line,
		"DCA_PIXEL: totRowsPDet: %d, totColsPDet: %d, imsize: %d\n",
		totRowsPDet, totColsPDet, imSize);
	    DPRINT (10, procDebug, line);

	    sprintf(line, "DCA_PIXEL: array %d, original xStart: %d, yStart: %d\n",
		arrayNum, fpCfg.xStart, fpCfg.yStart);
	    DPRINT (10, procDebug, line);

	    /* Read pixel data directly into the shared memory page.
	     */
            mbusSend (SUPERVISOR, ANY, MB_STATUS, "Reading Pixel data...");

	    /* Update the Supervisor with the binning value.
	     */
	    if (fpCfg.xSize != ccd_xsize && fpCfg.ySize != ccd_ysize) {
		char  msg[SZMSG];

		memset (msg, 0, SZMSG);
	    	sprintf (msg, "nbin %d", (nbin = (ccd_ysize / fpCfg.ySize)));
            	mbusSend (SUPERVISOR, ANY, MB_SET, msg);
	    } else {
            	nbin = 1;
            	mbusSend (SUPERVISOR, ANY, MB_SET, "nbin 1");
	    }
fprintf (stderr, "SETTING BIN FACTOR = %d\n", nbin);


	    /*  Get an SMC page for the data.
	     */
	    smcMutexOn ();

	    page = smcGetPage (smc, TY_DATA, (XLONG)imSize, (attach=TRUE),
		(lock=TRUE));

#if 0
	    DPRINT (40, procDebug, "DCA_PIXEL: Re-settting x,y Start values\n");

	    switch (arrayNum) {
	    case 0:
	    	fpCfg.xStart = 0;
	    	fpCfg.yStart = 0;
		break;
	    case 1:
	    	fpCfg.xStart = fpCfg.xSize * nbin;
	    	fpCfg.yStart = 0;
		break;
	    case 2:
	    	fpCfg.xStart = 0;
	    	fpCfg.yStart = fpCfg.ySize * nbin; 
		break;
	    case 3:
	    	fpCfg.xStart = fpCfg.xSize * nbin;
	    	fpCfg.yStart = fpCfg.ySize * nbin; 
		break;
	    default:
	    	fpCfg.xStart = 0;
	    	fpCfg.yStart = 0;
	        DPRINT (40, procDebug, "DCA_PIXEL: ERROR arrayNum exceeds expected maximum of 3\n");
		break;
	    }

	    sprintf(line, "DCA_PIXEL: array %d, final xStart: %d, yStart: %d\n",
		arrayNum, fpCfg.xStart, fpCfg.yStart);
	    DPRINT (10, procDebug, line);
#endif
	    sprintf(line, "DCA_PIXEL: array %d,  xStart: %d, yStart: %d\n",
		arrayNum, fpCfg.xStart, fpCfg.yStart);
	    DPRINT (10, procDebug, line);

	    arrayNum++;

	    smcSetExpID (page, expID);
	    smcSetFPConfig (page, &fpCfg);
	    smcSetObsetID (page, obSetID);
	    smcSetColID (page, colID);
	    smcSetExpPageNum (page, expPageNum++);

	    idata = (int *) smcGetPageData (page);
	    if ((istat = dcaRecv(socket, (char *) idata, imSize)) != imSize)
		fprintf (stderr, "Error reading image size(dcaRecv) (%d): %d\n",
		       imSize, istat);

	    smcUnlock (page);
	    smcDetach (smc, page, FALSE);

	    smcMutexOff ();

	    DPRINTF (30, procDebug, "Read %d bytes\n", istat);
	    DPRINT (30, procDebug, "*** DONE DCA_PIXEL.\n");


	} else if (mtype & DCA_INIT) {
		;

	} else if (mtype & DCA_OPEN) {
	    if (mtype & DCA_CONNECT) {
		;			/* no-op */
	        DPRINT (30, procDebug, "*** DCA_OPEN CONNECT.\n");

	    } else if (mtype & DCA_EXP) {
	        /* Highlight the process in the Supervisor GUI.  */
	        DPRINT (30, procDebug, "*** DCA_OPEN EXPOSURE.\n");
                mbusSend (SUPERVISOR, ANY, MB_STATUS, "active");

	        /* Initialize the transfer total.  */
		transferTotal = 0;

	        /* Initialize the exposure page number. */
		expPageNum = 0;
	    }

	} else if (mtype & DCA_CLOSE) {

	    arrayNum = 0;
	    if (mtype & DCA_CONNECT) {
	        extern int tempCollector;

	        DPRINT (30, procDebug, "*** DCA_CLOSE CONNECT.\n\n\n");
        	mbusSend (SUPERVISOR, ANY, MB_SET, "connected 0");
                mbusSend (SUPERVISOR, ANY, MB_STATUS, "Closing connection...");

		mbusRemoveInputHandler (socket);
	        close (socket);
	        connected = 0;

	        /*  If we're just a temporary collector, disconnect from the 
	        **  message bus, close the SMC and exit.
	        */
	        if (tempCollector) {
		    int  mytid = mbAppGet(APP_TID);

	            smc = smcClose (smc, FALSE); 
		    mbusDisconnect (mytid);
		    exit (0);

	        } else
	            return (OK);

	    } else if (mtype & DCA_EXP) {

	        DPRINT (30, procDebug, "*** DCA_CLOSE EXPOSURE.\n\n\n");
        	mbusSend (SUPERVISOR, ANY, MB_STATUS, "inactive");
        	mbusSend (SUPERVISOR, ANY, MB_STATUS, "Waiting for input...");

	    	/* Update the Supervisor with the transfer stats.  */
		memset (msg, 0, SZMSG);
	    	sprintf (msg, "transfer %d", transferTotal);
            	mbusSend (SUPERVISOR, ANY, MB_SET, msg);

    		return (OK);
	    }

	} else {
	    msgh->type = DCA_FAIL;
	    msgh->whoami = DCA_COLLECTOR;
	}


	recvMsgSize = dcaRecv (socket, &buffer[0], SZMSG);
	/* break; */
    }				/* WHILE */

    return (OK);
}




char *
dcaTyp2Str (int type)
{
    char buf[128];

    bzero (buf, 128);

    if (type & DCA_OPEN)       strcat(buf, "DCA_OPEN:");
    if (type & DCA_CLOSE)      strcat(buf, "DCA_CLOSE:");
    if (type & DCA_READ)       strcat(buf, "DCA_READ:");
    if (type & DCA_WRITE)      strcat(buf, "DCA_WRITE:");
    if (type & DCA_INIT)       strcat(buf, "DCA_INIT:");
    if (type & DCA_FAIL)       strcat(buf, "DCA_FAIL:");
    if (type & DCA_NOOP)       strcat(buf, "DCA_NOOP:");
    if (type & DCA_ALIVE)      strcat(buf, "DCA_ALIVE:");
    if (type & DCA_SET)        strcat(buf, "DCA_SET:");
    if (type & DCA_GET)        strcat(buf, "DCA_GET:");
    if (type & DCA_SYS)        strcat(buf, "DCA_SYS:");
    if (type & DCA_CONNECT)    strcat(buf, "DCA_CONNECT:");
    if (type & DCA_CON)        strcat(buf, "DCA_CON:");
    if (type & DCA_EXP)        strcat(buf, "DCA_EXP:");
    if (type & DCA_META)       strcat(buf, "DCA_META:");
    if (type & DCA_STICKY)     strcat(buf, "DCA_STICKY:");
    if (type & DCA_PIXEL)      strcat(buf, "DCA_PIXEL:");
    if (type & DCA_OBS_CONFIG) strcat(buf, "DCA_OBS_CONFIG:");
    if (type & DCA_FP_CONFIG)  strcat(buf, "DCA_FP_CONFIG:");
    if (type & DCA_MD_CONFIG)  strcat(buf, "DCA_MD_CONFIG:");
    if (type & DCA_EXPID)      strcat(buf, "DCA_EXPID:");
    if (type & DCA_OBSID)      strcat(buf, "DCA_OBSID:");
    if (type & DCA_EXP_OBSID)  strcat(buf, "DCA_EXP_OBSID:");
    if (type & DCA_DIRECTORY)  strcat(buf, "DCA_DIRECTORY:");
    if (type & DCA_FNAME)      strcat(buf, "DCA_FNAME:");

    return (strdup (buf));
}
