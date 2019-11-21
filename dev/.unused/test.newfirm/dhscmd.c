#include "mbus.h"


/* Client socket program to send a large buffer to a listening server */


int loop = 1;
int delay = 0;
int interactive = 0;



int main(int argc, char *argv[])
{
    int i, mytid, from_tid, to_tid;
    char *group = "test";
    char buf[SZ_FNAME];


    /* Initialize connections to the message bus  */
    if ((mytid = mbusConnect("CMD_Test", "test", FALSE)) <= 0) {
	fprintf(stderr, "ERROR: Can't connect to message bus.\n");
	exit(1);
    }

    /* Process the command-line arguments.
     */
    for (i = 1; i < argc; i++) {
	if (strncmp(argv[i], "-help", 2) == 0) {
	    /*clientUsage (); */
	    exit(0);;

	} else if (strncmp(argv[i], "-group", 5) == 0) {
	    group = argv[++i];

	} else if (strncmp(argv[i], "-quit", 5) == 0) {
	    fprintf (stderr, "Sending QUIT to Group: %s ...\n", group);
	    mbusBcast(group, "quit", MB_DONE);

	} else if (strncmp(argv[i], "-expid", 5) == 0) {
	    sprintf (buf, "process %s", argv[++i]);
	    mbusBcast(group, buf, MB_START);

	} else if (strncmp(argv[i], "-next", 5) == 0) {
	    mbusBcast(group, "process next", MB_START);

	} else if (strncmp(argv[i], "-start", 5) == 0) {
	    mbusBcast(group, "start", MB_START);

	} else if (strncmp(argv[i], "-stop", 5) == 0) {
	    mbusBcast(group, "stop", MB_FINISH);

	} else if (strncmp(argv[i], "-delay", 5) == 0) {
	    delay = atoi(argv[++i]);
	    sleep(delay);

	} else if (strncmp(argv[i], "-interactive", 5) == 0) {
	    interactive++;
	    loop += 999;

	} else if (strncmp(argv[i], "-loop", 5) == 0) {
	    loop = atoi(argv[++i]);

	} else {
	    /*clientUsage (); */
	    exit(0);;
	}
    }





    mbusDisconnect(mytid);

    exit(0);
}
