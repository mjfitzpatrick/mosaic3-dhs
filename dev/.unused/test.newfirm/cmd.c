#include "mbus.h"


/* Client socket program to send a large buffer to a listening server */


int loop = 1;
int delay = 0;
int interactive = 0;



int main(int argc, char *argv[])
{
    int i, mytid, from_tid, to_tid;


    /* Initialize connections to the message bus  */
    if ((mytid = mbusConnect("CMD_Test", "test", FALSE)) <= 0) {
	fprintf(stderr, "ERROR: Can't connect to message bus.\n");
	exit(1);
    }

    /* Process the command-line arguments.
     */
    for (i = 1; i < argc; i++) {
	if (strncmp(argv[i], "-help", 5) == 0) {
	    /*clientUsage (); */
	    exit(0);;

	} else if (strncmp(argv[i], "-start", 5) == 0) {
	    mbusBcast("test", "start", MB_START);

	} else if (strncmp(argv[i], "-stop", 5) == 0) {
	    mbusBcast("test", "stop", MB_FINISH);

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
