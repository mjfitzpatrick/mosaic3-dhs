#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __POSIX__
#include <sys/select.h>
#else
#include <sys/time.h>
/*
#include <sys/types.h>
*/
#endif
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <ctype.h>

#if !defined(_FITSIO_H)
#include "fitsio.h"
#endif


#include "dhsCmds.h"
#include "smCache.h"
#include "mbus.h"
#include "pxf.h"

#define	SZ_BUF		128

char 	buf[SZ_BUF];

extern 	int console, noop;
extern	int seqno, dca_tid;



/**************************************************************************
**  MBUSMSGHANDLER -- Standard client application message handler.
**
*/
int
mbusMsgHandler (int fd, void *data)
{
    char *host = NULL, *msg = NULL;
    int   from_tid, to_tid, subject;
    int   mytid = mbAppGet (APP_TID);


    to_tid = subject = -1;
    if (mbusRecv (&from_tid, &to_tid, &subject, &host, &msg) < 0) {
	if (console)
            fprintf (stderr, "Error in mbusRecv()\n");
        if (host) free ((void *) host);
        if (msg)  free ((void *) msg);
	return (ERR);
    }

    /* See if message was meant specifically for me.... */
    if (to_tid == mytid && subject != MB_ERR) {
	if (console)
            fprintf (stderr, "SENDTO:  from:%d  subj:%d msg='%s'\n",
		from_tid, subject, msg);
        myHandler (from_tid, subject, msg);

    } else if (to_tid < 0) {
	if (console && strcmp (msg, "no-op") != 0)
            fprintf (stderr, "BCAST:  from:%d  subj:%d msg='%s'\n",
		from_tid, subject, msg);
        myHandler (from_tid, subject, msg);

    } else {
	if (console) {
            printf("Monitor...\n");
            printf("   from:%d\n   to:%d\n   subj:%d\n",
		from_tid, to_tid, subject);
            printf("   host:'%s'\n   msg='%s'\n", host, msg);
	}
    }

    if (host) free ((void *) host);
    if (msg)  free ((void *) msg);

    return (OK);
}



/*  Local Message Handler
 */
void
myHandler (int from, int subject, char *msg)
{
    int   tid = 0, pid = 0, mytid = mbAppGet (APP_TID);
    char  *me, who[128], host[128];
    char  *w = who, *h = host;
    double expID;

    extern smCache_t *smc;


    me = (char *)mbAppGetName();

    switch (subject) {
    case MB_CONNECT:
	if (console)
	    fprintf (stderr, "CONNECT on %s: %s\n", me, msg);

	mbParseConnectMsg (msg, &tid, &w, &h, &pid);

	/*  If it's the supervisor connecting, and we don't already have an
	 *  established connection to the Super, set it up now.
	 */
	if (isSupervisor (who) && mbAppGet (APP_STID) < 0) {
	    mytid = mbAppGet(APP_TID);
	    if (mbConnectToSuper (mytid, tid, host, pid) == OK)
		mbAppSet (APP_TID, abs(mytid));
	}

	/*  When we get a CONNECT message, post a notifier so we're
	 *  alerted whent the task exits.
	mbAddTaskExitHandler (tid, myTaskExited);
	 */

	break;

    case MB_START:
	if (console) 
	    fprintf (stderr, "START on %s: %s\n", me, msg);

	if (strncmp (msg, "process", 7) == 0) {
	    /*  Begin processing an image given the ExpID.
	    */
	    expID = (double) atof (&msg[8]);
	    if (console)
	    	fprintf (stderr, "PXF processing ExpID %.6lf\n", expID);

	    /* Process the pages.
	    */
	    if (!noop)
	        pxfProcess (smc, expID);

	    dhsTransferComplete (dca_tid, seqno);

	    memset (buf, 0, SZ_BUF);
            sprintf (buf, "process pxf done %.6lf", expID);
            mbusSend (SUPERVISOR, ANY, MB_SET, buf);
            mbusSend (SUPERVISOR, ANY, MB_STATUS, "inactive");
            mbusSend (SUPERVISOR, ANY, MB_STATUS, "Ready...");

            mbusSend (SUPERVISOR, ANY, MB_ACK, "");
            mbusSend (SUPERVISOR, ANY, MB_ACK, "");
            mbusSend (SUPERVISOR, ANY, MB_ACK, "");

	    if (console) {
		fprintf (stderr, "\n");
		fprintf (stderr, "**************************************\n");
		fprintf (stderr, "PXF processing Done: ExpID %.6lf\n", expID);
		fprintf (stderr, "**************************************\n\n");
	    }
	}
	break;

    case MB_SET:
	if (console && strncmp (msg, "no-op", 5) != 0)
	    fprintf (stderr, "SET on %s: %s\n", me, msg);

	if (strncmp (msg, "dca_tid", 7) == 0) {
	    dca_tid = atoi (&msg[8]);

	} else if (strncmp (msg, "nbin", 4) == 0) {
	    /*
	    nbin = atoi (&msg[5]);
	    */

	} else if (strncmp (msg, "seqno", 5) == 0) {
	    seqno = atoi (&msg[6]);

	} else if (strncmp (msg, "no-op", 5) == 0) {
	    /*
	    mbusSend (SUPERVISOR, ANY, MB_ACK, "");
	    */
		;

        } else if (strncmp (msg, "keyword add", 11) == 0) {
            char *sp, *op, *ip = &msg[12];
	    char buf[SZ_LINE];

	    /* printf ("Adding keyword monitor: '%s'\n", ip); */
	    while (*ip) {
		/* skip leading whitespace */
		for (sp=ip; *ip && isspace(*sp); sp++) ip++;

		/* Copy the keyword to the list.
		*/
		bzero (buf, SZ_LINE);
		for (op=&buf[0]; *ip && !isspace (*ip); ) *op++ = *ip++;
		if (! keywList[NKeywords])
		    keywList[NKeywords] = malloc (SZ_LINE);
		strcpy (keywList[NKeywords++], buf);
	    }

        } else if (strncmp (msg, "keyword del", 11) == 0) {
            char *key = &msg[12];
            int  i, j;

            for (i=0; i < NKeywords; i++) {
                if (strcmp (keywList[i], key) == 0) {
                    for (j=i+1; j < NKeywords; )
                        strcpy (keywList[i++], keywList[j++]);
                    NKeywords--;
                    break;
                }
            }
        }
	break;

    case MB_EXITING:
	/*  If it's the supervisor disconnecting, .....
	 */
	if (console) 
	    fprintf (stderr, "DISCONNECT on %s: %s\n", me, msg);
	break;

    case MB_STATUS:
	/* Send a Status response.... */
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "Ready...");
	break;

    case MB_PING:
	/* Return an ACK/STATUS to sender .... */
        mbusSend (SUPERVISOR, ANY, MB_ACK, "");
	break;

    case MB_DONE: 				/* Exit for now... */
	exit (0);
	break;

    case MB_ERR: 				/* No-op */
	break;

    default:
	if (console) {
	    fprintf (stderr, "DEFAULT recv:%d:  ", subject);
	    fprintf (stderr, "   from:%d  subj:%d\n   msg='%s'\n",
		from, subject, msg);
	}
	if (strncmp (msg, "quit", 4) == 0)
	    exit (0);
    }

    return;
}
