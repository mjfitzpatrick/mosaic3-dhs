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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Tcl/tcl.h>


#include "super.h"


#define CONF_DEBUG (getenv("CONF_DEBUG")!=NULL||access("/tmp/CONF_DEBUG",F_OK)==0)


#define	 SZ_CMDLINE	512

extern	int console;


static int supReadConfigFile (supDataPtr sup);




/*****************************************************************************
**  SUPCONFIGURE -- Configure the system and start any requested child
**  processed.  
*/
void
supConfigure (supDataPtr sup)
{
    register int i, delay=0;;
    confTaskPtr  cp;
    char cmd[SZ_CMDLINE], display[SZ_LINE], rootcmd[SZ_LINE], root[SZ_LINE];
    panPairPtr pp;
    int	debugLevel=0, winX=0, winY=0, winXstep=0, winYstep=0;


    /* Nothing to do without a config file.
     */
    if (strcmp (sup->config, "none") == 0)
	return;
    if (strcmp (sup->config, "default") == 0)
	sup->config = DEF_CONFIG;


    memset (display, 0, SZ_LINE);
    memset (rootcmd, 0, SZ_LINE);

    /* Read the configuration file, return on an error.
     */
    if (supReadConfigFile (sup) != 0)
	return;


    /* Now start the tasks.
    sup->nclients = 0;
     */
    for (i=0; i < sup->numConfTasks; i++) {
	cp = &sup->confTable[i];
	if (strncasecmp (cp->name, "super", 5) == 0)	/* skip supervisor */
	    continue;

	/* Check for a pairing table configuration.  Note that we overload
	** the variables in the configuration, the syntax for a pairing in 
	** the config file is:
	**
	**	pair  <pan_host>  <collector_host> [':' <port>]
	**
	** We assume a 1-to-1 mapping of machines (and don't check for a
	** duplicate).  If we get a second PAN connection from a host where
	** the collector is already assigned, we'll create a temp collector.
	*/
	if (strncasecmp (cp->name, "pair", 4) == 0) {	
	    char *ps = index (cp->command, (int)':');

	    pp = &sup->pairTable[sup->numPanPairs];
	    memset (pp, 0, sizeof (panPair));
	    strcpy (pp->phost, cp->host);
	    if (ps) {
		*ps++ = '\0';
	        strcpy (pp->chost, cp->command);
		pp->socket = atoi (ps);
	    } else {
	        strcpy (pp->chost, cp->command);
		pp->socket = 0;
	    }
	    sup->numPanPairs++;


	/* Create a pseudo host table so we can map from 'generic' machine
	** names to specific IP addresses used to make socket connections.
	** This allows us to move the machine without changing the config-
	** uration all the time.
	*/
	} else if (strncasecmp (cp->name, "host", 4) == 0) {	
	    hostTablePtr ht;
 	    char *ip;

	    ht = &sup->hosts[sup->numHostTable];
	    strcpy (ht->name, cp->host);
	    						/* strip comments */
	    for (ip=cp->command; *ip && (!isspace(*ip)); ip++)
		;
	    if (*ip) *ip = '\0';
	    strcpy (ht->ip_addr, cp->command);
	    sup->numHostTable++;


	/* Specify the keywords we wish to monitor as the system runs.  After
	** each PXF connects we'll send it the list of keywords, as each
	** image is processed the PXF will send back the values of those
	** keywords.  The third parameter is either the reserved string 'any'
	** or the name of a specific keyword database to monitor.  The
	** keyword monitoring is done by creating a list of 'db.keyw' strings
	** to be matched by the PXF as it processes the metadata pages.
	*/
	} else if (strncasecmp (cp->name, "monitor", 7) == 0) {	
	    keywMonPtr kw;
	    char *ip;

	    kw = &keywList[NKeywords++];

	    /* Truncate after first word to remove comments. */
	    for (ip=cp->command; *ip && !isspace(*ip); ip++) ;
	    *ip = '\0';

	    strcpy (kw->keyw, cp->host);
	    strcpy (kw->dbname, cp->command);
	 


	/* Specify the console parameters, i.e. window placements, statup
	** command fragments, debug levels, etc.
	*/
	} else if (strncasecmp (cp->name, "console", 7) == 0) {	
	    if (strncasecmp (cp->host, "display", 4) == 0) {
		/* Check to see whether we'll use the environment
		** DISPLAY setting or something hardwired.
		*/
	        if (strncasecmp (cp->command, "env", 3) == 0) {
		    char *env_disp;

		    if ((env_disp = getenv("DISPLAY")) == (char *)NULL) {
		        strcpy (display, cp->command);
	            } else {
		        strcpy (display, env_disp);
	            }
	        } else {
		    strcpy (display, cp->command);
	        }
	    } else if (strncasecmp (cp->host, "rootcmd", 4) == 0)
		strcpy (root, cp->command);
	    else if (strncasecmp (cp->host, "debugLevel", 4) == 0)
		debugLevel = atoi (cp->command);
	    else if (strncasecmp (cp->host, "winXinit", 7) == 0)
		winX = atoi (cp->command);
	    else if (strncasecmp (cp->host, "winYinit", 7) == 0)
		winY = atoi (cp->command);
	    else if (strncasecmp (cp->host, "winXstep", 7) == 0)
		winXstep = atoi (cp->command);
	    else if (strncasecmp (cp->host, "winYstep", 7) == 0)
		winYstep = atoi (cp->command);
	    else if (strncasecmp (cp->host, "client_delay", 12) == 0) {
		if (console)
		    printf ("Client delay: %d seconds...\n", atoi(cp->command));
		delay =  atoi (cp->command);
	    }


	/* Specify the "trigger host", i.e. a CloseExposure message from 
	** this machine means the data transfer is complete and we can begin
	** processing.
	*/
	} else if (strncasecmp (cp->name, "trigger", 7) == 0) {	
	    strcpy (sup->triggerHost, cp->host);

	} else {

	    /* FIXME:  Needs to be replaced with an exec/fork.....assuming
	    **         we ever decide to use the config file to start the
	    **         processes.
	    */
    	    memset (cmd, 0, SZ_CMDLINE);
	    if (console) {
		sprintf (rootcmd, root, winX, winY);
	        sprintf (cmd, "ssh %s %s -display %s -e %s -debug %d &",
		    cp->host, rootcmd, display, cp->command, debugLevel);
		printf ("Spawning: %s\n", cmd);

		winX += winXstep;
		winY += winYstep;
	    } else
	        sprintf (cmd, "ssh %s %s &", cp->host, cp->command);

	    system (cmd);
	    if (delay > 0)
		sleep (delay);
	    sup->nclients++;
	}
    }

    if (CONF_DEBUG) {
	fprintf (stderr, "PAIRING TABLE:\n");
	for (i=0; i < sup->numPanPairs; i++) {
	   fprintf (stderr, "%d:  pan='%s'  coll='%s'\n", i, 
		sup->pairTable[i].phost,
		sup->pairTable[i].chost);
	}

	fprintf (stderr, "KEYWORD MONITORING LIST:\n");
	for (i=0; i < NKeywords; i++) {
	   fprintf (stderr, "%d:  keyw='%s'  db='%s'\n", i, 
		keywList[i].keyw, keywList[i].dbname);
  	}
    }
}




/*****************************************************************************
**  SUPREADCONFIGFILE -- Read the configuration file.
*/

#define SZ_CONFIG_LINE		512

static int
supReadConfigFile (sup)
supDataPtr sup;
{
    register int i, j, cnum=0, done = 0;
    char  line[SZ_CONFIG_LINE];
    confTaskPtr  cp;
    FILE  *fd;


    if (CONF_DEBUG) 
	fprintf (stderr, "Opening config file '%s'\n", sup->config);

    if ((fd = fopen (sup->config, "r")) == (FILE *)NULL) {
	fprintf (stderr, "Error opening config file '%s'\n", sup->config);
	return (ERR);
    }


    /* Loop over the lines in the file, populating the config table.
     */
    memset (line, 0, SZ_CONFIG_LINE);
    while (fgets (line, SZ_CONFIG_LINE, fd) != NULL) {

	/*  skip comments/blanks	
	 */
	if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')		
	    continue;

	cp = &sup->confTable[cnum];
        memset (cp, 0, sizeof (confTask));	

	for (i=0, j=0; !isspace(line[i]); i++)	/* extract task name	*/
	    cp->name[j++] = line[i];
	while (isspace(line[i])) i++;		/* skip spaces		*/

	for (j=0; !isspace(line[i]); i++)	/* extract host name	*/
	    cp->host[j++] = line[i];
	while (isspace(line[i])) i++;		/* skip spaces		*/

	done = 0;
	j = 0;
	while (!done) {				/* extract cmd string	*/
	    for (; line[i] != '\n' && line[i] != '\\'; i++)
	        cp->command[j++] = line[i];

	    if (line[i] == '\\') {
	        done = (fgets (line, SZ_CONFIG_LINE, fd) == NULL);
		for (i=0; isspace(line[i]); i++) ;/* skip spaces	*/
	    } else
	        done = 1;
	}

    
	if (CONF_DEBUG) 
	    fprintf (stderr, "%d:  name='%s'  host='%s'  cmd='%s'\n", 
	        cnum, cp->name, cp->host, cp->command);

	cnum++;
        memset (line, 0, SZ_CONFIG_LINE);
    }
    sup->numConfTasks = cnum;

    fclose (fd);

    return (OK);
}
