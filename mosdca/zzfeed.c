#include <stdio.h>
#include <pvm3.h>
#include <sys/types.h>
#include "dso.h"
#include "mosdca.h"

/*
 * ZZFEED -- Test utility to feed a fake image to the DCA.
 *
 *  Usage:	zzfeed [-name imagename] [-type imagetype]
 *		       [-nimage nimage] [-size nx ny] [-seqno seqno]
 *		       [-direct] [-synch interval] [-buflen nbytes]
 *		       [-verbose]
 *
 * zzfeed will parse the argument list to determine the attributes of the
 * test image, then connect to the message bus and download the image.
 */

/* typedef unsigned short ushort; */

#define	DEF_SZDATABUF 32768
int buflen = DEF_SZDATABUF;
ushort *databuf;

/* Default arguments. */
char *imagename = "testimage";
char *imagetype = "notype";
int verbose = 0;
int direct = 0;
int synch = 20;
int pixtype = DSO_USHORT;
int naxes = 2;
int nimage = 4;
int nx = 1024;
int ny = 1024;
int extend = 0;
int seqno = 0;


/* ZZFEED_MAIN -- The main program.
 */
main (argc, argv)
int argc;
char **argv;
{
	int mytid, mosdca;
	int mode, info, i;

	/* Attach to the message bus (PVM currently). */
	if ((mytid = pvm_mytid()) < 0) {
	    pvm_perror ("zzfeed");
	    return (-1);
	} else
	    seqno = mytid;

	/* Process any command line arguments.
	 */
	for (i=1;  i < argc;  i++) {
	    char *argp = argv[i];

	    if (       !strcmp (argp, "-direct")) {
		direct++;
	    } else if (!strcmp (argp, "-verbose")) {
		verbose++;
	    } else if (!strcmp (argp, "-synch")) {
		synch = atoi (argv[++i]);
	    } else if (!strcmp (argp, "-buflen")) {
		buflen = atoi (argv[++i]);
	    } else if (!strcmp (argp, "-name")) {
		imagename = argv[++i];
	    } else if (!strcmp (argp, "-type")) {
		imagetype = argv[++i];
	    } else if (!strncmp(argp, "-nimage", 4)) {
		nimage = atoi (argv[++i]);
	    } else if (!strcmp (argp, "-size")) {
		nx = atoi (argv[++i]);
		ny = atoi (argv[++i]);
	    } else if (!strcmp (argp, "-seqno")) {
		seqno = atoi (argv[++i]);
	    } else {
		fprintf (stderr, "zzfeed: unknown switch %s\n", argp);
	    }
	}

	if (!(databuf = (ushort *) malloc (buflen * sizeof(ushort)))) {
	    fprintf (stderr, "cannot allocate data buffer\n");
	    exit (1);
	}

	if (direct)
	    pvm_setopt (PvmRoute, PvmRouteDirect);

	printf ("starting zzfeed client, pid=%d tid=%d buf=%d %s\n",
	    getpid(), mytid, buflen, direct ? " direct routing" : "");
	fflush (stdout);

	/* Locate the DCA.  */
	info = pvm_lookup (MOSDCA, 0, &mosdca);
	if (info == PvmNoEntry) {
	    fprintf (stderr, "cannot locate the DCA server\n");
	    return (-1);
	} else
	    fprintf (stderr, "mosdca server found at tid=%d\n", mosdca);

	/* Verify that we can talk to the server. */
	if (GetMode(mosdca) == M_INACTIVE)
	    fprintf (stderr, "communication with mosdca verified\n");
	else {
	    fprintf (stderr, "communication with mosdca fails\n");
	    return (-1);
	}

	/* Download the test data.  */
	SendTestData (mytid, mosdca);

	pvm_exit();
	return (0);
}


/* SendTestData -- Send the test data set to the DCA.
 */
SendTestData (mytid, mosdca)
int mytid, mosdca;
{
	int status, i;

	/* Send StartReadout.
	 */
	fprintf (stderr, "start readout...\n");

	pvm_initsend (PvmDataDefault);
	pvm_pkint (&seqno, 1, 1);
	pvm_pkstr (imagename);
	pvm_pkstr (imagetype);
	pvm_send (mosdca, DCA_StartReadout);

	if (GetStatus(mosdca,DCA_StartReadout) < 0) {
	    fprintf (stderr, "start readout fails\n");
	    return (-1);
	}

	/* Send ConfigureImage.
	 */
	fprintf (stderr, "configure image...\n");

	pvm_initsend (PvmDataDefault);
	pvm_pkint (&seqno, 1, 1);
	pvm_pkstr (imagename);
	pvm_pkstr (imagetype);
	pvm_pkint (&nimage, 1, 1);

	for (i=0;  i < nimage;  i++) {
	    pvm_pkint (&pixtype, 1, 1);
	    pvm_pkint (&naxes, 1, 1);
	    pvm_pkint (&nx, 1, 1);
	    pvm_pkint (&ny, 1, 1);
	    pvm_pkint (&extend, 1, 1);
	    pvm_pkstr ("");
	    pvm_pkstr ("");
	}
	pvm_send (mosdca, DCA_ConfigureImage);

	if (GetStatus(mosdca,DCA_ConfigureImage) < 0) {
	    fprintf (stderr, "configure image fails\n");
	    return (-1);
	}

	/* Send the pixel data. 
	 */
	fprintf (stderr, "send pixel data ...\n");
	SendPixels (mytid, mosdca, nimage, nx, ny);
	fprintf (stderr, "done sending pixel data\n");

	/* Wind it up.
	 */
	fprintf (stderr, "end readout...\n");

	pvm_initsend (PvmDataDefault);
	pvm_pkint (&seqno, 1, 1);
	pvm_send (mosdca, DCA_EndReadout);

	status = GetStatus (mosdca, DCA_EndReadout);
	if (status < 0) {
	    fprintf (stderr, "end readout fails (mode = %d)\n", status);
	    return (-1);
	} else
	    fprintf (stderr, "end readout successful\n");

	return (0);
}


/* SendPixels -- Send fake pixel array data to the DCA.
 */
SendPixels (mytid, mosdca, nimage, nx, ny)
int mytid, mosdca;
int nimage, nx, ny;
{
	static int datalen;
	static DCA_Stream st[32];
	int nstreams = nimage;
	int swapped = 0;
	register int dy, y, i, j, v;
	int nout, nsync;

	{   static int initialized;
	    if (!initialized) {
		for (i = 0;  i < buflen;  i += 26)
		    for (j=0;  j < 26;  j++) {
			v = j + 'A';
			if (i + j < buflen)
			    databuf[i+j] = (v << 8 | v);
		    }
		initialized++;
	    }
	}

	/* Get chunk size, an integral number of lines. */
	dy = buflen / (nimage * nx);
	if (dy <= 0) {
	    fprintf (stderr, "SendPixels: out of space (zzfeed buf)\n");
	    return;
	}

	/* Send in each data buffer dy lines of each of the nimage images.
	 */
	for (y=0, nout=nsync=0;  y < ny;  y += dy) {
	    if (verbose) {
		if (!nout)
		    fprintf (stderr, "write pixels at y=");
		fprintf (stderr, "%d ", y);
		if (++nout >= 10) {
		    fprintf (stderr, "\n");
		    nout = 0;
		}
	    }

	    /* Build the write pixels message.
	     */
	    pvm_initsend (PvmDataInPlace);
	    pvm_pkint (&seqno, 1, 1);
	    datalen = nimage * dy * nx * sizeof(ushort);
	    pvm_pkint (&datalen, 1, 1);
	    pvm_pkint (&swapped, 1, 1);
	    pvm_pkint (&nstreams, 1, 1);

	    /* Pack the stream descriptors. */
	    for (i=0;  i < nimage;  i++) {
		st[i].offset = i * nx * dy;
		st[i].npix = nx * dy;
		st[i].stride = 1;
		st[i].destimage = i + 1;
		st[i].x0 = 0;
		st[i].y0 = y;
		st[i].xstep = 1;
		st[i].ystep = 1;
		st[i].xydir = 0;
		pvm_pkint ((int *)&st[i], sizeof(st[i])/sizeof(int), 1);
	    }

	    /* Append the pixel data. */
	    pvm_pkbyte ((char *)databuf, datalen, 1);
	    pvm_send (mosdca, DCA_WritePixelData);

	    /* Sync ever so often to avoid buffer overruns. */
	    if (++nsync >= synch) {
		if (nout && verbose) {
		    fprintf (stderr, "\n");
		    nout = 0;
		}
		fprintf (stderr, "synchronize with server...");
		pvm_initsend (PvmDataDefault);
		pvm_pkint (&seqno, 1, 1);
		pvm_send (mosdca, DCA_Synchronize);
		if (pvm_recv (mosdca, DCA_Synchronize) < 0)
		    fprintf (stderr, " failed\n");
		else
		    fprintf (stderr, " ok\n");
		nsync = 0;
	    }
	}
	if (nout && verbose)
	    fprintf (stderr, "\n");
}


/* GetMode -- Query the server for the current readout mode.
 */
GetMode (mosdca)
int mosdca;
{
	int mode;

	pvm_initsend (PvmDataDefault);
	pvm_pkint (&seqno, 1, 1);
	if (pvm_send (mosdca, DCA_GetStatus) < 0) {
	    fprintf (stderr, "send to mosdca fails\n");
	    return (-1);
	}
	if (pvm_recv (mosdca, DCA_GetStatus) < 0) {
	    fprintf (stderr, "recv from mosdca fails\n");
	    return (-1);
	} else
	    pvm_upkint (&mode, 1, 1);

	return (mode);
}


/* GetStatus -- Get a status packet.
 */
GetStatus (mosdca, tag)
int mosdca;
int tag;
{
	int status;

	if (pvm_recv (mosdca, tag) < 0) {
	    fprintf (stderr, "recv from mosdca fails\n");
	    return (-1);
	} else
	    pvm_upkint (&status, 1, 1);

	return (status);
}
