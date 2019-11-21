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
#include <ctype.h>

#include "mbus.h"


void    inputEventHandler (char *cmd);
int	pvm_handle   (int fd, void *client_data);
int	stdin_handle (int fd, void *client_data);
char   *consTok ();
void    consHelp ();



/**************************************************************************
**  CONSMSGHANDLER -- Handle messages from the user console (i.e. stdin)
*/
int
consMsgHandler (int fd, void *data)
{
    int n;
    char buf[2048];
    int  mytid = mbAppGet(APP_TID);


    n = read(fd, buf, 2048);		/* already saved fileno(stdin)	*/
    buf[strlen(buf) - 1] = '\0';	/* kill newline  		*/

    /*  If we see an EOF or 'quit' command, disconnect from the mbus
     *  and shutdown, otherwise process the command.
     */
    if (n <= 0 || strncmp (buf, "quit", 4) == 0) {
	mbusDisconnect (mytid);
        exit (0);

    } else
        inputEventHandler (buf); 	/* main message handler  	*/

    return (OK);
}


/* Handle a command given to us from the commandline.
 */
void
inputEventHandler (char *cmd)
{
    char   cmdin[128], *tok = NULL, *group = NULL;
    char  *arg1=NULL, *arg2=NULL, *arg3=NULL;
    static int init = 0;


    /* Command loop */
    if (init++ == 0)  printf("command (%d)>  ", getpid());


    bzero (cmdin, 128);				/* Save input command 	*/
    strcpy(cmdin, cmd);

    if (cmd[0] == '?') {				/* HELP    	*/
	consHelp ();

    } else if (strncmp(cmd, "init", 3) == 0) {		/* INIT    	*/
	printf("Initializing...");
	mbusInitialize(SUPERVISOR, NULL);
	printf("done.");

    } else if (strncmp(cmd, "ping", 3) == 0) {		/* PING   */
	if ((tok = consTok(cmd, 2))) {
	    if (tok[0] == 's') {
	        printf ("Supervisor Ping = %d\n", 
		    mbusSend (SUPERVISOR, ANY, MB_PING, NULL) );
	    } else if (tok[0] == 'c') {
	        printf ("Client Ping = %d\n", 
		    mbusSend (CLIENT, ANY, MB_PING, NULL) );
	    }
	}

    } else if (strncmp(cmd, "stop", 3) == 0) {		/* STOP   */
	if ((tok = consTok(cmd, 2))) {
	    if (tok[0] == 's') {
		printf("Stopping Supervisor...NYI\n");
	    } else if (tok[0] == 'c') {
		printf("Stopping Client...NYI\n");
	    }
	}

    } else if (strncmp(cmd, "bcast", 3) == 0) {		/* BCAST  */
	if ((group = consTok(cmd, 2)))
	    mbusBcast (group, (arg1=consTok(cmdin,3)), MB_STATUS);

    } else if (strncmp(cmd, "send", 3) == 0) {		/* SEND   */
	if ((tok = consTok(cmd, 2))) {
	    if (tok[0] == 's') {
		mbusSend(SUPERVISOR, ANY, MB_STATUS, (arg1=consTok(cmdin,3)));
	    } else if (tok[0] == 'c') {
		mbusSend(CLIENT, ANY, MB_STATUS, (arg1=consTok(cmdin,3)));
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
	extern int pvm_getfds ();

	nfds = pvm_getfds (&fds);
	printf("nfds = %d\t", nfds);
	for (i = 0; i < nfds; i++)
	    printf("fd[%d]=%d  ", i, fds[i]);
	printf("\n");

    } else
	printf ("unrecognized command: '%s'\n", cmd);


    if (tok)   free (tok); 			/* Clean up		*/
    if (group) free (group);
    if (arg1)  free (arg1);
    if (arg2)  free (arg2);
    if (arg3)  free (arg3);

    printf("\ncommand (%d)>  ", getpid());
}


/***************************************************************************
**  CONSTOK -- Return the requested token from a string.
*/
char *
consTok (char *str, int N)
{
    register int i;
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

    if (ip) free (ip);
    return ( strdup (buf) );
}


/***************************************************************************
**  CONSHELP -- Print the accepted console help commands.
*/
void
consHelp ()
{
    printf("Commands:\n");
    printf("    super [u|d]\t\tclient [u|d]\tspawn [c|s]\tstop [c|s]\n");
    printf("    init\t\thalt\t\tgetfds\tselect\n");
    printf("\n");
}
