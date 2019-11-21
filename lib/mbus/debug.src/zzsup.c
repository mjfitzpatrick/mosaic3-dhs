#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef __POSIX__
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stddef.h>

#include <pvm3.h>

#include "mbus.h"


#define  DEBUG		0
#define  SELWIDTH	32
#define  SZ_LINE	1024


int	hostAdded();

int main (int argc, char **argv)
{
    int tid = 0;
    int nfds = -1, *fds = NULL, i;
    char *whoAmI = (argc > 1 ? argv[1] : SUPERVISOR);


    /* Connect application to the bus. */
    tid = mbusConnect (whoAmI, "super", TRUE);

    if (DEBUG) printf ("%s:  tid = %d\n", whoAmI, tid);

    testListener();

    printf ("Disconnecting....\n");
    mbusDisconnect (tid);
}




testListener ()
{
    int     i, n, *pvm_fds, nfds;
    int     bufid, info, size;
    char    buf[2048];
    fd_set  fds, allset;

    char    *from=NULL, *to=NULL, *host=NULL, *msg=NULL;
    int	    from_tid, to_tid, subject;
    int     my_tid = mbAppGet(APP_TID);
		

    if ((nfds = pvm_getfds (&pvm_fds)) == 0) {
        fprintf (stderr, "Not connected to message bus!\n"); 
	return (-1);

    } else if (DEBUG) {
        printf ("nfds = %d\t", nfds);
        for (i=0; i < nfds; i++)
            printf ("fd[%d] = %d\n", i, pvm_fds[i]);
    }


    FD_ZERO (&allset);
    FD_SET (pvm_fds[0], &allset);
    FD_SET (fileno(stdin), &allset);
    while (1) {
        fds = allset;
        bzero (buf, 2048);

	printf ("Waiting for messages...\n");
        if ((n = select (SELWIDTH, &fds, NULL, NULL, NULL)) > 0) {

            if (FD_ISSET(pvm_fds[0], &fds)) {

		to_tid = subject = -1;
		if (mbusRecv (&from_tid, &to_tid, &subject, &host, &msg) < 0) {
		    printf ("Error in mbusRecv()\n");
		}

		/* See if message was meant specifically for me.... */
		if (to_tid == my_tid && subject != MB_ERR) {
		    supHandler (from_tid, subject, msg);

		} else {
		    printf ("Monitor...\n");
		    printf ("   from:%d\n   to:%d\n   subj:%d\n",
		        from_tid, to_tid, subject);
		    printf ("   host:'%s'\n   msg='%s'\n", host, msg);
		}



            } else if (FD_ISSET(fileno(stdin), &fds)) {

                n = read (fileno(stdin), buf, 2048);
		buf[strlen(buf)-1] = NULL;		/* kill newline  */
                if (DEBUG) printf ("stdin: n=%d  msg='%s'\n", n, buf);
                if (n <= 0)
		    break;
            }
        }
    }

    return (0);
}

		    
/* Supervisor Message Handler
 */
#define MSG_CONNECT	0
#define MSG_DISCONNECT	1
#define MSG_ORPHAN	2
#define MSG_FAIL	3
#define MSG_ERR		4
#define MSG_STATUS	5

char *msgClass[] = {
     "Connect",
     "Disconnect",
     "Orphan",
     "Fail",
     "Err",
     "Status",
     NULL
};

int
msgCode (char *msg)
{
    int  i, code = ERR;

    if (msg) {
        for (i=0; msgClass[i]; i++) {
	    if (strncasecmp (msg, msgClass[i], strlen(msgClass[i])) == 0) {
	        code = i;
	        break;
	    }
        }
    }
    return (code);
}



supHandler (int from, int subject, char *msg)
{
    int from_tid, to_tid, subj;
    char *txt;

    switch (subject) {
    case MB_CONNECT:
        printf ("MB_CONNECT:%d: %s\n", subject, msg);
	break;
    case MB_EXITING:
        printf ("MB_DISCONNECT:%d: %s\n", subject, msg);
	break;

    case MB_ORPHAN:
        printf ("MB_ORPHAN:%d: ", subject);
	sscanf (msg, "Orphan {From: %d  To: %s  Subj: %d -- (%s)}",
	    &from_tid, &to_tid, &subj, txt);
	break;

    case MB_ERR:
        printf ("MB_ERR:%d:  %s\n", subject, msg);
	break;

    case MB_STATUS:
        printf ("MB_STATUS:%d:  %s\n", subject, msg);
	break;

    default:
        printf ("Super recv:  ");
        printf ("   from:%d  subj:%d\n   msg='%s'\n", from, subject, msg);
    }
}



int hostAdded (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf( "*** %d new hosts just added ***\n", n );
}



int hostExited (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf( "*** %d new hosts just exited ***\n", n );
}



int hostDeleted (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf( "*** %d new hosts just deleted ***\n", n );
}

