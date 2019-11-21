#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <pvm3.h>
#include <time.h>
#include <tcl.h>
#include <errno.h>

#include "dso.h"
#include "dsim.h"
#include "kwdb.h"
#include "vmcache.h"
#include "mosdca.h"
#include "mbus.h"

/*
 * MOSDCA.C -- Mosaic Data Capture Agent (DCA).  The DCA captures image data
 * from the message bus and writes out imagefiles to disk.
 *
 *   Usage: mosdca [flags - see below]
 *
 *	 [-dir <dir>]		# working directory
 *	 [-debug <level>]	# debug level for message output
 *	 [-vminit <initstr>]	# VMcache initialization
 *	 [-imformat <format>]	# imagefile filename format
 *	 [-imindex <index>]	# imagefile filename index
 *	 [-maxkw <maxkw>]	# max header keywords (both)
 *	 [-maxgkw <v>]		# max global header keywords
 *	 [-maxikw <v>]		# max mage header keywords
 *	 [-reqlevel <v>]	# client request enable level (0-2)
 *
 * All command line options can also be set at runtime via message bus
 * requests to the DCA, using the SetParam and GetParam requests.
 *
 * The Data Capture Agent (DCA) for the NOAO Mosaic provides a message bus
 * based image capture service for CCD detectors, in particular the NOAO
 * CCD Mosaic.  When started up the DCA automatically connects to the
 * message bus.  The DCA sits in a loop indefinitely watching the message
 * bus for message events.  When a CCD detector such as the Mosaic reads out
 * an image it writes events and header or pixel data packets to the message
 * bus as data is read out of the detector.  The DCA receives these message
 * packets and formats and outputs an image to disk.
 *
 * Each readout sequence is identified by a unique sequence number assigned
 * by the readout client.  Once a readout has started, each subsequent message
 * associated with that readout must be tagged with the sequence number for
 * that readout.  Multiple readouts can be simultaneously in progress.  Each
 * readout sequence has a separate context identified by its sequence number.
 *
 * DCA parameters such as the current working directory and the debug level
 * can be set on the command line, or via DCA_SetParam requests at runtime.
 * Such global options affect all readout contexts.
 *
 * The operation of the DCA can be monitored by enabling debug output.  By
 * default debug output is written to the standard output (if the DCA is
 * started under PVM this output can be intercepted otherwise it is written
 * to the pvmd log).  The debug flag takes an integer argument defining the
 * debug level.  Level one logs all primary operations providing a record
 * of the actions performed by the DCA.  Debug levels higher than 1 provide
 * internal information about the operation of the DCA.
 */

#define	IMDIM		2
#define	MAX_CONTEXT	8
#define	MAX_KWDB	64
#define	MAX_CLIENTS	16
#define	SZ_NAME		128
#define	SZ_LINE		1024
#define	SZ_PATHNAME	256
#define	SZ_VALSTR	8192
#define	SZ_KWDATA	32768
#define	MAX_KEYWORDS	72
#define PIXTYPE		int
#define	DEF_VMINIT	"cachesize=312m"

/* Readout context descriptor. */
struct context {
	int seqno;			/* readout sequence number 	     */
	int mode;			/* readout mode (stage) 	     */
	int iostat;			/* nonzero if any errors occurred    */
	int nkwdb;			/* number of keyword databases 	     */
	int maxkw_global;		/* max global keywords 		     */
	int maxkw_image;		/* max image keywords 		     */
	int synchronized;		/* set after synchronize with client */
	int nhdrpkt, npixpkt;		/* number asynchronous packets 	     */
	int thdrpkt, tpixpkt;		/* total number of packets 	     */
	pointer dsim;			/* DSIM handle for output imagefile  */
	pointer kwdb_list[MAX_KWDB];	/* list of keyword databases 	     */
	char imagename[SZ_NAME];	/* logical image name from feed      */
	char imagetype[SZ_NAME];	/* logical image type from feed      */
	char fname[SZ_PATHNAME];	/* imagefile filename 		     */
	char ktm[SZ_PATHNAME];		/* keyword translation module 	     */
	int datalen, swapped, nstreams;	/* WritePixels context 		     */
	DCA_Stream *streams;		/* WritePixels context 		     */
	pointer pixels;			/* WritePixels context 		     */

	double expID;			/* Exposure ID			     */
	char   *obsID;			/* Observation Set ID		     */
};
typedef struct context Context;

Context dca_context[MAX_CONTEXT], *last_context;
static int encoding = PvmDataDefault;

extern int errno;
extern int nopen_kwdbs;

int    debug = 1;
FILE  *debug_out = NULL;
char   debugfile[SZ_PATHNAME];

char   defktm[SZ_PATHNAME];
int    maxkw_global = MAX_KEYWORDS;
int    maxkw_image = MAX_KEYWORDS;
char   imformat[SZ_PATHNAME] 	= "%T%04I.fits";
char   curdir[SZ_PATHNAME];
char   vminit[SZ_PATHNAME] 	= DEF_VMINIT;
int    imindex 			= 1;
int    clobber 			= 0;
double expID 			= 0.0;
char   *obsID 			= "";
void   *vm;

time_t t1, t2;



/* Request enable level.  0 denies all remote SetParam requests (hence only
 * the command line can set options).  1 allows only a console task to issue
 * SetParam requests.  2 or greater allows any client to issue SetParams.
 */
int request_level = 1;
int console = 0;

int clients[MAX_CLIENTS];
int nclients = 0;

char *GetParam();
char *dprintf(int level, char *name, char *fmt, ...);
Context *setContext();
Context *getContext();
pointer GetKWDB();
static char *broadcast(int tid, char *class, char *fmt, ...);
static int dca_register();
static int dca_unregister();
extern char *getcwd(), *getenv();
static int tcl_dprintf();
static int streq();
static int encode_i4(), decode_i4();

#define max(a,b)	((a)<(b)?(b):(a))
#define min(a,b)	((a)<(b)?(a):(b))
#define abs(a)		((a)<0?(-a):(a))


/* DCA_MAIN -- Main for the Data Capture Agent.
 */
main (argc, argv)
int argc;
char **argv;
{
	int mytid, i;
	char buf[SZ_PATHNAME];


	getcwd (curdir, SZ_PATHNAME);
	if (getenv ("clobber"))
	    clobber++;

	debug_out = stdout;

	/* Process any command line arguments.
	 */
	for (i=1;  i < argc;  i++) {
	    char *argp = argv[i];

	    if (!strcmp (argp, "-dir")) {
		char *dir;

		dir = argv[++i];
		if (chdir(dir) < 0) {
		    dprintf (0, "dca_main", "chdir %s failed\n", dir);
		    return (-1);
		} else {
		    strcpy (curdir, dir);
		    broadcast (0, "file", "directory %s\n", curdir);
		}

	    } else if (!strcmp (argp, "-debug")) {
		debug = atoi (argv[++i]);

	    } else if (!strcmp (argp, "-mdebug")) {
		putenv ("MALLOC_CHECK_=2");

	    } else if (!strcmp (argp, "-reqlevel")) {
		request_level = atoi (argv[++i]);

	    } else if (!strcmp (argp, "-debugfile")) {
		strcpy (debugfile, argv[++i]);
		if (!(debug_out = fopen (debugfile, "a")))
		    debug_out = stdout;

	    } else if (!strcmp (argp, "-vminit")) {
		strcpy (vminit, argv[++i]);

	    } else if (!strcmp (argp, "-imformat")) {
		strcpy (imformat, argv[++i]);

	    } else if (!strcmp (argp, "-imindex")) {
		imindex = atoi (argv[++i]);

	    } else if (!strcmp (argp, "-ktm")) {
		strcpy (defktm, argv[++i]);

	    } else if (!strcmp (argp, "-clobber")) {
		clobber++;
	    } else if (!strcmp (argp, "-noclobber")) {
		clobber = 0;

	    } else if (!strcmp (argp, "-maxkw")) {
		maxkw_global = maxkw_image = atoi (argv[++i]);

	    } else if (!strcmp (argp, "-maxgkw")) {
		maxkw_global = atoi (argv[++i]);

	    } else if (!strcmp (argp, "-maxikw")) {
		maxkw_image = atoi (argv[++i]);

	    } else {
		dprintf (0, "dca_main", "unrecognized argument %s\n", argp);
	    }
	}

	/* Attach to the message bus (PVM currently).
	if ((mytid = pvm_mytid()) < 0) {
	    pvm_perror ("mosdca");
	    return (-1);
	}
	*/
	if ((mytid = mbusConnect ("NF-DCA", "NF-DCA", 0)) <= 0) {
	    fprintf (stderr, "ERROR: Can't connect to message bus.\n");
	    return (-1);
	}
	sprintf (buf, "dca_tid %d\0", mytid);
	mbusSend (SUPERVISOR, ANY, MB_SET, buf);
	sprintf (buf, "dca_cwd %s\0", curdir);
	mbusSend (SUPERVISOR, ANY, MB_SET, buf);

	/* Register the server with the message bus. */
	if (dca_register (MOSDCA, mytid) < 0) {
	    pvm_perror ("mosdca(register)");
	    return (-1);
	}

	dprintf (1, "dca_main", "starting DCA, pid=%d tid=%d\n",
	    getpid(), mytid);

	/* Initialize the VM cache. */
	dprintf (1, "dca_main", "starting vmcache, %s\n", vminit);
	if (!(vm = vm_initcache (NULL, vminit))) {
	    dprintf (0, "dca_main", "vmcache initialization failed\n");
	    return (-1);
	}

	/* Main message processing loop. */
	mbusSend (SUPERVISOR, ANY, MB_READY, "READY NF-DCA");
	dca_handle (mytid);

	dca_unregister (MOSDCA, mytid);
	pvm_exit();
	return (0);
}


/* DCA_HANDLE -- Main event processing loop for the data capture agent.
 * Watch for incoming messages and dispatch them to handlers as they arrive.
 */
dca_handle (mytid)
int mytid;
{
	register Context *ctx = NULL;
	int status, len, msgtag, tid;
	int seqno, i, j, *ip;

	for (;;) {
	    /* Wait for the next message to arrive. */
	    if (pvm_recv (-1, -1) < 0) {
		dprintf (0, "dca_handle", "pvm_recv error\n");
		pvm_perror ("dca_handle(recv)");
		return;
	    }

	    /* Get message attributes. */
	    if (pvm_bufinfo (pvm_getrbuf(), &len, &msgtag, &tid) < 0) {
		dprintf (0, "dca_handle", "pvm_bufinfo error\n");
		pvm_perror ("dca_handle(bufinfo)");
		return;
	    }

	    /* Handle the message. */
	    switch (msgtag) {
	    case DCA_Quit:
	    case 64:			/* NDHS MB_DONE tag	*/
		/* Shutdown.
		 */
		broadcast (0, "control", "mosdca %d quits\n", mytid);
		Shutdown();
		return;

	    case DCA_RequestConsole:
		/* Become the console task.
		 */
		broadcast (0, "control", "console requested by %d\n", tid);

		if (!console) {
		    console = tid;
		    broadcast (0, "control", "console granted to %d\n", tid);
		}

		pvm_initsend (encoding);
		status = (console == tid) ? 0 : -1;
		pvm_pkint (&status, 1, 1);
		pvm_send (tid, DCA_RequestConsole);
		break;

	    case DCA_ReleaseConsole:
		/* Free the console.
		 */
		if (console == tid) {
		    broadcast (0, "control", "console released by %d\n", tid);
		    console = 0;
		}
		break;

	    case DCA_SetParam: {
		/* Set a server parameter.
		 */
		Context *ctx;
		char param[SZ_NAME];
		char value[SZ_VALSTR];
		int seqno;

		pvm_upkint (&seqno, 1, 1);
		pvm_upkstr (param);
		pvm_upkstr (value);

		ctx = seqno ? getContext(seqno) : (Context *)NULL;
		SetParam (ctx, tid, param, value);

		broadcast (0, "control",
		    "setparam %s to %s (task %d)\n", param, value, tid);
		break;
	    }

	    case DCA_GetParam: {
		/* Get a server parameter.
		 */
		Context *ctx;
		char param[SZ_NAME];
		char *value;
		int seqno;

		pvm_upkint (&seqno, 1, 1);
		pvm_upkstr (param);

		ctx = seqno ? getContext(seqno) : (Context *)NULL;
		value = GetParam (ctx, param);

		pvm_initsend (encoding);
		pvm_pkstr (value ? value : "");
		pvm_send (tid, DCA_GetParam);
		if (ctx)
		    ctx->synchronized++;

		broadcast (0, "control",
		    "getparam %s (task %d)\n", param, tid);
		break;
	    }

	    case DCA_SubscribeEvents: {
		/* Client request to subscribe to DCA events.  The event 
		 * class argument is ignored at present.
		 */
		char client[SZ_NAME];
		char eventclass[SZ_LINE];
		int i, slot = 0;

		pvm_upkstr (client);
		pvm_upkstr (eventclass);

		/* Find an empty slot in the client list. */
		for (i=0, slot = -1;  i < MAX_CLIENTS;  i++)
		    if (clients[i] == 0) {
			slot = i;
			break;
		    }

		/* Enter the subscription. */
		if (slot >= 0) {
		    dprintf (1, "mosdca",
			"subscription request from %s for %s events\n",
			client, eventclass);
		    clients[slot] = tid;
		    nclients++;
		} else {
		    dprintf (0, "mosdca",
			"subscription request from %s denied, maxclients\n",
			client);
		}

		/* Report the subscription event. */
		broadcast (0, "control",
		    "client %s requests all events of class `%s'\n",
		    client, eventclass);
		broadcast (tid, "file", "directory %s\n", curdir);
		break;
	    }

	    case DCA_UnsubscribeEvents: {
		/* Client request to unsubscribe to DCA events.  At present
		 * all event subscriptions by the client are unsubscribed
		 * and the event class argument is ignored.
		 */
		char client[SZ_NAME];
		char eventclass[SZ_LINE];
		int i;

		pvm_upkstr (client);
		pvm_upkstr (eventclass);

		/* Clear any slots used by this client. */
		for (i=0;  i < MAX_CLIENTS;  i++)
		    if (clients[i] == tid)
			clients[i] = 0;

		/* Compact the client list (so that we can pass it directly
		 * to pvm_mcast).
		 */
		for (nclients=i=0;  i < MAX_CLIENTS;  i++)
		    if (clients[i] > 0)
			clients[nclients++] = clients[i];

		dprintf (1, "mosdca",
		    "unsubscription request from %s\n", client);

		/* Report the unsubscription event. */
		broadcast (0, "control",
		    "client %s unsubscribes all events of class `%s'\n",
		    client, eventclass);
		break;
	    }

	    case DCA_GetStatus: {
		/* Return the status of an image given its readout sequence
		 * number.  The readout mode (stage of processing) is returned
		 * followed by a status value, which is nonzero if any errors
		 * have occurred during processing.
		 */
		Context *ctx;
		int seqno, mode, iostat;

		pvm_upkint (&seqno, 1, 1);
		ctx = seqno ? getContext(seqno) : NULL;
		mode = (ctx ? ctx->mode : M_INACTIVE);
		iostat = (ctx ? ctx->iostat : 0);

		pvm_initsend (encoding);
		pvm_pkint (&mode, 1, 1);
		pvm_pkint (&iostat, 1, 1);
		pvm_send (tid, DCA_GetStatus);
		if (ctx)
		    ctx->synchronized++;
		break;
	    }

	    case DCA_StartReadout: {
		/* Start a readout sequence tagged by the given sequence 
		 * number.  Zero is returned if the request is successful
		 * and -1 if an error occurs.
		 */
		register Context *ctx = NULL;
		char imagename[DCA_SZIMNAME];
		char imagetype[DCA_SZIMTYPE];
		int seqno, status, iostat;

		mbusSend (SUPERVISOR, ANY, MB_STATUS, "active");

		pvm_upkint (&seqno, 1, 1);
		pvm_upkstr (imagename);
		pvm_upkstr (imagetype);

		/* StartReadout requested event. */
		broadcast (0, "readout",
		"StartReadout requested seqno=%d imagename=%s imagetype=%s\n", 
		    seqno, imagename, imagetype);

		if (seqno && (ctx = setContext(seqno)))
		    if (StartReadout (ctx, imagename, imagetype) < 0) {
			freeContext (seqno);
			ctx = NULL;
		    }

		status = ctx ? 0 : -1;
		iostat = ctx ? 0 : -1;
		pvm_initsend (encoding);
		pvm_pkint (&status, 1, 1);
		pvm_pkint (&iostat, 1, 1);
		pvm_send (tid, DCA_StartReadout);
		if (ctx) {
		    ctx->synchronized++;
    		    ctx->nhdrpkt = 0;
    		    ctx->npixpkt = 0;
    		    ctx->thdrpkt = 0;
    		    ctx->tpixpkt = 0;
		}

		/* StartReadout completed event. */
		broadcast (0, "readout",
		    "StartReadout completed seqno=%d status=%d iostat=%d\n",
		    seqno, status, iostat);

		/* Send newimage created event. */
		if (ctx) {
		    broadcast (0, "file", "start %s %s\n",
			dsim_FileName (ctx->fname),
			(status == 0) ? "ok" : "error");
		}
		break;
	    }

	    case DCA_TransferComplete: {
		register Context *ctx = NULL;
		int seqno, status=1, iostat;
		char msg[SZ_LINE];

		pvm_upkint (&seqno, 1, 1);

		/* TransferComplete event. */
		dprintf (0, "TransferComplete", "Sequence number %d\n", seqno);

		sprintf (msg, "dca_trans_done %d\0", seqno);
		mbusSend (SUPERVISOR, ANY, MB_SET, msg);
		break;
	    }

	    case DCA_EndReadout: {
		/* Normal end of readout.  Zero is returned if the request is
		 * successful, otherwise -1.  The i/o status for the entire
		 * readout is also returned.  This will be zero if there were
		 * no processing errors, nonzero if there were any errors
		 * during readout (GetStatus can also be called to query the
		 * i/o status and processing mode during reaodut).  EndReadout
		 * does not return until the output image has been fully
		 * processed and closed.
		 */
		register Context *ctx = NULL;
		int seqno, status=-1, iostat;
		char buf[SZ_LINE], fname[SZ_LINE];


		pvm_upkint (&seqno, 1, 1);

		/* EndReadout requested event. */
		broadcast (0, "readout", "EndReadout requested seqno=%d\n",
		    seqno);

		if (seqno && (ctx = getContext (seqno))) {
		    status = EndReadout (ctx, 0);
		    iostat = ctx->iostat;
		    bzero (buf, SZ_LINE);
		    sprintf (buf, "dca_trans  %d %d",
			ctx->thdrpkt, ctx->tpixpkt);
		    sprintf (fname, "%s/%s\0", curdir, ctx->fname);
		    freeContext (seqno);
		} else {
		    dprintf (0, "EndReadout",
			"Bad sequence number %d\n", seqno);
		}

		pvm_initsend (encoding);
		pvm_pkint (&status, 1, 1);
		pvm_pkint (&iostat, 1, 1);
		pvm_send (tid, DCA_EndReadout);
		if (ctx)
		    ctx->synchronized++;

		/* EndReadout completed event. */
		broadcast (0, "readout",
		    "EndReadout completed seqno=%d status=%d iostat=%d\n",
		    seqno, status, iostat);

		/* Send newimage completed event. */
		if (ctx) {
		    broadcast (0, "file", "finish %s %s\n",
			dsim_FileName (ctx->fname),
			(status == 0) ? "ok" : "error");
		}

		/* Flush it...Initial buffer set above.
		*/
		mbusSend (SUPERVISOR, ANY, MB_SET, buf);

        	sprintf (buf, "dca_done %.6lf  %s\0", expID, fname);
        	mbusSend (SUPERVISOR, ANY, MB_SET, buf);

		mbusSend (SUPERVISOR, ANY, MB_STATUS, "inactive");
		mbusSend (SUPERVISOR, ANY, MB_STATUS, "Readout complete...");
/*
        	for (i=0;  i < 20;  i++)
	            mbusSend (SUPERVISOR, ANY, MB_ACK, "");
*/

		/* Get the end time...
	 	*/
		t2 = time ((time_t)NULL);
		dprintf (1, "EndReadout", "[%d] Complete [Time: ~%d sec]\n", 
		    seqno, (t2-t1));
	        dprintf (1,"EndReadout",
		    "[%d]************************************\n\n", seqno);


		break;
	    }

	    case DCA_AbortReadout: {
		/* Abort the current readout.
		 */
		register Context *ctx = NULL;
		int seqno, status=-1, iostat;

		pvm_upkint (&seqno, 1, 1);
		broadcast (0, "readout", "AbortReadout seqno=%d\n", seqno);

		if (seqno && (ctx = getContext (seqno))) {
		    status = EndReadout (ctx, 1);
		    iostat = ctx->iostat;
		    freeContext (seqno);
		} else {
		    dprintf (0, "AbortReadout",
			"Bad sequence number %d\n", seqno);
		}

		pvm_initsend (encoding);
		pvm_pkint (&status, 1, 1);
		pvm_pkint (&iostat, 1, 1);
		pvm_send (tid, DCA_AbortReadout);
		if (ctx)
		    ctx->synchronized++;
		break;
	    }

	    case DCA_ConfigureImage: {
		/* Configure the image geometry.  Called when the number of
		 * images and their sizes are known, to allocate space in the
		 * imagefile.  Zero is returned if the request is successful,
		 * otherwise -1.
		 */
		register Context *ctx = NULL;
		char imagename[DCA_SZIMNAME];
		char imagetype[DCA_SZIMTYPE];
		int seqno, nimages, i, j;
		DCA_Image *images, *im;
		int status = -1;

		static char s_requested[] =
"ConfigureImage requested seqno=%d imagename=%s imagetype=%s nimages=%d\n";
		static char s_geometry[] =
"ConfigureImage geometry seqno=%d image=%d pixtype=%d naxes=%d nx=%d ny=%d\n";
		static char s_completed[] =
		    "ConfigureImage completed seqno=%d status=%d iostat=%d\n";


		/* Get the start time...
	 	*/
		t1 = time ((time_t)NULL);

		pvm_upkint (&seqno, 1, 1);
		pvm_upkstr (imagename);
		pvm_upkstr (imagetype);
		pvm_upkint (&nimages, 1, 1);

		broadcast (0, "readout", s_requested,
		    seqno, imagename, imagetype, nimages);

dprintf (1, "ConfigureImage", "Seqno  [%d]\n", seqno);
		if (seqno && (ctx = getContext (seqno))) {
		    
dprintf (1, "ConfigureImage", "Context used for [%d] = 0x%x\n", seqno, ctx);
		    images = (DCA_Image *) calloc (nimages, sizeof(DCA_Image));
		    if (images) {
			for (i=0;  i < nimages;  i++) {
			    im = &images[i];
			    pvm_upkint (&im->pixtype, 1, 1);
			    pvm_upkint (&im->naxes, 1, 1);
			    for (j=0;  j < im->naxes;  j++)
				pvm_upkint (&im->axlen[j], 1, 1);
			    for (  ;  j < DCA_MAXDIM;  j++)
				im->axlen[j] = 1;
			    pvm_upkint (&im->extend, 1, 1);
			    if (pvm_upkstr (im->name) < 0)
				im->name[0] = '\0';
			    if (pvm_upkstr (im->type) < 0)
				im->type[0] = '\0';

			    broadcast (0, "readout", s_geometry, seqno, i+1,
				im->pixtype, im->naxes, im->axlen[0], im->axlen[1]);
			}
			status = ConfigureImage (ctx,
			    imagename, imagetype, nimages, images);
			free ((char *) images);
		    }
		} else {
		    dprintf (0, "ConfigureImage",
			"Bad sequence number %d\n", seqno);
		}

		pvm_initsend (encoding);
		pvm_pkint (&status, 1, 1);
		pvm_pkint (&ctx->iostat, 1, 1);
		pvm_send (tid, DCA_ConfigureImage);
		if (ctx)
		    ctx->synchronized++;

		broadcast (0, "readout", s_completed,
		    seqno, status, ctx->iostat);
		break;
	    }

	    case DCA_WriteHeaderData: {
		/* Write a block of header data to the image (actually to a
		 * keyword database for subsequent disposal).  This request
		 * is asynchronous and does not return a status.  GetStatus
		 * can be called to query the mode and i/o status.
		 */
		register Context *ctx = NULL;
		char kwgroup[SZ_NAME];
		char kwdata[SZ_KWDATA];
		int seqno, nkeywords, i;
		char *sbuf, *op;
		DCA_Keyword *kwl;

		dprintf (3, "WHdr", "*********** WriteHeader Requested\n");
		pvm_upkint (&seqno, 1, 1);
		pvm_upkstr (kwgroup);
		pvm_upkint (&nkeywords, 1, 1);

		dprintf (3, "WHdr", 
		    "***********[%d]  keygroup='%s', nkey=%d, len=%d\n", 
		    seqno, kwgroup, nkeywords, len);
		if (!seqno || !(ctx = getContext (seqno))) {
		    dprintf (0, "WriteHeaderData",
			"Bad sequence number %d\n", seqno);
		    break;
		}

		if (!(sbuf = (char *) malloc(len))) {
		    ctx->iostat++;
		    break;
		}
		if (!(kwl = (DCA_Keyword *) malloc
			(nkeywords * sizeof(DCA_Keyword)))) {
		    ctx->iostat++;
		    break;
		}

		/* Unpack the keyword data into the internal descriptor.
		 */
		for (i=0, op=sbuf;  i < nkeywords;  i++) {
		    pvm_upkstr (op);
		    kwl[i].keyword = op;  op += strlen(op) + 1;
		    pvm_upkstr (op);
		    kwl[i].value = op;    op += strlen(op) + 1;
		    pvm_upkstr (op);
		    kwl[i].type = op;     op += strlen(op) + 1;
		    pvm_upkstr (op);
		    kwl[i].comment = op;  op += strlen(op) + 1;
		}

		/* Construct an ASCII list of the keywords.
		 */
		bzero (kwdata, SZ_KWDATA);
		op = kwdata;
		*op++ = '{';
		for (i=0;  i < nkeywords;  i++) {
		    sprintf (op, "{ %s {%s} %s {%s} } ",
			kwl[i].keyword, 
			kwl[i].value, 
			kwl[i].type, 
			kwl[i].comment);
		    while (*op)
			op++;
		}
		*op++ = '}';
		*op++ = '\0';

		    
		dprintf (3, "WHdr",
    		    "**** WriteHeaderData seqno=%d ctx=0x%x kwgroup=%s nkeywords=%d %160.160s\n",
		    seqno, ctx, kwgroup, nkeywords, kwdata);

		broadcast (0, "readout.io",
		    "WriteHeaderData seqno=%d kwgroup=%s nkeywords=%d %s\n",
		    seqno, kwgroup, nkeywords, kwdata);

		if (WriteHeaderData (ctx, kwgroup, nkeywords, kwl) < 0)
		    ctx->iostat++;

		free ((char *) kwl);
		free ((char *) sbuf);
		break;
	    }

	    case DCA_WritePixelData:
	    case DCA_WritePixHeader:
		/* Write a block of pixels to the output image.  The pixels
		 * are passed in a block containing one or more "streams" of
		 * pixels which are written to different regions of the output
		 * image.  The stream descriptors define how to map pixels
		 * from the data block to the output image.  This request
		 * is asynchronous and does not return a status.  GetStatus
		 * can be called to query the mode and i/o status.
		 */
		pvm_upkint (&seqno, 1, 1);
		if (!seqno || !(ctx = getContext (seqno))) {
		    dprintf (0, "WritePixelData",
			"Bad sequence number %d\n", seqno);
		    break;
		}

		pvm_upkint (&ctx->datalen, 1, 1);
		pvm_upkint (&ctx->swapped, 1, 1);
		pvm_upkint (&ctx->nstreams, 1, 1);

dprintf (4, "WPixHdr", "*************** seqno = %d\n", seqno);
dprintf (4, "WPixHdr", "*************** ctx = 0x%x\n", ctx);
dprintf (4, "WPixHdr", "*************** datalen = %d\n", ctx->datalen);
dprintf (4, "WPixHdr", "*************** swapped = %d\n", ctx->swapped);
dprintf (4, "WPixHdr", "*************** nstreams = %d\n", ctx->nstreams);
		broadcast (0, "readout.io",
		"WritePixelData seqno=%d datalen=%d nstreams=%d npixels=%d\n",
		    ctx->seqno, ctx->datalen, ctx->nstreams,
		    ctx->datalen / sizeof(PIXTYPE));

		/* Get an array of stream descriptors. */
		ctx->streams =
		    (DCA_Stream *) malloc (ctx->nstreams * sizeof(DCA_Stream));
		if (!ctx->streams) {
		    ctx->iostat++;
		    break;
		}

dprintf (4, "WPix", "WritePixHdr seqno=%d datalen=%d nstreams=%d npixels=%d\n",
    ctx->seqno, ctx->datalen, ctx->nstreams,
    ctx->datalen / sizeof(PIXTYPE));

		/* Unpack the stream descriptors. */
		for (i=0;  i < ctx->nstreams;  i++)
		    pvm_upkint ((int *) &(ctx->streams[i]),
			sizeof (DCA_Stream) / sizeof(int), 1);

		/* Unpack the raw input pixel array.  It would be nice if we
		 * could call WritePixelData on the raw message buffer to save
		 * a memory-to-memory copy, but since a large message can be
		 * fragmented and passed in multiple message bufs we need to
		 * reassemble it into a single large buffer before decoding
		 * and writing to the output image.  We can speed things up
		 * a bit however by using upkbyte, which will BCOPY the data
		 * out of the message buffers.
		 */
		if (!(ctx->pixels = (pointer) malloc (ctx->datalen))) {
		    free ((char *) ctx->streams);
		    ctx->iostat++;
		    break;
		}
dprintf (4, "WPix", 
    "WritePixData ctx=0x%x seqno=%d ctx->pixels=0x%x ctx->datalen=%d\n",
    ctx, ctx->seqno, ctx->pixels, ctx->datalen);


		/* If the write pixels operation is broken into separate
		 * header and data message packets, we are done until the
		 * data packet arrives.
		 */
		if (msgtag == DCA_WritePixHeader)
		    continue;
		/* otherwise fall through */

	    case DCA_WritePixPixels:
		/* We get here either in a fall through from a WritePixels,
		 * or when processing the WritePixPixels message following
		 * a header decode with WritePixHeader.  If we are processing
		 * a WritePixPixels then the seqno is passed in encoded
		 * format in the binary message data.
		 */
		if (msgtag == DCA_WritePixPixels) {
		    pvm_upkbyte ((char *)&seqno, sizeof(seqno), 1);
		    seqno = decode_i4 (seqno);
		    if (!seqno || !(ctx = getContext (seqno))) {
			dprintf (0, "WritePixelData",
			    "Bad sequence number %d\n", seqno);
			break;
		    }
		    dprintf (3, "DCA_WritePixPixels",
		        "[%d] seqno %d  ctx=0x%x\n", seqno, seqno, ctx);
		}

		dprintf (3, "DCA_WritePixPixels",
		    "[%d] ctx=0x%x  unpacking %d bytes from 0x%x\n", 
		    seqno, ctx, ctx->datalen, ctx->pixels);

		pvm_upkbyte ((char *)ctx->pixels, ctx->datalen, 1);

#if 1
		/* Encode the pixels (if necessary) for storage in the
		 * imagefile.
		 */
		dsim_EncodePixels (ctx->dsim, ctx->streams[0].destimage,
		    ctx->pixels, ctx->pixels, ctx->datalen, ctx->swapped);
#endif

		/* Descramble and copy data to the output image. */
		dprintf (3, "DCA_WritePixPixels",
		    " calling WritePixelData with ctx nstreams = %d and ctx pixels = %d\n",
			ctx->nstreams, ctx->pixels);

		if (WritePixelData (ctx,
			ctx->nstreams, ctx->streams, ctx->pixels) < 0)
		    ctx->iostat++;

		free ((char *) ctx->pixels);
		free ((char *) ctx->streams);
		break;

	    case DCA_Synchronize: {
		/* Echo a synchronize packet back to the caller.  This is
		 * used to flush any queued messages and synchronize two
		 * processes, e.g. to prevent data overruns.
		 */
		register Context *ctx = NULL;
		int seqno, async=1;

		pvm_upkint (&seqno, 1, 1);
		ctx = seqno ? getContext(seqno) : NULL;

		/* Sync the image. */
		if (ctx && dsim_Sync (ctx->dsim, async) < 0)
		    ctx->iostat++;

		dprintf (1, "DCA_Synchronize",
		"[%d] synch after %d header %d data packets, iostat=%d\n",
		    seqno, ctx ? ctx->nhdrpkt : 0, ctx ? ctx->npixpkt : 0,
		    ctx ? ctx->iostat : 0);

		pvm_initsend (encoding);
		pvm_send (tid, DCA_Synchronize);
		if (ctx) {
		    char buf[SZ_LINE];

		    bzero (buf, SZ_LINE);
		    sprintf (buf, "dca_trans  %d %d", 
			ctx->thdrpkt, ctx->tpixpkt);
		    mbusSend (SUPERVISOR, ANY, MB_SET, buf);

		    ctx->synchronized++;
		    ctx->nhdrpkt = 0;
		    ctx->npixpkt = 0;
		}
		break;
	    }

	    default:
		break;
	    }

	}
}

bob() { int i = 0; }

/*
 * Event handlers.
 * ---------------
 */


/* Shutdown -- Clean up and shutdown the DCA server.
 */
Shutdown()
{
	register Context *ctx;
	register int i;

	dprintf (1, "dca_handle", "Shutting down the server...\n");

	for (i=0, ctx = &dca_context[0];  i < MAX_CONTEXT;  i++, ctx++)
	    if (ctx->seqno)
		EndReadout (ctx, 1);

	vm_closecache (vm);
	dprintf (1, "dca_handle", "Shutdown the server\n");
}


/* SetParam -- Set a server parameter or option.
 */
SetParam (ctx, tid, param, value)
register Context *ctx;
int tid;
char *param;
char *value;
{
	/* Determine if the requesting task has permission to issue this
	 * request.
	 */
	if (!request_level || (request_level == 1 && tid != console)) {
	    dprintf (2, "SetParam", "set %s request from task %d denied\n",
		param, tid);
	    return (-1);
	} else
	    dprintf (2, "SetParam", "%s = `%s'\n", param, value);

	if (!strncmp (param, "directory", 3)) {
	    /* Change the current working directory.  The directory must
	     * exist and be writeable or the request is denied.
	     */
	    char path[SZ_PATHNAME];
	    char *newdir = value;
	    int status=0, fd = -1;

	    /* Ignore redundant requests. */
	    if (strcmp (newdir, curdir) == 0)
		return (0);

	    sprintf (path, "%s/dcaXXXXXX", newdir);
	    if ((fd = mkstemp(path)) < 0 || chdir(newdir) < 0) {
		dprintf (0, "SetParam", "[%d] set directory %s failed\n",
		    ctx ? ctx->seqno : 0, newdir);
		status = -1;
	    } else {
		dprintf (1, "SetParam", "[%d] set directory to %s\n",
		    ctx ? ctx->seqno : 0, newdir);
		strcpy (curdir, newdir);
		broadcast (0, "file", "directory %s\n", curdir);
		status = 0;
	    }

	    if (fd >= 0) {
		close (fd);
		unlink (path);
	    }

	    return (status);

	} else if (!strcmp (param, "reqlevel")) {
	    /* Set the request enable level. */
	    request_level = atoi (value);
	    return (0);

	} else if (!strcmp (param, "debug")) {
	    /* Set the debug level for debug/log messages. */
	    debug = atoi (value);
	    return (0);

	} else if (!strcmp (param, "debugfile")) {
	    strcpy (debugfile, value);
	    if (!(debug_out = fopen (debugfile, "a")))
		debug_out = stdout;

	} else if (!strcmp (param, "imformat")) {
	    strcpy (imformat, value);
	    return (0);

	} else if (!strcmp (param, "imindex")) {
	    imindex = atoi (value);
	    return (0);

	} else if (!strcmp (param, "ktm")) {
	    strcpy (ctx ? ctx->ktm : defktm, value);

	} else if (!strcmp (param, "clobber")) {
	    clobber = (streq (value, "yes"));
	    return (0);

	} else if (!strcmp (param, "maxkw_global")) {
	    if (ctx)
		ctx->maxkw_global = atoi (value);
	    else
		maxkw_global = atoi (value);
	    return (0);

	} else if (!strcmp (param, "maxkw_image")) {
	    if (ctx)
		ctx->maxkw_image = atoi (value);
	    else
		maxkw_image = atoi (value);
	    return (0);

	} else if (!strcmp (param, "maxkw")) {
	    if (ctx)
		ctx->maxkw_global = ctx->maxkw_image = atoi (value);
	    else
		maxkw_global = maxkw_image = atoi (value);
	    return (0);

	} else if (!strcmp (param, "expID")) {
	    if (ctx)
		ctx->expID = expID = atof (value);
	    else
		expID = atof (value);
	    return (0);


	} else if (!strcmp (param, "ObsID")) {
		;		/* no-op for now.... 	*/
	    return (0);
	}

	return (-1);
}


/* GetParam -- Get a server parameter or option.
 */
char *
GetParam (ctx, param)
register Context *ctx;
char *param;
{
	static char valstr[SZ_VALSTR];
	register char *s = NULL;

	dprintf (1, "GetParam", "%s -> ", param);

	if (!strncmp (param, "directory", 3)) {
	    /* Get the current working directory. */
	    s = getcwd (valstr, SZ_PATHNAME);

	} else if (!strcmp (param, "reqlevel")) {
	    /* Set the request enable level. */
	    sprintf (s = valstr, "%d", request_level);

	} else if (!strcmp (param, "debug")) {
	    /* Get the debug level for debug/log messages. */
	    sprintf (s = valstr, "%d", debug);

	} else if (!strcmp (param, "debugfile")) {
	    /* Get the debug file filename. */
	    if (debug_out == stdout)
		strcpy (s = valstr, "stdout");
	    else
		s = debugfile;

	} else if (!strcmp (param, "imformat")) {
	    s = imformat;

	} else if (!strcmp (param, "imindex")) {
	    sprintf (s = valstr, "%d", imindex);

	} else if (!strcmp (param, "ktm")) {
	    s = ctx ? ctx->ktm : defktm;

	} else if (!strcmp (param, "clobber")) {
	    s = clobber ? "yes" : "no";

	} else if (!strcmp (param, "maxkw")) {
	    sprintf (s = valstr, "%d", min(maxkw_global,maxkw_image));

	} else if (!strcmp (param, "maxgkw")) {
	    sprintf (s = valstr, "%d", ctx ? ctx->maxkw_global : maxkw_global);

	} else if (!strcmp (param, "maxikw")) {
	    sprintf (s = valstr, "%d", ctx ? ctx->maxkw_image : maxkw_image);
	}

	dprintf (1, NULL, "`%s'\n", s ? s : "[undefined]");
	return (s);
}


/* Broadcast -- Broadcast a DCA event.  The message is formatted and sent to
 * the client given by TID, or to all clients that have subscribed to DCA
 * events if tid=0.
 *
 *   Usage:	broadcast (tid, class, format, [args])
 *
 * Where "class" is the event class.
 */
static char *
broadcast (int tid, char *class, char *fmt, ...)
{
	va_list args;
	char msgtext[SZ_VALSTR];

	va_start (args, fmt);

	/* Format the message. */
	(void) vsprintf (msgtext, fmt, args);

	/* Broadcast the event message. */
	if (nclients > 0) {
	    pvm_initsend (encoding);
	    pvm_pkstr (class);
	    pvm_pkstr (msgtext);
	    pvm_mcast (clients, nclients, DCA_Event);
	}
	va_end (args);

	dprintf (3, "broadcast", "%s %s\n", class, msgtext);
}


/* StartReadout -- Start a readout of a new image.
 */
StartReadout (ctx, imagename, imagetype)
register Context *ctx;
char *imagename;
char *imagetype;
{
	char newfile[SZ_PATHNAME];

	ctx->mode = M_START;
	strcpy (ctx->imagename, imagename);
	strcpy (ctx->imagetype, imagetype);
	errno = 0;

	dprintf (1, "StartReadout",
	    "[%d] ------- imagename=%s imagetype=%s -------\n",
	    ctx->seqno, imagename, imagetype);

	/* Get new image filename. */
	MakeFileName (ctx, imformat, &imindex);
	strcpy (newfile, dsim_FileName (ctx->fname));

	dprintf (1, "StartReadout",
	    "[%d] ------- created imagename=%s / %s -------\n",
	    ctx->seqno, newfile, ctx->fname);

	/* Make sure we aren't clobbering an existing image.  If so and 
	 * clobber is enabled overwrite the image.  Otherwise if we would
	 * be clobbering the image, change the name to avoid doing so.
	 */
	if (access (newfile, 0) == 0) {
	    if (clobber) {
		dprintf (2, "dsim_Open",
		    "[%d] operation will replace file %s\n",
		    ctx->seqno, ctx->fname);
		unlink (newfile);

	    } else {
		char fname[SZ_PATHNAME];
		char oname[SZ_PATHNAME];
		char *ip, *extn;
		int i;

		/* Print warning message. */
		dprintf (0, "dsim_Open", "[%d] operation would overwrite %s\n",
		    ctx->seqno, ctx->fname);

		/* Get file root name and extension. */
		strcpy (oname, dsim_FileName(ctx->fname));
		for (ip=oname, extn=NULL;  *ip;  ip++)
		    if (*ip == '.')
			extn = ip + 1;
		if (extn)
		    *(extn-1) = '\0';

		/* Create new filename. */
		for (i='a';  i <= 'z';  i++) {
		    sprintf (fname, "%s%c.%s", oname, i, extn ? extn : "");
		    if (access (fname, 0) < 0)
			break;
		}

		strcpy (ctx->fname, fname);
		dprintf (0, "dsim_Open", "[%d] output filename changed to %s\n",
		    ctx->seqno, ctx->fname);
	    }
	}

	if (ctx->dsim = dsim_Open (vm, ctx->fname, DSO_CREATE)) {
	    dprintf (1, "dsim_Open", "[%d] create succeeded for %s\n",
		ctx->seqno, ctx->fname);
	} else {
	    dprintf (0, "dsim_Open", "[%d] create failed for %s (errno=%d)\n",
		ctx->seqno, ctx->fname, errno);
	    ctx->mode = M_INACTIVE;
	    return (-1);
	}

	return (0);
}


/* EndReadout -- Called when a readout has completed to close the output
 * image and free any resources associated with the given image seqno.
 * Errors are reported but we ignore them and proceed as best we can so as
 * to leave as complete an image as possible.
 */
EndReadout (ctx, abort)
register Context *ctx;
int abort;
{
	int status = 0, nimages, i;
	char kwdb_list[SZ_LINE], buf[SZ_FNAME], *op;
	char imageid[SZ_NAME], dbname[SZ_FNAME], imname[SZ_FNAME];
	Tcl_Interp *tcl = NULL;
	pointer o_kwdb[MAX_KWDB];
	int n_okwdb = 0;
	char *ip, *s;

	ctx->mode = M_FINISH;
	dprintf (1, "EndReadout", "[%d] %s\n", ctx->seqno,
	    abort ? "abort readout" : "normal end of readout");

	/* Sync the image. */
	if (dsim_Sync (ctx->dsim, 1) < 0)
	    ctx->iostat++;

	/* Create a Tcl interpreter and run the keyword translation module
	 * script (KTM) to translate the instrument keywords to the format
	 * required for the output image.
	 */
	if (!(tcl = Tcl_CreateInterp())) {
	    status = -1;
	    goto cleanup;
	}

	/* Load the KWTB Tcl module. */
	kwdb_TclInit (tcl);

	/* Make the DCA dprintf function available to Tcl. */
	Tcl_CreateCommand (tcl, "dprintf", tcl_dprintf, NULL, NULL);

	/* The list of input keyword databases is passed to the KTM as the
	 * tcl variable "inkwdbs", the value consisting of a list of KWDB
	 * pointers.
	 */
	for (i=0, op=kwdb_list;  i < ctx->nkwdb;  i++) {
	    sprintf (op, "0x%x", ctx->kwdb_list[i]);
	    while (*op)
		op++;
	    *op++ = ' ';
	}
	*op++ = '\0';
	dprintf (1, "EndReadout", "[%d] kwdb_list: '%s'\n",
	    ctx->seqno, kwdb_list);
	if (Tcl_SetVar (tcl, "inkwdbs", kwdb_list, 0) == NULL)
	    status = -1;

	/* Debug stuff for inspecting input keyword groups.
	 */
	if (debug >= 2) {
	    char *ip, *op, kwdb_names[SZ_LINE];
	    pointer kwdb;
	    int i;

	    for (i=0, op=kwdb_names;  i < ctx->nkwdb;  i++) {
		kwdb = ctx->kwdb_list[i];
		for (ip=kwdb_Name(kwdb);  *op = *ip++;  op++)
		    ;
		if (i+1 < ctx->nkwdb)
		    *op++ = ' ';
	    }
	    *op++ = '\0';
	    dprintf (2, "EndReadout", "[%d] inkwdbs kwdb_names=`%s'\n",
		ctx->seqno, kwdb_names);
	    dprintf (2, "EndReadout", "[%d] inkwdbs kwdb_list=`%s'\n",
		ctx->seqno, kwdb_list);

	    if (debug >= 3) {
		for (i=0;  i < ctx->nkwdb;  i++) {
		    char name[SZ_NAME];
		    int level = 3;

		    kwdb = ctx->kwdb_list[i];
		    sprintf (name, "%s [%d]", kwdb_Name(kwdb), ctx->seqno);

		    dprintf (level, name,
			"------------------ %d keywords ----------------\n",
			kwdb_Len (kwdb));
		    PrintKWDB (kwdb, name, level);
		}
	    }
	}

	/* The "imageid" string is used by the KTM to identify dprintf msgs. */
	for (ip=s=ctx->ktm;  *ip;  ip++) {
	    if (*ip == '/')
		s = ip + 1;
	}
	sprintf (imageid, "%s [%d]", s, ctx->seqno);
	if (Tcl_SetVar (tcl, "imageid", imageid, 0) == NULL)
	    status = -1;
	dprintf (1, "EndReadout", "[%d] setting imageid='%s'...\n", 
	    ctx->seqno, imageid);

	/* Pass the filename to the TCL environment for the KTM. */
	sprintf (imname, "%s\0", ctx->fname);
	sprintf (dbname, "%s\0", ctx->fname);
	for (ip=dbname+strlen(dbname)-1; *ip != '.' && ip > dbname; ip--)
	    ;
	*ip = '\0';				/* replace the extension */
	strcat (dbname, ".dat");
	if (Tcl_SetVar (tcl, "dbname", dbname, 0) == NULL)
	    status = -1;
	dprintf (1, "EndReadout", "[%d] setting dbname='%s'...\n", 
	    ctx->seqno, dbname);

	if (Tcl_SetVar (tcl, "imname", imname, 0) == NULL)
	    status = -1;
	dprintf (1, "EndReadout", "[%d] setting imname='%s'...\n", 
	    ctx->seqno, imname);

	/* Pass the DCA context for the current readout as a set of TCL
	 * global variables.
	 */
	SetTclContext (tcl, ctx);

	/* Run the KTM script. */
	dprintf (2, "EndReadout", "[%d] translate keywords...\n", ctx->seqno);
	if (*ctx->ktm && access(ctx->ktm,0) == 0) {
	    dprintf (2, "EndReadout", "[%d] KTM = %s\n", ctx->seqno, ctx->ktm);
	    if (RunTcl (tcl, ctx->ktm, ctx->seqno) < 0)
		status = -1;
	} else {
	    dprintf (0, "EndReadout", "[%d] NO KTM\n", ctx->seqno);
	    status = -1;
	}

	dprintf (2, "EndReadout", "[%d] write keywords to image...\n",
	    ctx->seqno);

	/* Write the output extension headers.  The KTM script returns the
	 * a list of the output keywords databases in the variable outkwdbs.
	 */
	if (s = (char *)Tcl_GetVar (tcl, "outkwdbs", 0)) {
	    pointer kwdb;
	    char *start;

	    for (n_okwdb=0;  s && n_okwdb < MAX_KWDB;  n_okwdb++) {
		start = s;  kwdb = (pointer) strtol (s, &s, 16);
		if (!kwdb || start == s)
		    break;
		o_kwdb[n_okwdb] = kwdb;
	    }
	} else {
	    dprintf (0, "EndReadout", "[%d] cannot get variable outkwdbs\n",
		ctx->seqno);
	    status = -1;
	}

	nimages = dsim_Get (ctx->dsim, 0, DSIM_NImages);
	dprintf (1, "EndReadout", "[%d] Updating image headers\n", ctx->seqno);
	dprintf (1, "EndReadout", "[%d] Number of open keyword dbs = %d\n",
	    ctx->seqno, n_okwdb);

	for (i=0;  i < n_okwdb;  i++) {
	    pointer kwdb = o_kwdb[i];
	    char *name = kwdb_Name (kwdb);
	    int imageno;

	    if (streq (name, "PHU"))
		imageno = 0;
	    else
		imageno = atoi (name + 2);

	    if (imageno < 0 || imageno > nimages) {
		dprintf (0, "EndReadout", "[%d] no image for kwdb=%s\n",
		    ctx->seqno, name);
	    } else {
		dprintf (2, "EndReadout",
		    "[%d] Updating header for kwdb=%s, image=%d\n",
		    ctx->seqno, name, imageno);
		if (dsim_WriteHeader (ctx->dsim, imageno, kwdb) < 0) {
		    dprintf (0, "EndReadout", "[%d] error updating %s header\n",
			ctx->seqno, name);
		    status = -1;
		}
	    }

	    if (kwdb) {
		dprintf (1, "EndReadout", "[%d] Closing '%s'\n",
	    	    ctx->seqno, name);
		kwdb_Close (kwdb);
	    }
	}


cleanup:
	/* Delete the keyword databases. */
	for (i=0;  i < ctx->nkwdb;  i++)
	    kwdb_Close (ctx->kwdb_list[i]);
	ctx->nkwdb = 0;

	/* Close the output image. */
	ctx->mode = M_DONE;
	if (ctx->dsim) {
	    if (dsim_Close (ctx->dsim) < 0) {
		status = -1;
		dprintf (0, "EndReadout", "[%d] image sync and close failed\n",
		    ctx->seqno);
	    }
	    ctx->dsim = NULL;
	}
	dprintf (1, "EndReadout", "[%d] close image, status=%d iostat=%d\n",
	    ctx->seqno, status, ctx->iostat);
	dprintf (1, "EndReadout", "[%d] close image, expID=%lf obsID=%s\n",
	    ctx->seqno, expID, ctx->obsID);
	dprintf (1,"EndReadout","[%d] Number of open KW databases: %d\n",
	    ctx->seqno, nopen_kwdbs);
	dprintf (1,"EndReadout","[%d]************************************\n",
	    ctx->seqno);

	if (tcl)
	    Tcl_DeleteInterp (tcl);

	return (status);
}


/* SetTclContext -- Pass various items of DCA context for the current 
 * readout to a Tcl interpreter as global variables.
 */
SetTclContext (tcl, ctx)
Tcl_Interp *tcl;
register Context *ctx;
{
	register pointer dsim = ctx->dsim;
	char buf[SZ_LINE], index[32];
	int nimages, pixtype, i, status = 0;

	/*
	 * Global readout context.
	 */

	sprintf (buf, "%d", ctx->seqno);
	if (Tcl_SetVar (tcl, "seqno", buf, 0) == NULL)
	    status = -1;

	sprintf (buf, "%d", ctx->iostat);
	if (Tcl_SetVar (tcl, "iostat", buf, 0) == NULL)
	    status = -1;

	sprintf (buf, "%d", ctx->nkwdb);
	if (Tcl_SetVar (tcl, "nkwdb", buf, 0) == NULL)
	    status = -1;

	sprintf (buf, "%d", ctx->maxkw_global);
	if (Tcl_SetVar (tcl, "maxgkw", buf, 0) == NULL)
	    status = -1;

	sprintf (buf, "%d", ctx->maxkw_image);
	if (Tcl_SetVar (tcl, "maxikw", buf, 0) == NULL)
	    status = -1;

	sprintf (buf, "0x%x", ctx->dsim);
	if (Tcl_SetVar (tcl, "dsim", buf, 0) == NULL)
	    status = -1;

	if (Tcl_SetVar (tcl, "imagename", ctx->imagename, 0) == NULL)
	    status = -1;
	if (Tcl_SetVar (tcl, "imagetype", ctx->imagetype, 0) == NULL)
	    status = -1;
	if (Tcl_SetVar (tcl, "imagefile", ctx->fname, 0) == NULL)
	    status = -1;
	if (Tcl_SetVar (tcl, "ktmfile", ctx->ktm, 0) == NULL)
	    status = -1;

	/*
	 * Image parameters.
	 */

	if (!dsim)
	    return (status);

	nimages = dsim_Get (dsim, 0, DSIM_NImages);

	sprintf (buf, "%d", nimages);
	if (Tcl_SetVar (tcl, "nimages", buf, 0) == NULL)
	    status = -1;

	for (i=1;  i <= nimages;  i++) {
	    sprintf (index, "%d", i);

	    pixtype = dsim_Get (dsim, i, DSIM_PixelType);
	    switch (pixtype) {
	    case DSO_UBYTE:
		strcpy (buf, "ubyte");
		break;
	    case DSO_SHORT:
		strcpy (buf, "short");
		break;
	    case DSO_USHORT:
		strcpy (buf, "ushort");
		break;
	    case DSO_INT:
		strcpy (buf, "int");
		break;
	    case DSO_LONG:
		strcpy (buf, "long");
		break;
	    case DSO_REAL:
		strcpy (buf, "real");
		break;
	    case DSO_DOUBLE:
		strcpy (buf, "double");
		break;
	    default:
		strcpy (buf, "badtype");
	    }
	    if (Tcl_SetVar2 (tcl, "pixtype", index, buf, 0) == NULL)
		status = -1;

	    sprintf (buf, "%d", dsim_Get(dsim,i,DSIM_NAxes));
	    if (Tcl_SetVar2 (tcl, "naxes", index, buf, 0) == NULL)
		status = -1;

	    sprintf (buf, "%d", dsim_Get(dsim,i,DSIM_Axlen1));
	    if (Tcl_SetVar2 (tcl, "axlen1", index, buf, 0) == NULL)
		status = -1;

	    sprintf (buf, "%d", dsim_Get(dsim,i,DSIM_Axlen2));
	    if (Tcl_SetVar2 (tcl, "axlen2", index, buf, 0) == NULL)
		status = -1;
	}

	return (status);
}


/* ConfigureImage -- Fix the layout of the output image file.  ConfigureImage
 * should be called when the primary size attributes of the output image are
 * known, allowing the image geometry (file layout) to be computed and the
 * output imagefile prepared to receive data.
 */
ConfigureImage (ctx, imagename, imagetype, nimages, images)
register Context *ctx;
char *imagename, *imagetype;
int nimages;
DCA_Image *images;
{
	register pointer dsim = ctx->dsim;
	register DCA_Image *im;
	int status, i;

	ctx->mode = M_READY;
	dprintf (1, "ConfigureImage", "[%d] name=%s type=%s nimages=%d\n",
	    ctx->seqno, imagename, imagetype, nimages);
	if (*imagename)
	    strcpy (ctx->imagename, imagename);
	if (*imagetype)
	    strcpy (ctx->imagetype, imagetype);

	/* Set the global imagefile attributes. */
	dsim_Set (dsim, 0, DSIM_NImages, nimages);
	dsim_Set (dsim, 0, DSIM_MaxKeywords, ctx->maxkw_global);

	/* Set the individual image attributes. */
	for (i=1;  i <= nimages;  i++) {
	    im = &images[i-1];
	    dprintf (2, "ConfigureImage",
		"[%d] %d: nam=%s typ=%s pty=%d axes=%d[%d,%d] ext=%d\n",
		ctx->seqno, i, im->name, im->type, im->pixtype,
		im->naxes, im->axlen[0], im->axlen[1], im->extend);

	    dsim_Set (dsim, i, DSIM_MaxKeywords, ctx->maxkw_image);
	    dsim_Set (dsim, i, DSIM_ObjectName, im->name);
	    dsim_Set (dsim, i, DSIM_ObjectType, im->type);
	    dsim_Set (dsim, i, DSIM_PixelType, im->pixtype);
	    dsim_Set (dsim, i, DSIM_NAxes, im->naxes);
	    dsim_Set (dsim, i, DSIM_Axlen1, im->axlen[0]);
	    dsim_Set (dsim, i, DSIM_Axlen2, im->axlen[1]);
	    dsim_Set (dsim, i, DSIM_Axlen3, im->axlen[2]);
	    dsim_Set (dsim, i, DSIM_Extend, im->extend);
	}

	/* Configure the image. */
	status = dsim_Configure (dsim);

	/* If the configure fails the data feed will abort (no EndReadout)
	 * so we should go ahead and free the readout context.  This closes
	 * the imagefile.
	 */
	if (status < 0) {
	    char fname[SZ_PATHNAME];
	    strcpy (fname, ctx->fname);
	    freeContext (ctx->seqno);
	    ctx->mode = M_DONE;
	    unlink (dsim_FileName (fname));
	}

	return (status);
}


/* WriteHeaderData -- Write a block of header data.  Header data is not
 * written directly to the output image, rather it is buffered in named KWDB
 * keyword databases for subsequent header translation.  There can be any
 * number of keyword databases for the incoming image.  Keywords are grouped
 * according to the kwgroup name, one group per database.  Successive calls
 * to WriteHeaderData for the same keyword group will append keywords to the
 * group.  The set of kwgroup names is arbitrary and depends upon the system
 * which feeds data to the DCA.  Note this is only the raw keyword input.
 * The subsequent system-dependent header translation stage initiated by
 * EndReadout will convert these raw keyword groupings to the global header
 * and individual image headers written to the output imagefile.
 */
WriteHeaderData (ctx, kwgroup, nkeywords, kwl)
register Context *ctx;
char *kwgroup;
int nkeywords;
DCA_Keyword *kwl;
{
	register DCA_Keyword *kw;
	register pointer kwdb;
	int i;

	ctx->mode = M_WRITING;
	if (ctx->synchronized) {
	    dprintf (1, "WriteHeaderData",
		"[%d] begin asynchronous write sequence\n", ctx->seqno);
	}
	dprintf (3, "WriteHeaderData", "[%d] kwgroup=%s nkeywords=%d\n",
	    ctx->seqno, kwgroup, nkeywords);

	/* Get the keyword database for the given kwname.  A new KWDB is
	 * allocated when we see a kwname for the first time.
	 */
	if (!(kwdb = GetKWDB (ctx, kwgroup)))
	    return (-1);

	/* Append the block of keywords to the KWDB. */
	for (i=0;  i < nkeywords;  i++) {
	    kw = &kwl[i];
	    dprintf (3, "WriteHeaderData", "[%d] %s [%s] = %s / %s\n",
		ctx->seqno, kw->keyword, kw->type, kw->value, kw->comment);
	    if (kwdb_AddEntry (kwdb,
		    kw->keyword, kw->value, kw->type, kw->comment) < 0)
		return (-1);
	}

	ctx->synchronized = 0;
	ctx->nhdrpkt++;
	ctx->thdrpkt++;
	return (0);
}


/* WritePixelData -- Write a block of pixel data to the output image.  The
 * format of the input pixel data block is defined by the stream descriptors.
 * Each defines a mapping from the input pixel data block to a region of the
 * output image.  This allows the incoming data block to simultaneously
 * contain data for several different regions of the output image.  The input
 * data may be arbitrarily interleaved, packed, or aliased as determined by
 * the stream descriptors.  
 */
WritePixelData (ctx, nstreams, streams, pixels)
register Context *ctx;
int nstreams;
DCA_Stream *streams;
pointer pixels;
{
	register DCA_Stream *sp;
	register pointer dsim = ctx->dsim;
	int destimage, npix, x0, y0, stream;
	int status=0, totpix, axlen1, axlen2;
	int sv[IMDIM], nv[IMDIM], mv[IMDIM], i;
	pointer io;

	ctx->mode = M_WRITING;
	if (ctx->synchronized) {
	    dprintf (2, "WritePixelData",
		"[%d] begin asynchronous write sequence\n", ctx->seqno);
	}
	dprintf (3, "WritePixelData", "[%d] nstreams=%d\n",
	    ctx->seqno, nstreams);

	/* For each pixel stream in the input buffer extract the pixels and
	 * write them to a rectangular region of the output image.  Each
	 * stream fills entire lines or columns, advancing to the next column
	 * or line as the line or column being written to is filled.
	 */
	for (stream=0, sp=streams;  stream < nstreams;  stream++, sp++) {
	    x0 = sp->x0;
	    y0 = sp->y0;
	    npix = sp->npix;
	    destimage = sp->destimage;
	    axlen1 = mv[0] = dsim_Get (dsim, sp->destimage, DSIM_Axlen1);
	    axlen2 = mv[1] = dsim_Get (dsim, sp->destimage, DSIM_Axlen2);

	    dprintf (4, "WritePixelData",
		"[%d] write stream %d [%d@%d;%d] -> %d[%d,%d;%d,%d;%d] %d %d\n",
		ctx->seqno, stream, sp->npix, sp->offset, sp->stride,
		sp->destimage, sp->x0, sp->y0, sp->xstep, sp->ystep, sp->xydir,
		axlen1, axlen2);

	    /* Perform some basic error checks.
fprintf (stderr, "ERRCHK: npix=%d  (x0,y0) = %d,%d  axlen=%d,%d\n",
    npix, x0, y0, axlen1, axlen2);
fprintf (stderr, "ERRCHK: destimage=%d  xstep=%d  ystep=%d\n",
    destimage, sp->xstep, sp->ystep);
	    */
	    if (!npix || (x0 < 0 || x0 >= axlen1) || (y0 < 0 || y0 >= axlen2)
		|| (destimage < 1) || (sp->xstep == 0) || (sp->ystep == 0)) {

		dprintf (0, "WritePixelData",
		    "[%d] bad parameter for stream %d\n", ctx->seqno, stream);
		ctx->iostat++;
		status = -1;
		continue;
	    }

	    /* Compute the rectangular region of the destination image
	     * which we are writing into.  This is used only to buffer
	     * and/or lock the region and it doesn't have to be exact so
	     * long as all the pixels we will write to are included.
	     */
	    totpix = npix * abs(sp->xstep) * abs(sp->ystep);

	    if (sp->xydir) {
		/* Fill columns.  The i/o region starts at XS,YS and is NX
		 * columns wide and NY=axlen2 lines high, the full height
		 * of the image.  NX is negative if we step to the left
		 * (xstep < 0) as each column fills.
		 */
		sv[0] = sp->x0;
		nv[0] = (totpix + axlen2-1) / axlen2;
		if (sp->xstep < 0)
		    nv[0] = -nv[0];

		nv[1] = axlen2;
		if (sp->ystep > 0)
		    sv[1] = 0;
		else {
		    sv[1] = axlen2 - 1;
		    nv[1] = -nv[1];
		}

	    } else {
		/* Fill lines.  The i/o region starts at XS,YS and is NY lines
		 * high each of which is NX=axlen1 pixels long, the full width
		 * of the image.  NY is negative if we step down (ystep < 0) 
		 * as each line fills.
		 */
		sv[1] = sp->y0;
		nv[1] = (totpix + axlen1-1) / axlen1;
		if (sp->ystep < 0)
		    nv[1] = -nv[1];

		nv[0] = axlen1;
		if (sp->xstep > 0)
		    sv[0] = 0;
		else {
		    sv[0] = axlen1 - 1;
		    nv[0] = -nv[0];
		}
	    }

	    /* Normalize the i/o region.  Convert SV,NV, where NV can be
	     * negative (as computed above), to the equivalent region where
	     * SV refers to the lower left corner and NV is always positive.
	     * Clip the i/o region at the image edge since our computation
	     * above may not be exact.
	     */
	    for (i=0;  i < 2;  i++) {
		if (nv[i] < 0) {
		    nv[i] = -nv[i];
		    sv[i] = sv[i] - nv[i] + 1;
		    if (sv[i] < 0)
			sv[i] = 0;
		}
		if (sv[i] + nv[i] - 1 >= mv[i])
		    --nv[i];
	    }

	    /* Get the i/o region. */
	    io = dsim_StartIO (dsim, destimage, DSO_RDWR, sv, nv);
	    if (!io) {
		dprintf (0, "dsim_StartIO",
		    "[%d] could not get region for stream %d\n",
		    ctx->seqno, stream);
		ctx->iostat++;
		status = -1;
		continue;
	    }

	    /* Copy the pixel stream to the output image. */
	    if (WritePixels (dsim, io, axlen1, axlen2,
		    pixels, npix, sp->offset, sp->stride,
		    sp->x0, sp->y0, sp->xstep, sp->ystep, sp->xydir) < 0) {

		dprintf (0, "WritePixelData",
		    "[%d] xstep or ystep is zero for stream %d\n",
		    ctx->seqno, stream);
		ctx->iostat++;
		status = -1;
	    }

	    /* Release the i/o region. */
	    if (dsim_FinishIO (io) < 0) {
		dprintf (0, "dsim_FinishIO",
		    "[%d] error releasing i/o region for stream %d\n",
		    ctx->seqno, stream);
		ctx->iostat++;
		status = -1;
	    }
	}

	ctx->synchronized = 0;
	ctx->npixpkt++;
	ctx->tpixpkt++;
	return (status);
}


/* WritePixels -- Copy a stream of pixels from a linear input buffer to the
 * output 2D image.  Assuming large images this is the inner loop for the DCA,
 * along with moving data out of the message buffers.
 */
WritePixels (dsim, io, axlen1, axlen2,	/* output image and i/o region */
	pixels, npix, offset, stride,	/* linear input buffer */
	x0, y0, dx, dy, xydir)		/* output pixels to be written */

pointer dsim, io;
int	axlen1, axlen2;
pointer pixels;
int	npix, offset, stride;
int	x0, y0, dx, dy, xydir;
{
	register PIXTYPE *ip, *op;
	register int istep, ostep, n;
	int nleft, np, i, j;

/*
fprintf (stderr, "WritePix: axlen=%d,%d  npix=%d offset=%d stride=%d\n",
    axlen1, axlen2, npix, offset, stride);
fprintf (stderr, "        : x0=%d y0=%d dx=%d dy=%d  xydir=%d\n",
    x0, y0, dx, dy, xydir);
*/
	ip = (PIXTYPE *) pixels + offset;
	op = (PIXTYPE *) dsim_Pixel2D (io, i=x0, j=y0);

	if (xydir && dy > 0) {
	    /* Fill columns in the +y direction starting at x0,y0.
	     */
	    istep = stride;
	    ostep = dy * axlen1;
	    for (nleft=npix;  nleft > 0;  nleft -= np) {
		np = min (nleft, (axlen2 - j) / abs(dy));
		for (n = np;  --n >= 0;  ip += istep, op += ostep)
		    *op = *ip;
		op = (PIXTYPE *) dsim_Pixel2D (io, i=i+dx, j=0);
	    }

	} else if (xydir && dy < 0) {
	    /* Fill columns in the -y direction starting at x0,y0.
	     */
	    istep = stride;
	    ostep = dy * axlen1;
	    for (nleft=npix;  nleft > 0;  nleft -= np) {
		np = min (nleft, (j + 1) / abs(dy));
		for (n = np;  --n >= 0;  ip += istep, op += ostep)
		    *op = *ip;
		op = (PIXTYPE *) dsim_Pixel2D (io, i=i+dx, j=axlen2-1);
	    }

	} else if (!xydir && dx > 0) {
	    /* Fill lines in the +x direction starting at x0,y0.
	     */
	    istep = stride;
	    ostep = dx;
	    for (nleft=npix;  nleft > 0;  nleft -= np) {
		np = min (nleft, (axlen1 - i) / abs(dx));
		for (n = np;  --n >= 0;  ip += istep, op += ostep)
		    *op = *ip;
		op = (PIXTYPE *) dsim_Pixel2D (io, i=0, j=j+dy);
	    }

	} else if (!xydir && dx < 0) {
	    /* Fill lines in the -x direction starting at x0,y0.
	     */
	    istep = stride;
	    ostep = dx;
	    for (nleft=npix;  nleft > 0;  nleft -= np) {
		np = min (nleft, (i + 1) / abs(dx));
		for (n = np;  --n >= 0;  ip += istep, op += ostep)
		    *op = *ip;
		op = (PIXTYPE *) dsim_Pixel2D (io, i=axlen1-1, j=j+dy);
	    }

	} else
	    return (-1);

	return (0);
}


/*
 * Internal routines.
 * ------------------
 */

/* setContext -- Allocate a context descriptor, used to store the context
 * for an imagefile readout.
 */
Context *
setContext (seqno)
int seqno;
{
	register Context *ctx;
	register int i;

	if (!seqno)
	    return (NULL);

	for (i=0;  i < MAX_CONTEXT;  i++) {
	    ctx = &dca_context[i];
	    if (!ctx->seqno) {
		memset ((void *)ctx, 0, sizeof(Context));
		ctx->seqno = seqno;
		ctx->maxkw_global = maxkw_global;
		ctx->maxkw_image = maxkw_image;
		strcpy (ctx->ktm, defktm);
		return (last_context = ctx);
	    }
	}

	return (NULL);
}


/* getContext -- Get a context descriptor given the sequence number.
 */
Context *
getContext (seqno)
register int seqno;
{
	register int i;

	dprintf (3, "getContext", "[%d] seqno=%d\n", seqno, seqno);

	if (last_context && last_context->seqno == seqno)
	    return (last_context);

	for (i=0;  i < MAX_CONTEXT;  i++)
	    if (dca_context[i].seqno == seqno)
		return (last_context = &dca_context[i]);

	return (NULL);
}


/* freeContext -- Free a context descriptor given the sequence number.
 */
freeContext (seqno)
int seqno;
{
	register Context *ctx;
	register int i, j;

	dprintf (3, "freeContext", "[%d] seqno=%d\n", seqno, seqno);

	for (i=0;  i < MAX_CONTEXT;  i++) {
	    ctx = &dca_context[i];
	    if (ctx->seqno == seqno) {
		for (j=0;  j < ctx->nkwdb;  j++)
		    kwdb_Close (ctx->kwdb_list[j]);
		ctx->nkwdb = 0;
		if (ctx->dsim) {
		    dsim_Close (ctx->dsim);
		    ctx->dsim = NULL;
		}
		if (last_context == ctx)
		    last_context = NULL;
		ctx->seqno = 0;
		return (0);
	    }
	}

	return (-1);
}


/* GetKWDB -- Get the kwdb handle for the named keyword group.  A new group
 * is added if the named group is not found.
 */
pointer
GetKWDB (ctx, kwgroup)
register Context *ctx;
char *kwgroup;
{
	register pointer kwdb;
	register int i;

	for (i=0;  i < ctx->nkwdb;  i++) {
	    kwdb = ctx->kwdb_list[i];
	    if (!strcmp (kwgroup, kwdb_Name(kwdb))) { 
		dprintf (3, "GetKWDB", "[%d] kwgroup='%s' name='%s' MATCHED\n",
    		    ctx->seqno, kwgroup, kwdb_Name(kwdb));
		return (kwdb);
	    }
	}

	if (ctx->nkwdb + 1 >= MAX_KWDB)
	    return (NULL);
	if (!(kwdb = kwdb_Open (kwgroup)))
	    return (NULL);

	return (ctx->kwdb_list[ctx->nkwdb++] = kwdb);
}


/* PrintKWDB -- Print the contents of a keyword database.
 */
PrintKWDB (kwdb, name, level)
pointer kwdb;
char *name;
int level;
{
	char *keyword, *value, *type, *comment;
	int ep;

	for (ep=kwdb_Head(kwdb);  ep;  ep=kwdb_Next(kwdb,ep)) {
	    if ((kwdb_GetEntry(kwdb,ep,&keyword,&value,&type,&comment)) < 0)
		return (-1);
	    dprintf (level, name, "%8s %s %s / %s\n",
		keyword, type, value, comment);
	}

	return (0);
}


/* MakeFileName -- Make the imagefile filename.  There are lots of possible
 * ways to do this.  The default method provided here is to use the imformat
 * format to generate the filename.  imformat may be any printf-style string.
 * Within this format the following special sequences are defined:
 *
 *	%N	the imagename field from the StartReadout message
 *	%T	the imagetype field from the StartReadout message
 *	%I	the value of "imindex"; incremented for each new image
 *
 * In the case of %I the actual format is %d.  If a digit follows the %
 * then any format can be given in place of the `I', e.g. "%03d" would 
 * print a three digit decimal field.
 *
 * Filename generation depends upon the context given in ctx.  The output
 * filename is stored in the context descriptor.
 */
MakeFileName (ctx, imformat, imindex)
register Context *ctx;
char *imformat;
int *imindex;
{
	register char *ap, *ip, *op;
	char buf[80], fmt[80];

	bzero (buf, 80);
	bzero (fmt, 80);
	    
	dprintf (1, "MakeFileName", "imformat=%s  imindex=%d\n", 
	    imformat, *imindex);

	for (ip=imformat, op=ctx->fname;  *ip;  ) {
	    if (*ip == '%') {
		switch (*(ip+1)) {
		case 'N':
		    for (ap=ctx->imagename;  *op = *ap++;  op++)
			;
		    ip += 2;
		    break;
		case 'T':
		    for (ap=ctx->imagetype;  *op = *ap++;  op++)
			;
		    ip += 2;
		    break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'I':
		    /* %dddI */
		    for (ap=fmt, *ap++ = *ip++;  isdigit(*ip) || *ip == '-';  )
			*ap++ = *ip++;
		    *ap++ = (*ip == 'I') ? 'd' : *ip;  ip++;
		    *ap++ = '\0';
		    sprintf (buf, fmt, (*imindex)++);

		    for (ap=buf;  *op = *ap++;  op++)
			;
		    break;

		default:
		    *op++ = *ip++;
		    break;
		}
	    } else
		*op++ = *ip++;
	}

	*op++ = '\0';
	
	dprintf (0, "MakeFileName", "ctx->fname='%s'\n", ctx->fname);
}

 
 
/* RUNTCL -- Execute a Tcl script.
 */
RunTcl (tcl, fname, seqno)
Tcl_Interp *tcl;
char *fname;
int seqno;
{
	int status;

	dprintf (2, "RunTcl", "[%d] execute script %s\n", seqno, fname);
	status = Tcl_EvalFile (tcl, fname);

	if (status == TCL_OK) {
	    dprintf (2, "RunTcl",
		"[%d] script completed with no errors\n", seqno);
	} else {
	    dprintf (0, "RunTcl", "[%d] error on line %d: %s\n", seqno,
		tcl->errorLine, tcl->result);
	}

	return (status == TCL_OK ? 0 : -1);
}


/* DPRINTF -- Format and print a debug message.
 *
 *   Usage: dprintf (level, name, fmt, args)
 *
 * The output line will be of the form "name: text", where text is the
 * string generated by executing a printf using fmt and args.  If name=NULL
 * the only the formatted text is output, e.g. to continue printing on the
 * same line of output.  Output is generated only if the current debug level
 * equals or exceeds the level of the message.  The output is automatically
 * flushed for level=1 messages, higher level messages are buffered.
 */

char *
dprintf (int level, char *name, char *fmt, ...)
{
	va_list args;


	va_start (args, fmt);

	if (debug >= level) {
	    if (name) {
		/* Print the debug level. */
		(void) fprintf (debug_out, "%d ", level);

		/* Print name of function which output is from. */
		(void) fprintf (debug_out, "%s: ", name);
	    }

	    /* Print out remainder of message. */
	    (void) vfprintf (debug_out, fmt, args);

	    if (level <= 1)
		fflush (debug_out);
	}
	va_end (args);
}


/* TCL_DPRINTF -- Tcl interface to the DCA dprintf command, used for debug
 * and error output.
 *
 * Usage:	dprintf level name format [args ...]
 *
 * Level is the integer debug level of the message.  Name is the message name
 * prefix.
 */
static int
tcl_dprintf (client_data, tcl, argc, argv)
void *client_data;
Tcl_Interp *tcl;
int argc;
char **argv;
{
	char *name, *format;
	int level, o;

	if (argc < 4)
	    return (TCL_ERROR);

	level = atoi (argv[1]);
	name = argv[2];
	format = argv[3];
	o = 4;

	switch (argc) {
	case 4:
	    dprintf (level, name, format);
	    break;
	case 5:
	    dprintf (level, name, format, argv[o]);
	    break;
	case 6:
	    dprintf (level, name, format, argv[o], argv[o+1]);
	    break;
	case 7:
	    dprintf (level, name, format, argv[o], argv[o+1], argv[o+2]);
	    break;
	case 8:
	    dprintf (level, name, format, argv[o], argv[o+1], argv[o+2],
		argv[o+3]);
	    break;
	case 9:
	    dprintf (level, name, format, argv[o], argv[o+1], argv[o+2],
		argv[o+3], argv[o+4]);
	    break;
	default:
	    return (TCL_ERROR);
	}

	return (TCL_OK);
}


/* DCA_REGISTER -- Register the server name with the message bus so that other
 * tasks can determine its tid.
 */
static int
dca_register (name, tid)
char *name;
int tid;
{
	int cc, rtid;

	/* Delete any existing registry entries.  This will shutdown and
	 * unregister any other DCA servers on the message bus (only one
	 * active DCA is currently permitted).
	 */
	if ((cc = pvm_lookup (name, -1, &rtid)) >= 0) {
	    /* Already registered? */
	    if (rtid == tid)
		return (0);

	    /* Unregister any other DCA servers. */
	    do {
		dprintf (2, "dca_register",
		    "delete registry %s=%d\n", name, rtid);
		pvm_initsend (encoding);
		pvm_send (rtid, DCA_Quit);
		pvm_delete (name, cc);
	    } while ((cc = pvm_lookup (name, -1, &rtid)) >= 0);
	}

	cc = pvm_insert (name, -1, tid);
	if (cc < 0) {
	    pvm_perror ("dca_register(pvm_insert)");
	    return (cc);
	}

	return (0);
}


/* DCA_UNREGISTER -- Unregister the DCA server.
 */
static int
dca_unregister (name, tid)
char *name;
int tid;
{
	int cc, mosdca;

	while ((cc = pvm_lookup (name, -1, &mosdca)) >= 0) {
	    if (mosdca == tid) {
		if (pvm_delete (name, cc) < 0) {
		    pvm_perror ("dca_unregister(pvm_delete)");
		    return (-1);
		}
	    }
	}

	return (0);
}


/* STREQ -- Case insensitive string compare.
 */
static int
streq (s1, s2)
register char *s1, *s2;
{
	register int c1, c2;

	for (;;) {
	    c1 = *s1++;
	    c2 = *s2++;
	    if (!c1 || !c2)
		break;

	    if (isupper(c1))
		c1 = tolower(c1);
	    if (isupper(c2))
		c2 = tolower(c2);
	    if (c1 != c2)
		break;
	}

	return (!c1 && !c2);
}


/* ENCODE_I4 -- Encode a 4 byte integer as a byte sequence.
 */
static int
encode_i4 (val)
int val;
{
	int oval = 0;
	register unsigned char *op = (unsigned char *) &oval;

	*op++ = ((val      ) & 0377);
	*op++ = ((val >>  8) & 0377);
	*op++ = ((val >> 16) & 0377);
	*op++ = ((val >> 24) & 0377);

	return (oval);
}


/* DEODE_I4 -- Decode a 4 byte integer stored as a byte sequence.
 */
static int
decode_i4 (val)
int val;
{
	register unsigned char *ip = (unsigned char *) &val;
	int oval = 0;

	oval |= ((*ip++)      );
	oval |= ((*ip++) <<  8);
	oval |= ((*ip++) << 16);
	oval |= ((*ip++) << 24);

	return (oval);
}
