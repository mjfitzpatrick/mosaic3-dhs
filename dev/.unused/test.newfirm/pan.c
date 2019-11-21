#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif


#include "dcaDhs.h"
#include "mbus.h"
#include "hdr.h"

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

int sim = 0;
int loop = 1;
int delay = 0;
int interactive = 0;

/*
int procDebug = 30;
*/

static char *trim (char *buf);



int main(int argc, char *argv[])
{
    int    i, j, k, mytid, PanOffset=0, use_nocs=1;
    int    nelem, sockbufsize, size, nkeys=0, offset=200, bstart=0;
    int    xs[4], ys[4], ncols, nrows;
    int    totRowsPDet, totColsPDet, imSize, bytesPxl;
    int    from_tid, to_tid, subject;
    int    expnum, nim, blkSize;
    int   *p, *imAddr, szIm, sock, pv;

    char   resp[80], retStr[80], mdBuf[(MAX_KEYWORDS * LEN_AVPAIR)];
    char  *op, *obsID = "TestID";
    char   keyw[32], val[32], comment[64];
    char  *host, *msg, *group = "Pan_Test";

    double expID;
    XLONG   istat;

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
	    group = "PanA_Test";

	} else if (strncmp(argv[i], "-B", 2) == 0) {
	    PanOffset = 1;
	    group = "PanB_Test";

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

	} else {
            fprintf (stderr, "ERROR Invalid arg: '%s'\n", argv[i]);
	    exit(0);;
	}
    }



    /* Initialize connections to the message bus.  */
    if ((mytid = mbusConnect("CMD_Test", group, FALSE)) <= 0) {
	fprintf (stderr, "ERROR: Can't connect to message bus.\n");
	exit(1);
    }

    /* initialize global values */
    dhs.expID = 0;
    dhs.obsSetID = obsID;

    ncols = 2112;
    nrows = 2048;
    nelem = ncols * nrows;
    totColsPDet = ncols;
    totRowsPDet = nrows;

    szIm = totRowsPDet * totColsPDet;
    p = malloc(szIm * sizeof(int));


    bytesPxl = 4;
    imSize = totRowsPDet * totColsPDet * bytesPxl;
    xs[0] = 0;
    ys[0] = 0;
    xs[1] = 2 * ncols;
    ys[1] = 0;
    xs[2] = 0;
    ys[2] = 2 * nrows;
    xs[3] = 2 * ncols;
    ys[3] = 2 * nrows;


    /*  Simulate a SysOpen from the PAN.
     */
    istat = 0;
    printf("=================start dhsSysOpen=================\n");
    dhsSysOpen(&istat, resp, (dhsHandle *) & dhsID, (XLONG) IamPan);
    if (istat != DHS_OK) {
	fprintf(stderr, "ERROR: dhsSysOpen failed. \\\\ %s\n", resp);
	exit(1);
    }
    printf("================= end dhsSysOpen =================\n");

    sockbufsize = 0;
    size = sizeof(int);


    printf("=================start dhsOpenConnect=================\n");
    (void) dhsOpenConnect(&istat, resp, (dhsHandle *) & dhsID,
			  IamPan, &fpCfg);
    if (istat < 0) {
	fprintf(stderr, "ERROR: dhsOpenConnect failed. \\\\ %s\n", resp);
	exit(1);		/* exit for now */
    }
    printf("=================end dhsOpenConnect=================\n");


    /* set up for an AV Pair header write
     */
    bzero (&mdCfg, sizeof(mdCfg));
    mdCfg.metaType = DHS_MDTYPE_AVPAIR;
    mdCfg.numFields = DHS_AVP_NUMFIELDS;
    mdCfg.fieldSize[0] = (XLONG) DHS_AVP_NAMESIZE;
    mdCfg.fieldSize[1] = (XLONG) DHS_AVP_VALSIZE;
    mdCfg.fieldSize[2] = (XLONG) DHS_AVP_COMMENT;
    mdCfg.dataType[0]  = (XLONG) DHS_UBYTE;
    mdCfg.dataType[1]  = (XLONG) DHS_UBYTE;
    mdCfg.dataType[2]  = (XLONG) DHS_UBYTE;

    expID = 2454100.0;



    /*  AV Pair Header.  We create this for each image so we can verify
     **  we grabbed the right metadata pages, i.e. the keyword values will
     **  be "image_<expnum>_<keywnum>".  Likewise, the image array will
     **  have reference pixels that are "4000+<expnum>".
     */
    op = mdBuf;
    for (i = 0; Header[i].keyw; i++) {
	memset(keyw, ' ', 32);
	memset(val, ' ', 32);
	memset(comment, ' ', 64);

	sprintf(keyw, "%s", trim (Header[i].keyw));
	sprintf(val, "%s", trim (Header[i].val));
	sprintf(comment, "%s", trim (Header[i].comment));

	memmove(op, keyw, 32);
	memmove(&op[32], val, 32);
	memmove(&op[64], comment, 64);
	op += 128;
	nkeys++;
    }
    blkSize = (size_t) (nkeys * 128);
    printf("================= MSG Created %d header keywords ....\n", nkeys);

    printf("================= MSG waiting for commands.....\n");
    to_tid = subject = -1;
    expnum = 0;
    while (mbusRecv(&from_tid, &to_tid, &subject, &host, &msg) >= 0) {

	/* We don't readlly care about the value, we just want a 
	 ** monotonically increasing number.....
	expID += 0.01;
	 */
	expID = atof(msg);
        dhs.expID = expID;

	printf ("expID::: %.6lf   '%s'\n", expID, msg);


        /* Fill the array with a diagonal ramp.  Reference pixels will 
        ** indicate the image number.
        */
        for (i = 0; i < nrows; i++) {
           for (j = 0; j < ncols; j++) {
              if (j >= (ncols-64))
                  pv = 4000 + (100 * expnum);  		/* reference pixels */
              else
                  pv = i + j;				/* ramp 	    */

              p[i * ncols + j] = pv;			/* set pixel value  */
           }
        }

	for (i=1700; i < 1790; i++)			/*  Pan-A or Pan-B  */
	    for (j=1700; j < 1790; j++)
                p[i * ncols + j] = 100;
	if (PanOffset) {				/*  Pan-B	    */
	    for (k=1700; k < 1790; k++)
	        for (j=1800; j < 1890; j++)
       	            p[k * ncols + j] = 100;
	}

        for (i = 0; i < 1024; i++) {
           for (j = 0; j < ncols; j++) {
	        if (( i == j || j == (2048-i) )) {
		   for (k=0; k < 64; k++)
                       p[(i+k) * ncols + j] = 100;
		}
           }
        }


	printf("================= MSG subj: %d ===================\n", subject);

	if (subject == MB_START) {
	    if (strcmp (group, "PanB_Test") == 0)
		sleep (1);

	    /* Send OpenExp from PAN
	     */
	    printf("=================start dhsOpenExp=================\n");
	    (void) dhsOpenExp(&istat, resp, (dhsHandle) dhsID,
			      &fpCfg, &expID, obsID);
	    printf("=================end dhsOpenExp=================\n");
	    if (istat < 0) {
		fprintf (stderr, "ERROR: PAN: dhsOpenExp failed. \\\\ %s\n",
		    resp);
		istat = 1;
		break;
	    }


	    /* Send Meta Data
	     */
    	    blkSize = (size_t) (nkeys * 128);
	    printf
		("=================start dhsSendMetaData=================\n");

	    if (!sim)
	        dhsSendMetaData(&istat, retStr, (dhsHandle) dhsID,
			        (char *) mdBuf, blkSize, &mdCfg, &expID,
			        obsID);
	    printf
		("=================end dhsSendMetaData=================\n");
	    if (istat < 0) {
		printf("ERROR: 1st dhsSendMetaData failed. \\\\ %s\n",
		       retStr);
		istat = 1;
		break;
	    }

	    /* Send 2 arrays of nelem size  */
	    nim = 2;
	    for (i = 0; i < nim; i++) {

		imAddr = p;

		if (i) {
		    for (k=1800; k < 1890; k++)
		        for (j=1800; j < 1890; j++)
       		            p[k * ncols + j] = 800;
		    if (PanOffset) {
		        for (k=1800; k < 1890; k++)
		            for (j=1700; j < 1790; j++)
       		                p[k * ncols + j] = 800;
		    }
		}


		fpCfg.xStart = (XLONG) xs[(PanOffset * 2) + i];
		fpCfg.yStart = (XLONG) ys[(PanOffset * 2) + i];
		fpCfg.xSize = ncols;
		fpCfg.ySize = nrows;
		printf("Sending %d bytes,  istat=%d\n", imSize, istat);
		printf
		   ("================start dhsSendPixelData================\n");
		if (!sim)
		    dhsSendPixelData(&istat, retStr, (dhsHandle) dhsID,
				     (void *) imAddr, (size_t) imSize,
				     &fpCfg, &expID, (char *) NULL);
		printf
		   ("=================end dhsSendPixelData=================\n");
		if (istat < 0) {
		    fprintf (stderr,
			"ERROR: dhsSendPixelData failed: %d. \\\\ %s\n",
			 istat, retStr);
		    istat = 1;
		    break;
		}

	    }


	    /* Send 2nd MD Post header from PAN
	     */
	    printf
		("=================start dhsSendMetaData=================\n");
	    if (!sim)
	        dhsSendMetaData(&istat, retStr, (dhsHandle) dhsID,
			        (char *) mdBuf, blkSize, &mdCfg, &expID,
			        obsID);
	    printf
		("=================end dhsSendMetaData=================\n");
	    if (istat < 0) {
		fprintf (stderr, "ERROR: 2nd dhsSendMetaData failed. \\\\ %s\n",
		       retStr);
		istat = 1;
		break;
	    }

	    printf ("=================start dhsCloseExp=================\n");
            (void) dhsCloseExp(&istat, resp, (dhsHandle) dhsID, expID);
	    printf ("=================end dhsCloseExp=================\n");
            if (istat < 0) {
                fprintf(stderr, "ERROR: panDataSaver: dhsCloseExp failed. \\\\ %s\n",
                    resp);
		istat = 1;
		break;
            }

	    /* Tell the NOCS tester to finish the exposure if we're PanB.
	    ** If we're PanA, tell PanB to start its readout instead.
	     */
	    if (use_nocs) {
		char msg[32];
		sprintf (msg, "%.6lf\0", expID);
		if (strcmp (group, "PanA_Test") == 0)
	            mbusBcast("PanB_Test", msg, MB_START);
		else
	            mbusBcast("NOCS_Test", msg, MB_FINISH);
	    }

	} else if (subject == MB_FINISH) {
	    ;			/* no-op        */

	} else {
	    printf("Unknown message: %d\n", subject);
	}

	printf ("\n\n");
	printf ("================= MSG waiting for commands.....\n");
	to_tid = subject = -1;

        if (host) free ((void *) host);
        if (msg)  free ((void *) msg);

    }


    printf("=================start dhsCloseConnect=================\n");
    dhsCloseConnect(&istat, retStr, (dhsHandle) dhsID);
    printf("=================end dhsCloseConnect=================\n");

    free(p);
    printf("=================Closing COLLECTOR socket=================\n");
    close(sock);


    mbusDisconnect (mytid);

    exit((int)istat);
}


static char *
trim (char *buf)
{
    char *ip = buf;

    while (*ip && *ip == ' ')
	ip++;

    return (ip);
}
