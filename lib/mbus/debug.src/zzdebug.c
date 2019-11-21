
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


#include "mbus.h"

#define  DEBUG		0
#define  SELWIDTH	32


char 	*mytok();
int	pvm_handle   (int fd, void *client_data);
int	stdin_handle (int fd, void *client_data);

int 	myTaskExited (int mid);
int 	myHostAdded (int mid);
int 	myHostDeleted (int mid);


int main(int argc, char **argv)
{
    int    mb_fd, user_fd;

    if (argc == 1 || strcmp (argv[1], "-s") == 0) {
	printf ("Starting as Super....\n");
	inputEventHandler ("super up");

    } else if (strcmp (argv[1], "-c") == 0) {
	printf ("Starting as Client....\n");
	inputEventHandler ("client up");

    } else {
	fprintf (stderr, "Unknown option '%s'\n", argv[1]);
	exit (1);
    }


    if ((mb_fd = mbGetMBusFD()) >= 0) 		/* Add the input handlers. */
	mbusAddInputHandler (mb_fd, pvm_handle, NULL);
    else
	perror ("Error adding MBUS input handler.");

    user_fd = fileno(stdin);			/* should be zero */
    mbusAddInputHandler (user_fd, stdin_handle, NULL);


    mbusAppMainLoop (NULL); 			/* Never returns.... */
}


char *mytok(char *str, int N)
{
    register int i, ntok = abs(N);
    char *ip = strdup(str), buf[1024];

    if (!str) return (NULL);

    while (isspace(*ip++));			/* skip leading whitespace */

    for (i = 0; *ip && i < (N - 1); i++) { 	/* find requested token    */
	while (!isspace(*ip++));
	while (isspace(*ip++));
    }
    ip--;

    strcpy (buf, ip);				/* terminate after token   */
    for (i=0; buf[i] != '\n' && !isspace(buf[i]); i++)
	;
    buf[i] = '\0';

/*  if (ip) free (ip); */
    return ( strdup (buf) );
}


int pvm_handle (int fd, void *data)
{
    char *from = NULL, *to = NULL, *host = NULL, *msg = NULL;
    int  from_tid, to_tid, subject;
    int  mytid = mbAppGet(APP_TID);


    to_tid = subject = -1;
    if (mbusRecv(&from_tid, &to_tid, &subject, &host, &msg) < 0) {
        printf("Error in mbusRecv()\n");
	return (ERR);
    }

    /* See if message was meant specifically for me.... */
    if (to_tid == mytid && subject != MB_ERR) {
        myHandler (from_tid, subject, msg);

    } else if (to_tid < 0) {
        printf ("BCAST:  from:%d  subj:%d msg='%s'\n", from_tid, subject, msg);
        myHandler (from_tid, subject, msg);

    } else {
        printf("Monitor...\n");
        printf("   from:%d\n   to:%d\n   subj:%d\n", from_tid, to_tid, subject);
        printf("   host:'%s'\n   msg='%s'\n", host, msg);
    }

    return (OK);
}


int stdin_handle (int fd, void *data)
{
    int n;
    char buf[2048];
    int  mytid = mbAppGet(APP_TID);


    n = read(fd, buf, 2048);		/* already saved fileno(stdin)	*/

    buf[strlen(buf) - 1] = NULL;	/* kill newline  		*/
    if (DEBUG)
        printf("stdin: n=%d  msg='%s'\n", n, buf);

    if (n <= 0 || strncmp (buf, "quit", 4) == 0) {
	mbusDisconnect (mytid);
        exit (0);

    } else {
	/* Pass off to our main message handler.
	 */
        inputEventHandler (buf);
    }

    return (OK);
}




/*  Local Message Handler
 */
myHandler(int from, int subject, char *msg)
{
    int   from_tid, to_tid, subj;
    int   tid = 0, pid = 0, status;
    int   mytid = mbAppGet (APP_TID);
    char  *txt, *me = mbAppGetName();
    char  who[128], host[128];


    switch (subject) {
    case MB_CONNECT:
	printf("CONNECT on %s: %s\n", me, msg);

	mbParseConnectMsg (msg, &tid, &who, &host, &pid);

	/*  If it's the supervisor connecting, and we don't already have an
	 *  established connection to the Super, set it up now.
	 */
	if (isSupervisor (who) && mbAppGet (APP_STID) < 0) {
printf ("B4 connect, app_tid = %d\n", mbAppGet(APP_TID));
	    mytid = mbAppGet(APP_TID);
	    if (mbConnectToSuper (mytid, tid, host, pid) == OK)
		mbAppSet (APP_TID, abs(mytid));
printf ("After connect, app_tid = %d\n", mbAppGet(APP_TID));
	}

	/*  When we get a CONNECT message, post a notifier so we're
	 *  alerted whent the task exits.
	 */
	mbAddTaskExitHandler (tid, myTaskExited);
	break;

    case MB_EXITING:
	printf("DISCONNECT on %s: %s\n", me, msg);
	break;

    case MB_STATUS:
	printf("STATUS on %s: %s\n", me, msg);
	break;

    case MB_ORPHAN:
	printf("ORPHAN on %s: ", me);
	sscanf(msg, "Orphan {From: %d  To: %s  Subj: %d -- (%s)}",
	       &from_tid, &to_tid, &subj, txt);
	break;

    case MB_PING:
	printf("PING on %s: %s\n", me, msg);
	break;

    case MB_ERR:
	printf("ERR on %s: %s\n", me, msg);
	break;

    default:
	printf("Super recv:%d:  ", subject);
	printf("   from:%d  subj:%d\n   msg='%s'\n", from, subject, msg);
    }
}


bob () { int i = 0; }


/* Handle a command given to us from the commandline.
 */
inputEventHandler (char *cmd)
{
    char   c, host[64], buf[64], resp[32], val[12], config[128];
    char   cmdin[128], *tok = NULL, *group = NULL;
    char   *arg1=NULL, *arg2=NULL, *arg3=NULL;
    int    i, nread=0,  stid=0, ctid=0;


    /* Command loop */
    printf("zzdebug (%d)>  ", getpid());


    /*if (DEBUG) printf ("cmd entered: '%s'\n", cmd); */

    bzero (cmdin, 128);
    strcpy(cmdin, cmd);
    if (cmd[0] == '?') {	/* HELP    */
	print_help();

    } else if (strncmp(cmd, "init", 3) == 0) {		/* INIT    */
	printf("Initializing...");
	mbusInitialize(SUPERVISOR, NULL);
	printf("done.");

    } else if (strncmp(cmd, "super", 3) == 0) {		/* SUPER  */
	if (tok = mytok(cmd, 2)) {
	    if (tok[0] == 'u') {
		printf("Connecting as Supervisor...");
		stid = mbusConnect(SUPERVISOR, "super", TRUE);
		printf("Super:  stid = %d\n", stid);

	    } else if (tok[0] == 'd') {
		printf("Disonnecting as Supervisor...");
		mbusDisconnect(stid);

	    } else {
		printf("Invalid token '%s'....\n", tok);
	    }
	}

    } else if (strncmp(cmd, "client", 3) == 0) {	/* CLIENT  */
	if (tok = mytok(cmd, 2)) {
	    if (tok[0] == 'u') {
		printf("Connecting as Client\n");
		ctid = mbusConnect(CLIENT, "client", FALSE);
		if (ctid > 0) 
		    printf("Client:   tid = %d\n", ctid);
		else {
		    ctid = -ctid;
		    printf("Client running (%d), Supervisor not found\n",ctid);
		}

	    } else if (tok[0] == 'd') {
		printf("Disonnecting as Client...");
		mbusDisconnect(ctid);
	    }
	}

    } else if (strncmp(cmd, "spawn", 3) == 0) {		/* SPAWN   */
	if (tok = mytok(cmd, 2)) {
	    int child = -1;

	    if (tok[0] == 's') {
		printf("Spawning new Supervisor...\n");
		mbusSpawn("zzsup", NULL, NULL, &child);
		printf("Super:   tid = %d\n", child);

	    } else if (tok[0] == 'c') {
		printf("Spawning new Client...\n");
		mbusSpawn("zzclient", NULL, NULL, &child);
		printf("Client:   tid = %d\n", child);
	    }
	}

    } else if (strncmp(cmd, "ping", 3) == 0) {		/* PING   */
	for (i=0; i < 3; i++) {
	    /*
	    printf ("Super Ping = %d\n", mbusPing (mbAppGet(APP_STID), 100) );
	    */
	    printf ("Super Ping = %d\n", 
		mbusSend (SUPERVISOR, ANY, MB_PING, NULL) );
	}

    } else if (strncmp(cmd, "stop", 3) == 0) {		/* STOP   */
	if (tok = mytok(cmd, 2)) {
	    if (tok[0] == 's') {
		printf("Stopping Supervisor...\n");
	    } else if (tok[0] == 'c') {
		printf("Stopping Client...\n");
	    }
	}

    } else if (strncmp(cmd, "bcast", 3) == 0) {		/* BCAST  */
	if (group = mytok(cmd, 2))
	    mbusBcast (group, (arg1=mytok(cmdin,3)), MB_STATUS);

    } else if (strncmp(cmd, "send", 3) == 0) {		/* SEND   */
	if (tok = mytok(cmd, 2)) {
	    if (tok[0] == 's') {
		mbusSend(SUPERVISOR, ANY, MB_STATUS, (arg1=mytok(cmdin,3)));
	    } else if (tok[0] == 'c') {
		mbusSend(CLIENT, ANY, MB_STATUS, (arg1=mytok(cmdin,3)));
	    } else {
		printf ("Usage: send [s|c] <msg>\n");
	    }
	}


    } else if (strncmp(cmd, "whoami", 3) == 0) {	/* WHOAMI   */
	printf ("%s\n",  mbAppGetName());

    } else if (strncmp(cmd, "halt", 3) == 0) {		/* HALT    */
	printf("Halting message bus....\n");
	if (mbusHalt() < 0)
	    fprintf(stderr, "Error halting message bus....\n");

    } else if (strncmp(cmd, "getfds", 3) == 0) {	/* GETFDS  */
	int i, nfds, *fds;

	nfds = pvm_getfds(&fds);
	printf("nfds = %d\t", nfds);
	for (i = 0; i < nfds; i++)
	    printf("fd[%d]=%d  ", i, fds[i]);
	printf("\n");

    } else {
	printf ("unrecognized command: '%s'\n", cmd);
    }

    if (tok)   free (tok);
    if (group) free (group);
    if (arg1)  free (arg1);
    if (arg2)  free (arg2);
    if (arg3)  free (arg3);

    printf("\nzzdebug (%d)>  ", getpid());
}


print_help()
{
    printf("Commands:\n");
    printf("    super [u|d]\t\tclient [u|d]\tspawn [c|s]\tstop [c|s]\n");
    printf("    init\t\thalt\t\tgetfds\tselect\n");
    printf("\n");
}


/*---------------------------------------------------------------------------
**  MBus Handlers -- These procedures are used to handle notification events
**  we get from the system.  For the moment we just print a status message,
**  a more formal method of updating the Supervisor state needs to be done.
**-------------------------------------------------------------------------*/

int myHostAdded (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf ("%s: %d new hosts just added ***\n", mbAppGetName(), n);
}

int myTaskExited (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf ("*** %s: task %d just exited ***\n", mbAppGetName(), n);
}

int myHostDeleted (int mid)
{
     int n;
     pvm_unpackf( "%d", &n );
     printf ("*** %d new hosts just deleted ***\n", mbAppGetName(), n);
}



