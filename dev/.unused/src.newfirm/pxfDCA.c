#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <pvm3.h>

#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_FITSIO_H)
#include "fitsio.h"
#endif


#include "smCache.h"
#include "mbus.h"
#include "pxfDCA.h"
#include "pxf.h"

#define SZ_VALSTR       80
#define SZ_CMDSTR       512
#define MAX_KWGROUPS    64
#define MAX_CARDS       512
#define MAX_IGNORED     10
#define MAX_HDRFILES    16
#define MAX_DIRFILES    512
#define HDRFILE_WAIT    3               /* wait for hdrfile creation */
#define MAX_WAIT        15              /* max hdrfile wait, seconds */

#ifdef SZ_FNAME
#undef SZ_FNAME
#endif
#define SZ_FNAME        256

#ifdef SZ_PATHNAME
#undef SZ_PATHNAME
#endif
#define SZ_PATHNAME     512

#define   MB_TIMEOUT    5		/* Message bus timeout (seconds). */


extern int console, verbose;
extern int procDebug;
extern int noop;

extern char *procPages;




static struct timeval mb_timeout;
#define SET_TIMEOUT(tm,sec)  ((tm)->tv_sec = (sec), (tm)->tv_usec = 0)


static char *procDCAMoveStr (char *instr, int len);

extern int   seqno, dca_tid;
extern int   errno;
extern char *getenv();



/**
 *  PROCDCAMETADATA -- Process a metadata page for the DCA.
 */
void
procDCAMetaData (smcPage_t *p)
{
    int   i, nkw, nsend, nkeyw, data_size, nmatch=0;
    char *cdata, *ip, *edata;
    smcSegment_t *seg = (smcSegment_t *) NULL; 
    mdConfig_t *mdCfg = smcGetMDConfig (p);
    fpConfig_t *fpCfg = smcGetFPConfig (p);
    double expID;
    char obsetID[80], kwdb[80], *keyw, *val, *comment;
    char *kmonMsg = (char *)NULL;

#ifdef DO_TIMINGS
    time_t	t1, t2;
#endif


#ifdef FAKE_READOUT
	return;
#endif


    seg = SEG_ADDR(p);
    data_size = seg->dsize;

    /* See whether we're supposed to process the page or not.
    */
    if ( (procPages && strstr(procPages, seg->colID) == NULL) ||
	 (p->type != TY_META) )
	     return;

#ifdef DO_TIMINGS
    t1 = time ((time_t)NULL);
#endif

    expID = smcGetExpID(p);
    strcpy (obsetID,smcGetObsetID(p));
    cdata = ip = (char *) smcGetPageData (p);
    edata = cdata + data_size;

    nkeyw = data_size / 128;		/* AV Pairs = 128 bytes/card	*/

    /* Generate the keyword database name.
    */
    bzero (kwdb, 80);
    if (seg->expPageNum == 0)
	sprintf (kwdb, "%s_Pre", seg->colID);
    else
	sprintf (kwdb, "%s_Post", seg->colID);

    if (console) {
        fprintf (stderr, "Proc META : 0x%x size=%d nkeyw=%d\n",
		(int) seg, data_size, nkeyw);
        fprintf (stderr, "          : ExpID=%.6lf ObsID=%s typ=%d\n",
	    expID, obsetID, p->type);
        fprintf (stderr, "          : mdCfg: type=%d  nfields=%d\n",
	    mdCfg->metaType, mdCfg->numFields);
        fprintf (stderr, "          : fpCfg: Start: %d,%d  Size: %d,%d\n", 
            fpCfg->xStart, fpCfg->yStart, fpCfg->xSize, fpCfg->ySize);
        fprintf (stderr, "          : page: colID='%s'  expPageNum=%d\n",
	    seg->colID, seg->expPageNum);
        fprintf (stderr, "          : seqno: %d  dca_tid: %d  kwdb: '%s'\n\n",
	    seg->seqno, dca_tid, kwdb);
    }

    ip = cdata;                           /* initialize                 */
    edata = (cdata + data_size);          /* find addr of end of data   */


    /* Initialize the keyword monitoring message.
    */
    if (NKeywords) {
        kmonMsg = calloc (1, NKeywords * SZ_LINE);
        sprintf (kmonMsg, "keywmon %.6lf ", expID);
	nmatch = 0;
    }

    for (nkw=0; nkw < nkeyw;) {

        /* Initialize the WriteHeaderData request for the keyword group.
        */
	seqno = seg->seqno;
	nsend = min((nkeyw-nkw), 16);
        pvm_initsend (PvmDataDefault);
        pvm_pkint (&seqno, 1, 1);
        pvm_pkstr (kwdb);
        pvm_pkint (&nsend, 1, 1);


        for (i=0; i < 16 && nkw < nkeyw; i++) {
	    keyw    = procDCAMoveStr (ip,      32);
	    val     = procDCAMoveStr (&ip[32], 32);
	    comment = procDCAMoveStr (&ip[64], 64);

            pvm_pkstr (keyw);		/* pack the strings		*/
            pvm_pkstr (val);
            pvm_pkstr ("S");
            pvm_pkstr (comment);

	    if (procDebug > 100 && nkw < 16)
    	        fprintf (stderr, "%2d: '%s' '%s' '%s'\n", nkw,keyw,val,comment);

	    free ((void *) keyw);
	    free ((void *) val);
	    free ((void *) comment);

	    ip += 128;
	    nkw++;
        }

	if (console && verbose)
	    fprintf (stderr, "Sending WriteHeaderData for %s (%d) (%d) (%d)\n",
    		kwdb, i, nkw, nsend);

        pvm_send (dca_tid, DCA_WriteHeaderData);

        /* Send Sync.
        pvm_initsend (PvmDataDefault);
        pvm_pkint (&seqno, 1, 1);
        pvm_send (dca_tid, DCA_Synchronize);
        mb_timeout.tv_sec = MB_TIMEOUT;
        mb_timeout.tv_usec = 0;
        if ((s=pvm_trecv (dca_tid, DCA_Synchronize, &mb_timeout)) <= 0) {
	    if (console) {
                fprintf (stderr, "pxf: %s", (s == 0) ? 
                     "timeout waiting for response from DCA\n" :
                     "failed to synchronize with DCA");
            }
        }
        */

    }
    if (console) fprintf (stderr, "DONE Processing META : sent=%d\n", nkw);


    /* Send the requested keywords to the Supervisor for display.
    */
    if (NKeywords && nmatch)
	mbusSend (SUPERVISOR, ANY, MB_SET, kmonMsg);

    if (kmonMsg) free (kmonMsg);	/* free space			*/

#ifdef DO_TIMINGS
    t2 = time ((time_t)NULL);
    fprintf (stderr, "Time [%d] procDCAMetaData()\n", (t2-t1));
#endif
}


/**
 *  Copy a string from an AVP buffer to a value, stripping leading and 
 *  trailing whitespace in the process.  Both 'instr' and 'outstr' are 
 *  assumed to be at least 'len' chars long.
 */

#define SZ_AVP_BUF	128

static char *
procDCAMoveStr (char *instr, int len)
{
    char *ip, buf[SZ_AVP_BUF];
    int i = len;

    
    bzero (buf, SZ_AVP_BUF);			/* zero buffer		*/
    bcopy (instr, buf, len);			/* move the string 	*/

						/* trim trailing space	*/
    for (i=strlen(buf)-1; (buf[i] == '\'' || isspace(buf[i])) && i > 0; )
	buf[i--] = '\0';					
    if (buf[i] == '\'' || isspace(buf[i]))
	buf[i--] = '\0';					
						/* skip leading space	*/
    for (ip=&buf[0], i=0; (*ip == '\'' || isspace(*ip)) && i++ < len; ip++)
	;					

    return (strdup(ip));			/* return final string 	*/
}


/**
 *  PROCDCADATA -- Process a data page for the DCA.
 */
void
procDCAData (smcPage_t *p)
{
    int    i, j, data_size, nx, ny, nchunks=128, datalen;
    int   *idata, *ip, swapped=0, nstreams=1, s, nbytes = 0;
    smcSegment_t *seg = (smcSegment_t *) NULL; 
    mdConfig_t *mdCfg = smcGetMDConfig (p);
    fpConfig_t *fpCfg = smcGetFPConfig (p);
    double expID;
    char obsetID[80];
    DCA_Stream st;
#ifdef DO_TIMINGS
    time_t  t1, t2;
#endif


#ifdef FAKE_READOUT
	return;
#endif


    seg = SEG_ADDR(p);
    data_size = seg->dsize;

    /* See whether we're supposed to process the page or not.
    */
    if ( (procPages && strstr (procPages, seg->colID) == NULL) ||
	 (p->type == TY_META) )
	     return;

#ifdef DO_TIMINGS
    t1 = time ((time_t) NULL);
#endif
    expID = smcGetExpID (p);
    strcpy (obsetID, smcGetObsetID (p));
    idata = ip = (int *) smcGetPageData (p);

    if (console) {
        fprintf (stderr, "Proc DATA : 0x%x size=%d\n", (int) seg, data_size);
        fprintf (stderr, "          : ExpID=%.6lf ObsID=%s seqno=%d\n",
	    expID, obsetID, seqno);
        fprintf (stderr, "          : mdCfg: type=%d  nfields=%d\n",
	    mdCfg->metaType, mdCfg->numFields);
        fprintf (stderr, "          : fpCfg: Start: %d,%d  Size: %d,%d\n", 
            fpCfg->xStart, fpCfg->yStart, fpCfg->xSize, fpCfg->ySize);
        fprintf (stderr, "          : page: colID='%s'  expPageNum=%d\n",
	    seg->colID, seg->expPageNum);
        fprintf (stderr, "          : seqno: %d dca_tid; %d\n", 
	    seg->seqno, dca_tid);
    }

    nx 		= CCD_W;		      /* CCD width		      */
    ny 		= CCD_H;		      /* CCD height		      */
    ip 		= idata;                      /* initialize            	      */
    datalen 	= CCD_W * CCD_BPP * CCD_ROWS; /* 32 rows of int => 64 chunks  */
    swapped 	= 1;
    nstreams 	= 1;

    st.offset 	= 0;
    st.npix 	= datalen / CCD_BPP;
    st.stride 	= 1;
    st.xydir 	= 0;
    st.x0 	= 0;
    st.y0 	= 0;


    /*  Set up the detector quadrants to go into the image extensions using
    **  the usual convention of numbering from the LLC.  The output image
    **  will contain:
    */
    for (i=0; i < NCCDS; i++) {
fprintf (stderr, "[%d] colID='%s'  pageNum = %d\n",i, seg->colID,seg->expPageNum);
	if (strncmp (seg->colID, rasInfo[i].colID, 4) == 0 && 
	    seg->expPageNum == rasInfo[i].pageNum) {
		st.destimage = rasInfo[i].destNum;
fprintf (stderr, "[%d] destNum = %d\n", i, st.destimage);
		break;
	}
    }


    /*  Send the data.
    */
    for (j=0; j < nchunks; j++) {

        /* Initialize the WriteHeaderData request for the keyword group.
        */
        seqno = seg->seqno;
        pvm_initsend (PvmDataDefault);
        pvm_pkint (&seqno, 1, 1);
        pvm_pkint (&datalen, 1, 1);
        pvm_pkint (&swapped, 1, 1);
        pvm_pkint (&nstreams, 1, 1);

        st.y0 = (j * 32);	
	st.xstep = st.ystep = 1;

	pvm_pkint ((int *)&st, sizeof(st)/sizeof(int), 1);
	pvm_pkbyte ((char *)ip, datalen, 1);
	pvm_send (dca_tid, DCA_WritePixelData);

	if (console && verbose)
	    fprintf (stderr, "%d: (x,y) = (%d,%d) len=%d\n", j,
		st.x0, st.y0, datalen);

	if ((j % 16) == 0) {
	    pvm_initsend (PvmDataDefault);
	    pvm_pkint (&seqno, 1, 1);
	    pvm_send (dca_tid, DCA_Synchronize);

	    mb_timeout.tv_sec = MB_TIMEOUT;
	    mb_timeout.tv_usec = 0;
            if ((s=pvm_trecv (dca_tid, DCA_Synchronize, &mb_timeout)) <= 0) {
		if (console)
                    fprintf (stderr, "pxf: %s", (s == 0) ? 
                        "timeout waiting for response from DCA\n" :
                        "failed to synchronize with DCA");
            }
	}
	ip += st.npix;
	nbytes += datalen;
    }

    /* Send final Sync.
    */
    pvm_initsend (PvmDataDefault);
    pvm_pkint (&seqno, 1, 1);
    pvm_send (dca_tid, DCA_Synchronize);
    mb_timeout.tv_sec = MB_TIMEOUT;
    mb_timeout.tv_usec = 0;
    if ((s=pvm_trecv (dca_tid, DCA_Synchronize, &mb_timeout)) <= 0) {
	if (console) {
            fprintf (stderr, "pxf: %s", (s == 0) ? 
                 "timeout waiting for response from DCA\n" :
                 "failed to synchronize with DCA");
        }
    }

    if (console) {
	fprintf (stderr, ": nchunks=%d  %d bytes  %d pixels\n",
		nchunks, nbytes, nbytes / 4);
    }

#ifdef DO_TIMINGS
    t2 = time ((time_t)NULL);
    fprintf (stderr, "Time [%d] procDCAData()\n", (t2-t1));
#endif
}


/**
 *  INKEYWLIST -- See whether the given keyword and database are in the 
 *  monitor list.  The list of keywords isn't expected to be long so we
 *  skip any optimizations.
 */
int
inKeywList (char *kwdb, char *keyw)
{
    int  i;
    char  kw[SZ_LINE], *k;


    for (i=0; i < NKeywords; i++) {
	if (strncasecmp (keywList[i], "any", 3) == 0) {
	    /* If the keyword can come from any database, match only on
	    ** the keyword itself.
	    */
            k = keywList[i];
            if (strcasecmp (&k[4], keyw) == 0)
	        return (1);

	} else {
	    /* Otherwise, we were given the keyword list with a specific
	    ** database name in the form "db.keyw" so construct a string
	    ** for the match.
	    */
 	    memset (kw, 0, SZ_LINE);
	    sprintf (kw, "%s.%s", kwdb, keyw);
            if (strcasecmp (keywList[i], kw) == 0)
	        return (1);
	}
    }

    return (0);
}
