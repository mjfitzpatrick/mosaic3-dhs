#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

#include "instrument.h"
#include "dcaDhs.h"
#include "mbus.h"
#include "panHdr.h"

#include <time.h>
#include <sys/time.h>

#define  abs(a)		((a)<0?(-a):(a))

#ifndef OK
#define OK 	1
#endif
#ifndef IamPan
#define IamPan 	1
#endif
#ifndef IamNOCS
#define IamNOCS 2
#endif

#define MAX_KEYWORDS	4096
#define LEN_AVPAIR	128


struct timeval tv;
struct timezone tz;

int sim 		= 0;
int bin 		= 1;
int loop 		= 1;
int delay 		= 0;
int interactive 	= 0;
int npans 		= 1;
int verbose = 0;

int    	xs[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	ys[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int	ncols = 0,  ccd_xsize = 2148,
	nrows = 0,  ccd_ysize = 4096;
int    	totRowsPDet, totColsPDet, imSize, bytesPxl;

char 	*pTime(char *inStr);

static char *trim (char *buf);
static void  dhsError (char *proc, char *resp);


/**
 *  Program main.
 */
int
main(int argc, char *argv[])
{
    int    i, j, k, mytid, PanOffset=0, use_nocs=1;
    int    nelem=0, nkeys=0, from_tid, to_tid, subject;
    int    expnum, nimages, blkSize;
    int   *p, *imAddr, pv, bug=0;

    char   resp[SZ_LINE], mdBuf[(MAX_KEYWORDS * LEN_AVPAIR)];
    char  *op, *obsID = "TestID";
    char   keyw[32], val[32], comment[64];
    char  *host, *msg, *group = "Pan";

    double  expID;
    XLONG   istat = 0;

    dhsHandle  dhsID;
    mdConfig_t mdCfg;
    fpConfig_t fpCfg;



    /* Process the command-line arguments.
     */
    for (i = 1; i < argc; i++) {
	if (strncmp(argv[i], "-help", 5) == 0) {
            fprintf (stderr, "ERROR No Help Available\n");
	    exit(0);;

	} else if (strncmp(argv[i], "-delay", 5) == 0) {
	    delay = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-A", 2) == 0) {
	    PanOffset = 0;
	    group = "PanA";

	} else if (strncmp(argv[i], "-B", 2) == 0) {
	    npans = 2;
	    PanOffset = 1;
	    group = "PanB";

	} else if (strncmp(argv[i], "-bin", 4) == 0) {
	    bin++;

	} else if (strncmp(argv[i], "-bug", 4) == 0) {
	    bug++;

	} else if (strncmp(argv[i], "-no_nocs", 2) == 0) {
	    use_nocs = 0;

        } else if (strncmp(argv[i], "-host", 5) == 0) {
            dcaInitDCAHost ();
            dcaSetSimHost (argv[++i], 1);

        } else if (strncmp(argv[i], "-sim", 4) == 0) {
	    /* dcaSetSimMode (1); */
	    ;

	} else if (strncmp(argv[i], "-debug", 5) == 0) {
	    procDebug = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-group", 5) == 0) {
	    group = argv[++i];

	} else if (strncmp(argv[i], "-interactive", 5) == 0) {
	    interactive++;
	    loop += 999;

	} else if (strncmp(argv[i], "-loop", 5) == 0) {
	    loop = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-verbose", 8) == 0) {
	    verbose=1;

	} else {
            fprintf (stderr, "ERROR Invalid arg: '%s'\n", argv[i]);
	    exit(0);;
	}
    }


    if(verbose){
	fprintf(stderr,"pan : mbusConnect...\n");fflush(stderr);
    }

    if ((mytid = mbusConnect("CMD", group, FALSE)) <= 0) {
	fprintf (stderr, "ERROR: Can't connect to message bus.\n");
	exit(1);
    }

    /* initialize global values */
    dhs.expID 			= 0;
    dhs.obsSetID 		= obsID;


#ifdef NEWFIRM					/* NEWFIRM 		*/
    totColsPDet = ncols 	= 2112 / bin;
    totRowsPDet = nrows 	= 2048 / bin;
    bytesPxl 			= 4;
    nimages			= 2;		/* No. images / PAN	*/

    /*  CCD Readout Locations	
     */
    xs[0] = 0; 		    ys[0] = 0;
    xs[1] = 2 * ncols;      ys[1] = 0;
    xs[2] = 0; 		    ys[2] = 2 * nrows;
    xs[3] = 2 * ncols;      ys[3] = 2 * nrows;
#endif

#ifdef MOSAIC					/* MOSAIC 		*/
    bytesPxl 			= 4;
#ifdef USE2AMP
    nimages			= 16;		/* No. images / PAN	*/
    totColsPDet = ncols 	= 1024 / bin + 50;
    totRowsPDet = nrows 	= 4096 / bin;

    /*  CCD Readout Locations	
     */
    for (i=0; i < 8; i++)        { xs[i] = i * ccd_xsize;
				   ys[i] = 0;       
				 }
    for (j=0 ; i < 16; i++, j++) { xs[i] = j * ccd_xsize;
				   ys[i] = 2 * ccd_ysize;
				 }
#else
    nimages			= 8;		/* No. images / PAN	*/
    totColsPDet = ncols 	= 2048 / bin + 100;
    totRowsPDet = nrows 	= 4096 / bin;

    /*  CCD Readout Locations	
     */
    for (i=0; i < 4; i++)        { xs[i] = i * ccd_xsize;
				   ys[i] = 0;         
				 }
    for (j=0 ; i < 8; i++, j++)  { xs[i] = j * ccd_xsize;
				   ys[i] = 2 * ccd_ysize;
				 }
#endif		/* USE2AMP */
#endif		/* MOSAIC  */

    nelem       = ncols * nrows;
    imSize 	= totRowsPDet * totColsPDet * bytesPxl;
    p 		= malloc (nelem * sizeof(int));

    if(verbose){
	fprintf (stderr, "ncols: %d  nrows: %d  bin:%d\n", ncols, nrows, bin);fflush(stderr);
    }



    /*  Simulate a SysOpen from the PAN.  */

    if(verbose){
	fprintf (stderr, "pan : dhsSysOpen\n");fflush(stderr);
    }

    (void) dhsSysOpen (&istat, resp, (dhsHandle *) &dhsID, (XLONG) IamPan);

    if (istat != DHS_OK){
	dhsError ("dhsSysOpen", resp);
	fprintf(stderr,"pan: ERROR on dhsSysOpen\n");fflush(stderr);
    }


    if(verbose){
	fprintf (stderr, "pan : dhsOpenConnect\n");fflush(stderr);
    }
    (void) dhsOpenConnect (&istat, resp, (dhsHandle *) &dhsID, IamPan, &fpCfg);

    if (istat < 0){
	dhsError ("dhsOpenConnect", resp);
        fprintf (stderr, "pan : Error on dhsOpenConnect\n");fflush(stderr);
    }

    /*  Set up for an AV Pair header write.
    */
    if(verbose){fprintf (stderr, "pan : Init AV Pair \n");fflush(stderr);}
    memset (&mdCfg, 0, sizeof(mdCfg));

    mdCfg.metaType 	= DHS_MDTYPE_AVPAIR;
    mdCfg.numFields 	= DHS_AVP_NUMFIELDS;
    mdCfg.fieldSize[0] 	= (XLONG) DHS_AVP_NAMESIZE;
    mdCfg.fieldSize[1] 	= (XLONG) DHS_AVP_VALSIZE;
    mdCfg.fieldSize[2] 	= (XLONG) DHS_AVP_COMMENT;
    mdCfg.dataType[0]  	= (XLONG) DHS_UBYTE;
    mdCfg.dataType[1]  	= (XLONG) DHS_UBYTE;
    mdCfg.dataType[2]  	= (XLONG) DHS_UBYTE;

    expID = 2454100.0;


    /*  AV Pair Header.  We create this for each image so we can verify
    **  we grabbed the right metadata pages, i.e. the keyword values will
    **  be "image_<expnum>_<keywnum>".  Likewise, the image array will
    **  have reference pixels that are "4000+<expnum>".
    */
    op = mdBuf;
    for (i = 0; Header[i].keyw; i++) {
	memset (keyw, ' ', 32);
	memset (val, ' ', 32);
	memset (comment, ' ', 64);

	sprintf (keyw, "%s", trim (Header[i].keyw));
	sprintf (val, "%s", trim (Header[i].val));
	sprintf (comment, "%s", trim (Header[i].comment));

	memmove (op, keyw, 32);
	memmove (&op[32], val, 32);
	memmove (&op[64], comment, 64);
	op += 128;
	nkeys++;
    }
    blkSize = (size_t) (nkeys * 128);



    to_tid = subject = -1;
    expnum = 0;
    if(verbose){fprintf (stderr, "pan : Waiting for commands on message bus\n");fflush(stderr);}
    while (mbusRecv (&from_tid, &to_tid, &subject, &host, &msg) >= 0) {

        if(verbose){fprintf (stderr, "pan : mbusRecv message received \n");fflush(stderr);}

	/* We don't readlly care about the value, we just want a 
	 ** monotonically increasing number.....
	expID += 0.01;
	 */
	expID     = atof (msg);
        dhs.expID = expID;

	if(verbose){fprintf (stderr,"expID::: %.6lf   '%s'\n", expID, msg);fflush(stderr);}

        /* Fill the array with a diagonal ramp.  Reference pixels will 
        ** indicate the image number.
        */

        if(verbose){fprintf (stderr,"pan filling diagonal ramp\n");fflush(stderr);}

        for (i = 0; i < nrows; i++) {
           for (j = 0; j < ncols; j++) {
              if (j > (ncols - 50))
                  pv = 2000 + (100 * (expnum+1));  	/* bias pixels        */
              else if (j > (ncols - 100))
                  pv = 4000 + (200 * (expnum+1));  	/* bias pixels        */
              else
                  pv = i + j;				/* ramp   	      */

              p[i * ncols + j] = pv;			/* set pixel value    */
           }
        }

        for (i = 0; i < (ncols/2); i++) {		/* make stripes       */
           for (j = 0; j < ncols; j++) {
	        if (( i == j || j == ((nrows/2)-i) )) {
		   for (k=0; k < 64; k++)
                       p[(i+k) * ncols + j] = 100;
		}
           }
        }


	if(verbose){fprintf (stderr,"pan: MSG subj : %d\n",subject);fflush(stderr);}


	if (subject == MB_START) {
	    int istart =1700, iend = 1764;

	    if (strcmp (group, "PanB") == 0)
		sleep (1);

	    /*  Send OpenExp from PAN
	     */

  	    if(verbose){fprintf (stderr,"pan: dhsOpenExp\n");fflush(stderr);}

	    (void) dhsOpenExp (&istat, resp, (dhsHandle) dhsID,
			       &fpCfg, &expID, obsID);
	    if (istat < 0) {
		dhsError ("dhsOpenExp", resp);
		fprintf (stderr,"pan: Error dhsOpenExp\n");fflush(stderr);
		istat = 1;
		break;
	    }


	    /*  Send Meta Data
	     */

	    if(verbose){fprintf (stderr,"pan: Sending PRE META data\n");fflush(stderr);}

    	    blkSize = (size_t) (nkeys * 128);
	    if (!sim) {
	        (void) dhsSendMetaData (&istat, resp, (dhsHandle) dhsID,
		    (char *) mdBuf, blkSize, &mdCfg, &expID, obsID);
	        if (istat < 0) {
		    dhsError ("1st dhsSendMetaData", resp);
		    fprintf (stderr,"pan: Error dhsSendMetaData\n");fflush(stderr);
		    istat = 1;
		    break;
	        }
	    }


	    /*  Send N arrays of nelem size  
	     */

	    if(verbose){fprintf (stderr,"pan: Sending image arrays\n");fflush(stderr);}

	    for (i = 0; i < nimages; i++) {

		imAddr = p;

		for (k=istart; k < iend; k++)
		    for (j=(ncols/2); j < (ncols/2+60); j++) {
       		        p[k * ncols + j + 768] = 800;
       		        p[k * ncols + j - 768] = 200;
		    }
		if (PanOffset) {
		    for (k=istart; k < iend; k++)
		        for (j=(ncols/2); j < (ncols/2+60); j++) {
       		            p[k * ncols + j + 768] = 800;
       		            p[k * ncols + j - 768] = 200;
			}
		}
		istart -= 100;
		iend   -= 100;


		fpCfg.xStart = (XLONG) xs[(PanOffset * 2) + i];
		fpCfg.yStart = (XLONG) ys[(PanOffset * 2) + i];
		fpCfg.xSize = ncols;
		fpCfg.ySize = nrows;
		if (!sim) {
		    if(verbose){printf("Sending %d bytes,  istat=%d\n", imSize, istat);fflush(stderr);}
		    (void) dhsSendPixelData (&istat, resp, (dhsHandle) dhsID,
		        (void *)imAddr, (size_t)imSize, &fpCfg, &expID, NULL);
		    if (istat < 0) {
		        dhsError ("dhsSendPixelData", resp);
			fprintf(stderr,"pan : Error dhsSendPixelData\n");fflush(stderr);
		        istat = 1;
		        break;
		    }
		}
	    }


	    /* Send 2nd MD Post header from PAN
	     */
	    if (!sim) {
	        if(verbose){fprintf (stderr,"pan: Sending POST META data\n");fflush(stderr);}
	        (void) dhsSendMetaData (&istat, resp, (dhsHandle) dhsID,
		    (char *) mdBuf, blkSize, &mdCfg, &expID, obsID);
	        if (istat < 0) {
		    dhsError ("2nd dhsSendMetaData", resp);
		    fprintf(stderr,"pan : Error dhsSendMetaData\n");fflush(stderr);
		    istat = 1;
		    break;
	        }
	    }

            if (bug) {

		/*  Simulate the bug where the Supervisor sees the NOCS
		**  DCA_CLOSE before the one from the Pan.
		*/

		if(verbose){fprintf(stderr,"pan : simulating supervisor DCA_CLOSE bug\n");fflush(stderr);}

		char msg[SZ_FNAME];

		memset (msg, 0, SZ_FNAME);
		sprintf (msg, "%.6lf", expID);

	        mbusBcast ("NOCS", msg, MB_FINISH);
		sleep (3);
	    }
	    else {
	        //if(verbose){fprintf (stderr, "NO BUG NOCS DCA_CLOSE...\n");fflush(stderr);}
            }

	    if(verbose){fprintf(stderr,"pan : dhsCloseExp\n");fflush(stderr);}
            (void) dhsCloseExp(&istat, resp, (dhsHandle) dhsID, expID);
            if (istat < 0) {
		dhsError ("dhsCloseExp", resp);
		fprintf(stderr,"pan : Error dhsCloseExp\n");fflush(stderr);
		istat = 1;
		break;
	    }

	    /* Tell the NOCS tester to finish the exposure if we're PanB.
	    ** If we're PanA, tell PanB to start its readout instead.
	     */
	    if (use_nocs && !bug) {
		char msg[SZ_FNAME];

		memset (msg, 0, SZ_FNAME);
		sprintf (msg, "%.6lf", expID);

		if (npans == 2 && strcmp (group, "PanA") == 0){

		    if(verbose){fprintf(stderr,"pan : sending MB_START to PanB\n");fflush(stderr);}
	            mbusBcast ("PanB", msg, MB_START);
                }
		else{
		    if(verbose){fprintf(stderr,"pan : sending MB_FINISH to NOCS\n");fflush(stderr);}
	            mbusBcast ("NOCS", msg, MB_FINISH);
		}
	    }

	} else if (subject == MB_FINISH) {
	    ;			/* no-op        */

	} else {
	    fprintf (stderr,"Unknown message: %d\n", subject);fflush(stderr);
	}

	if(verbose){fprintf(stderr,"pan : Waiting for commands\n");fflush(stderr);}
	to_tid = subject = -1;

        if (host) free ((void *) host);
        if (msg)  free ((void *) msg);

    }


    if(verbose){fprintf(stderr,"pan : dhsCLoseConnect, istat = %d\n",istat);fflush(stderr);}
    dhsCloseConnect (&istat, resp, (dhsHandle) dhsID);
    if (istat != DHS_OK){
	dhsError ("dhsCloseConnect", resp);
	fprintf(stderr,"pan : Error dhsCLoseConnect, istat = %d\n",istat);fflush(stderr);
    }


    if(verbose){fprintf(stderr,"pan : mbusDisconnect, istat = %d\n",istat);fflush(stderr);}
    mbusDisconnect (mytid);

    free (p);				
    if(verbose){fprintf(stderr,"pan : exitting, istat = %d\n",istat);fflush(stderr);}
    exit ((int)istat);
}


	
static void
dhsError (char *proc, char *resp)
{
    fprintf (stderr, "ERROR: %s failed. \\\\ %s\n", proc, resp);
    exit (1);
}


static char *
trim (char *buf)
{
    char *ip = buf;

    while (*ip && *ip == ' ')
	ip++;

    return (ip);
}


char *pTime(char *inStr)
{
    /* declare some variables and initialize them */
    struct tm *tim;
    time_t t;

    t = time ((time_t *) &t);
    tim = localtime ((time_t *) &t);

    if (inStr == (char *) NULL)
	return inStr;

    sprintf(inStr, "%4d%02d%02dT%02d%02d%02d-", tim->tm_year + 1900,
	tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);

    return inStr;
}
