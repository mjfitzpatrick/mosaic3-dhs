#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

#include "dcaDhs.h"
#include "mbus.h"
#include "nocs.h"

#include <sys/time.h>


#ifndef OK
#define OK 	1
#endif
#ifndef IamPan
#define IamPan 	1
#endif
#ifndef IamNOCS
#define IamNOCS 2
#endif

#define N_PANS	2


struct timeval tv;
struct timezone tz;

int sim 	= 0;
int sky 	= 0;
int loop 	= 1;
int delay 	= 0;
int interactive = 0;
int use_nocs 	= 0;

int procDebug;

extern char *pTime();

static char *trim (char *buf);


int main(int argc, char *argv[])
{
    int   expnum, blkSize, ncols, nrows, nkeys;
    int   i, mytid, fcount = 0, n_pans = N_PANS;
    int   from_tid, to_tid, subject, *p;

    char  resp[80], retStr[80], mdBuf[32776];
    char  *op, *obsID = "TestID";
    char   keyw[32], val[32], comment[64], buf[80];
    char  *host, *msg;
    XLONG   istat, sock;
    double expID;


    mdConfig_t mdCfg;
    fpConfig_t fpCfg = {0L};
    dhsHandle dhsID;
    dhsHandle nocs_dhsID;



    /* Process the command-line arguments.
     */
    for (i = 1; i < argc; i++) {
	if (strncmp(argv[i], "-help", 5) == 0) {
	    /*clientUsage (); */
	    exit(0);;

	} else if (strncmp(argv[i], "-delay", 5) == 0) {
	    delay = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-debug", 5) == 0) {
	    procDebug = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-npans", 3) == 0) {
	    n_pans = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-interactive", 5) == 0) {
	    interactive++;
	    loop += 999;

        } else if (strncmp(argv[i], "-host", 5) == 0) {
            dcaInitDCAHost ();       		/* set simulated host */
            dcaSetSimHost (argv[++i], 1);

        } else if (strncmp(argv[i], "-sim", 4) == 0) {
	    /* dcaSetSimMode (1); */
	    sim++;

        } else if (strncmp(argv[i], "-sky", 4) == 0) {
	    /* If set, every other frame will be a NOCTYP=sky frame.
	    */
	    sky++;

	} else if (strncmp(argv[i], "-loop", 5) == 0) {
	    loop = atoi(argv[++i]);

	} else if (strncmp(argv[i], "-no_nocs", 8) == 0) {
	    use_nocs = 0;

	} else {
	    /*clientUsage (); */
	    exit(0);;
	}
    }



    /* Initialize connections. To the supervisior  */
    /* dhsHandle is a structure */
    if ((mytid = mbusConnect("CMD_Test", "NOCS_Test", FALSE)) <= 0) {
	fprintf(stderr, "ERROR: Can't connect to message bus.\n");
	exit(1);
    }

    /* initialize global values */
    dhs.expID = 0;
    dhs.obsSetID = "";


    /*  Simulate a SysOpen from the NOCS.
     */
    istat = 0;
    printf("=================start dhsSysOpen=================\n");
    dhsSysOpen(&istat, resp, (dhsHandle *) & nocs_dhsID, (XLONG) IamNOCS);
    if (istat != DHS_OK) {
	fprintf(stderr, "ERROR: dhsSysOpen failed. \\\\ %s\n", resp);
	exit(1);
    }
    printf("================= end dhsSysOpen =================\n");


    printf
	("=================start nocs dhsOpenConnect=================\n");
    (void) dhsOpenConnect(&istat, resp, (dhsHandle *) & nocs_dhsID,
			  IamNOCS, &fpCfg);
    if (istat < 0) {
	fprintf(stderr, "ERROR: dhsOpenConnect failed. \\\\ %s\n", resp);
	exit(1);		/* exit for now */
    }
    printf("=================end nocs dhsOpenConnect=================\n");


    /* set up for an AV Pair header write
     */
    mdCfg.metaType     = DHS_MDTYPE_AVPAIR;
    mdCfg.numFields    = DHS_AVP_NUMFIELDS;
    mdCfg.fieldSize[0] = (XLONG) DHS_AVP_NAMESIZE;
    mdCfg.fieldSize[1] = (XLONG) DHS_AVP_VALSIZE;
    mdCfg.fieldSize[2] = (XLONG) DHS_AVP_COMMENT;
    mdCfg.dataType[0]  = (XLONG) DHS_UBYTE;
    mdCfg.dataType[1]  = (XLONG) DHS_UBYTE;
    mdCfg.dataType[2]  = (XLONG) DHS_UBYTE;


    /* 
     */
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


    fpCfg.xStart = (XLONG) 0;
    fpCfg.yStart = (XLONG) 0;
    fpCfg.xSize = 2 * ncols;
    fpCfg.ySize = 2 * nrows;


    printf("================= MSG waiting for commands.....\n");
    to_tid = subject = -1;
    while (mbusRecv(&from_tid, &to_tid, &subject, &host, &msg) >= 0) {

	/* We don't readlly care about the value, we just want a 
	 ** monotonically increasing number.....
	expID += 0.01;
	 */
	expID = atof (msg);

	printf("================= MSG subj: %d '%s'\n", subject, msg);
	if (subject == MB_START) {
            char msg[32];

	    printf ("\t%s -- Got a START message\n", pTime(buf));

	    /* Send OpenExp from NOCS
	     */
	    printf
		("=================start nocs dhsOpenExp=================\n");
	    (void) dhsOpenExp(&istat, resp, (dhsHandle) nocs_dhsID, &fpCfg,
			      &expID, obsID);
	    printf
		("=================end nocs dhsOpenExp=================\n");
	    if (istat < 0) {
		printf("ERROR: NOCS: dhsOpenExp failed. \\\\ %s\n", resp);
		istat = 1;
		break;
	    }

	    /* Send PRE META from the NOCS.
	     */
	    printf
		("============== start nocs dhsSendMetaData ==============\n");

	    if (!sim)
	        dhsSendMetaData(&istat, retStr, (dhsHandle) nocs_dhsID,
			        (char *)mdBuf, blkSize, &mdCfg, &expID, obsID);
    	    printf
		("================= MSG Sent %d bytes (%d/%d keywords) ....\n",
		    blkSize, (blkSize / 128), nkeys);
	    printf
		("=================end nocs dhsSendMetaData=================\n");

	    if (istat < 0) {
		printf("ERROR: NOCS 2nd dhsSendMetaData failed. \\\\ %s\n",
		       retStr);
		istat = 1;
		break;
	    }

            /* Tell the PANs to start running.
             */
            sprintf (msg, "%.6lf\0", expID);
            mbusBcast("PanA_Test", msg, MB_START);

	    printf ("================= MSG waiting for FINISH.....\n");


	} else if (subject == MB_FINISH) {

	    printf ("\t%s -- Got a FINISH message\n", pTime(buf));

	    /* Skip until we get a FINISH from each of the PANs
	    fcount++;
	    if ((fcount % n_pans) == 0) {
		to_tid = subject = -1;
		continue;
	    }
	    */

	    /* Send POST META from NOCS
	     */
	    printf
		("=================start nocs dhsSendMetaData=================\n");
	    if (!sim)
	        dhsSendMetaData(&istat, retStr, (dhsHandle) nocs_dhsID,
			        (char *)mdBuf, blkSize, &mdCfg, &expID, obsID);
    	    printf
		("================= MSG Sent %d bytes (%d/%d keywords) ....\n",
		    blkSize, (blkSize / 128), nkeys);
	    printf
		("=================end nocs dhsSendMetaData=================\n");
	    if (istat < 0) {
		printf("ERROR: NOCS 2nd dhsSendMetaData failed. \\\\ %s\n",
		       retStr);
		istat = 1;
		break;
	    }

	    /* Send CloseExp from NOCS.  This is supposed to trigger the 
	     ** processing in the Supervisor.
	     */
	    printf
		("=================start nocs dhsCloseExp=================\n");
	    (void) dhsCloseExp(&istat, resp, (dhsHandle) nocs_dhsID,
			       expID);
	    printf
		("=================end nocs dhsCloseExp=================\n\n");
	    if (istat < 0) {
		printf("ERROR: NOCS: dhsCloseExp failed. \\\\ %s\n", resp);
		istat = 1;
		break;
	    }

            sprintf (msg, "%.6lf\0", expID);
            mbusBcast("trigger", msg, MB_FINISH);

	} else
	    printf("Unknown message: %d\n", subject);


	if (subject != MB_START) {
	    printf ("\n\n");
	    printf ("================= MSG waiting for commands.....\n");
	}

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

    exit(istat);
}



static char *
trim (char *buf)
{
    char *ip = buf;

    while (*ip && *ip == ' ')
        ip++;

    return (ip);
}

