#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
/* Function prototypes */
#ifdef __STDC__
#include <stddef.h>
#include <stdlib.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Tcl/tcl.h>
#include <Obm.h>

#include "super.h"


/*  Process Table Interface --
**
**
**    stat = procAddClient (sup, chan, name, host, port, tid, pid, update)
**    stat = procDelClient (sup, clid, update)
**   clid = procFindClient (sup, port, tid)
**          procDisconnect (sup, clid)
**          procCleanTable (sup)
**        procUpdateStatus (sup, clid, status, update)
**
**            procSetValue (sup, clid, msg, update)
**            procGetValue (sup, clid, msg, update)
**
**  procUpdateProcessTable (sup)
** procUpdateTransferTable (sup)
**
*/


#define PROC_DEBUG \
	    (getenv("PROC_DEBUG")!=NULL||access("/tmp/PROC_DEBUG",F_OK)==0)



/*  PROCADDCLIENT -- Add a client connection to the process table.  We simply
**  add the new process to the end of the table, somebody else will clean up
**  the dead processes.
*/
int
procAddClient (supDataPtr sup, clientIoChanPtr chan, char *name, 
    char *host, int port, int tid, pid_t pid, char *status, int update)
{
    clientProcPtr pcp, cp = &sup->procTable[sup->numClients];
    panConnPtr pc = &sup->connTable[sup->numPanConns];


    extern char *supNeuter();


    if (PROC_DEBUG) {
	fprintf (stderr,
	    "proc: adding name=%s host=%s:%d port=%d tid=%d nclients=%d\n",
	    name, host, pid, port, tid, sup->numClients);
    }

    if (strcmp (name, "CMD_Test") == 0) {
        if (PROC_DEBUG)
	    fprintf (stderr,
	        "proc: skipping test prog: name=%s host=%s:%d port=%d tid=%d\n",
	        name, host, pid, port, tid);
	return (OK);
    }

    if (sup->numClients == SZ_PROCTABLE) {
	fprintf (stderr, "procAddClient() Fails: ");
	fprintf (stderr, "Exceeded process table adding '%s'.\n", name);
	return (ERR);
    }

    memset (cp, 0, sizeof (clientProc));
    memset (cp->name, 0, SZ_PNAME);
    memset (cp->host, 0, SZ_PHOST);
    memset (cp->date, 0, SZ_PDATE);
    memset (cp->status, 0, SZ_PSTAT);

    strcpy (cp->name, name);

/* Strip domain and extension
    for (i=0; i < strlen(host) && host[i] != '.'; i++)  
        cp->host[i] = host[i];
*/
    strcpy (cp->host, supNeuter(host));
    cp->port = port;
    cp->tid  = tid;
    cp->pid  = pid;
    if (chan)
        bcopy (chan, &cp->chan, sizeof (clientIoChan));
    else
        memset (&cp->chan, 0, sizeof (clientIoChan));
    strcpy (cp->date, procTimestamp());
    strcpy (cp->status, status);

    cp->connected  = 0;

    sup->numClients++;

    /*  Now see if this is a Collector, if so add it to the pairing table
    **  so we can match it with a PAN when connected.  The PAN connects on
    **  on the socket interface (hopefully) after the Collector, so if we're
    **  a PAN, look for a Collector running on the preferred pair host.
    */
    if (strncasecmp (name, "collector", 9) == 0) {

	pc->collector = cp;
	pc->inUse = 0;
        sup->numPanConns++;

    } else if (strncasecmp (name, "pan", 3) == 0) {
	panPairPtr pp;
	register int i;

	/*  Find the this PAN client host in the pairing table.
	*/
	for (i=0; i < sup->numPanPairs; i++) {
	    pp = &sup->pairTable[i];	/* get machine pairing		*/
	    if (strcasecmp (supNeuter(host), pp->phost) == 0)
		break;
	}

	/*  Find the Collector running on the paired host.
	*/
	for (i=0; i < sup->numPanConns; i++) {
	    pc = &sup->connTable[i];	/* get the pan connection 	*/
	    pcp = pc->collector;	/* get collector		*/

	    if (pc->inUse > 0)
	        continue;

	    if (strcasecmp (pcp->host, pp->chost) == 0 &&
		pp->socket == pcp->port) {
		    /* Matched the Collector host with the entry for the PAN
		    ** in the pairing table.  Note we assume the pairings are
		    ** unique in the table.
		    */
		    pc->pan = cp;
		    break;
	    }
	}
    }

    if (update)
        procUpdateProcessTable (sup); 		/* update the GUI display */

    return (OK);
}


/*  PROCDELCLIENT -- Delete a client from the process table.  We're given
**  the client ID so delete the entry and shift the rest of the table,
**  update the display if needed.
*/
int
procDelClient (supDataPtr sup, int clid, int update)
{
    int i = clid;
    clientProcPtr cp = &sup->procTable[clid], np;


    if (PROC_DEBUG)
	fprintf (stderr, "Deleting Client '%s' ncl=%d\n", 
	    cp->name, sup->numClients);

    if (cp->connected) {
	fprintf (stderr,
	    "Warning: Client %d: name='%s' on '%s' still connected.\n",
	    clid, cp->name, cp->host);
	return (ERR);
    }

    for (i=clid; i < (sup->numClients-1); i++) {
	np = &sup->procTable[i+1];		/* next entry 		  */
	bcopy (np, cp, sizeof(clientProc));	/* shift      		  */
	cp = np;
    }
    np = &sup->procTable[sup->numClients];	/* next entry 		  */
    memset (np, 0, sizeof(clientProc));		/* clear last element 	  */
    sup->numClients--;				/* decrement count	  */

    if (update)
        procUpdateProcessTable (sup); 		/* update the GUI display */

    return (OK);
}


/*  PROCFINDCLIENT -- Find a client in the process table given either the
**  inet port on which it is connected, or the msgbus tid.  Return -1 if
**  not found, otherwise return the slot in the process table.
*/
int
procFindClient (supDataPtr sup, int port, int tid)
{
    register  int i;
    clientProcPtr cp;


    for (i=0; i < sup->numClients; i++) {
	cp = &sup->procTable[i];
	if ((cp->port == port && cp->tid == tid) ||	/* exact match	*/
	    (cp->port == port && tid <= 0) ||		/* port match	*/
	    (port <= 0 && cp->tid == tid))		/* tid match	*/
	 	return (i);
    }

    return (-1);
}


/*  PROCFINDBYSOCKET -- Find a client in the process table given either the
**  inet port on which it is connected, or the msgbus tid.  Return -1 if
**  not found, otherwise return the slot in the process table.
*/
int
procFindBySocket (supDataPtr sup, int socket_fd)
{
    register  int i;
    clientProcPtr cp;


    for (i=0; i < sup->numClients; i++) {
	cp = &sup->procTable[i];
	if (cp->chan.datain == socket_fd)
	    return (i);
    }

    return (-1);
}


/*  PROCDISCONNECT -- "Disconnect" a client, i.e. mark it as unused.
*/
void
procDisconnect (supDataPtr sup, int clid)
{
    clientProcPtr cp;
    

    if (PROC_DEBUG)
	fprintf (stderr, "Disconnecting proc %d\n", clid);

    if (clid < 0 || sup == (supDataPtr) NULL)
	return;

    cp = &sup->procTable[clid];
    cp->port = -1;
    cp->tid = -1;
    cp->connected = 0;
    strcpy (cp->date, procTimestamp());
    strcpy (cp->status, "Disconnected");

    procUpdateProcessTable (sup); 		/* Update the GUI display. */
}


/*  PROCCLEANTABLE -- Clean the process table, i.e. remove all the entries
**  that aren't active and update the GUI.  Because the table is small we'll
**  simply shift the table contents as needed rather than deal with the 
**  complexities of a fully dynamic table.
*/
void
procCleanTable (supDataPtr sup)
{
    clientProcPtr cp;
    int i;

    for (i=0; i < sup->numClients; ) {
	cp = &sup->procTable[i];
	if (strcasecmp (cp->name, "Supervisor") != 0) {
	    if (!cp->connected && sup->numClients > 1) {
	        procDelClient (sup, i, 0);
		continue;
	    }
	}
	i++;
    }
    procUpdateProcessTable (sup); 		/* Update the GUI display. */
}


/*  PROCUPDATESTATUS -- Update the status message of a given client id.
*/
int
procUpdateStatus (supDataPtr sup, int clid, char *status, int update)
{
    clientProcPtr cp;
    

    if (PROC_DEBUG > 100) {
	fprintf (stderr, 
	    "statusUpdate: numClients=%d clid=%d sup=0x%x stat='%s'\n", 
	    sup->numClients, clid, (int) sup, status);
    }

    if (clid < 0 || sup == (supDataPtr) NULL)
        return (ERR);
    
    if (strncmp (status, "trans", 5) == 0) {
        sup_message (sup, "transTab", &status[6]);

    } else if (strncmp (status, "file", 4) == 0) {
        sup_message (sup, "filesTab", &status[5]);

    } else if (strncmp (status, "dca", 3) == 0) {
        sup_message (sup, "dcaTransStat", &status[3]);

    } else if (strncmp (status, "active", 6) == 0) {
	sup->procTable[clid].active = 1;
        cp = &sup->procTable[clid];
        cp->active = 1;

    } else if (strncmp (status, "inactive", 8) == 0) {
	sup->procTable[clid].active = 0;
        cp = &sup->procTable[clid];
        cp->active = 0;

    } else { 			/* Default to a process status message.	*/
        cp = &sup->procTable[clid];
        strcpy (cp->date, procTimestamp());
	if (strncmp (status, "proc", 4) == 0)
            strcpy (cp->status, &status[5]);
	else
            strcpy (cp->status, status);
    }

    if (update)
        procUpdateProcessTable (sup); 		/* Update the GUI display. */

    return (OK);
}


/*  PROCSETVALUE -- Set a value associated with a client process.
*/
int
procSetValue (supDataPtr sup, int clid, char *msg, int update)
{
    clientProcPtr cp;
    panConnPtr    pc;
    int  i, ival;


    if (PROC_DEBUG) fprintf (stderr, "procSetValue: %d:  %s\n", clid, msg);

    if (clid < 0 || sup == (supDataPtr) NULL)
        return (ERR);
    

    if (strncmp (msg, "port", 4) == 0) {
        cp = &sup->procTable[clid];
        /* cp->port = atoi (&msg[5]); */
	sscanf (msg, "port %d %s", &cp->port, cp->colID);

    } else if (strncmp (msg, "transfer", 8) == 0) {
	ival = atoi (&msg[9]);

	/* Update the PAN connection table.
	*/
        cp = &sup->procTable[clid];
	for (i=0; i < sup->numPanConns; i++) {
	    pc = &sup->connTable[i];
	    if (pc->collector == cp) {
		pc->bytesTransferred += ival;
		pc->bytesTotal += ((double)ival / (1024.*1024*1024));
		strcpy (pc->timestamp, procTimestamp());
	    }
	}
        if (update)
            procUpdateTransferTable (sup); 	/* Update the GUI display. */

    } else if (strncmp (msg, "connected", 9) == 0) {
	char  smsg[128];


	ival = atoi (&msg[10]);

        cp = &sup->procTable[clid];
        cp->connected = ival;


        for (i=0; i < sup->numPanConns; i++) {
            pc = &sup->connTable[i];
            if (pc->collector == cp) {
		memset (smsg, 0, 128);
        	sprintf (smsg, "%s %d", pc->pan->host, ival);
        	sup_message (sup, "conStat", smsg);
	    }
	}

	/* Update the PAN connection table.
	*/
	for (i=0; i < sup->numPanConns; i++) {
	    pc = &sup->connTable[i];
/*
	    if (pc->collector == cp)
		pc->inUse = ival;
*/
	}
        if (update)
            procUpdateProcessTable (sup);	/* Update the GUI display. */
    } 

    return (OK);
}
    


/*  PROCGETVALUE -- Get a value associated with a client process.
*/
int
procGetValue (supDataPtr sup, int clid, char *msg, int update)
{
    /* Not yet implemented */
    return (0);
}


/*  PROCUPDATEPROCESTABLE -- Update the GUI with the new process table.
*/
void
procUpdateProcessTable (supDataPtr sup)
{
    char line[160], buf[(SZ_PROCTABLE * 80)], tid[10], port[10];
    char *fmt = " {  %-10.10s %-11.11s %-6d %-6s %-4s %8s %s } ";
    int i, nactive = 0, active[SZ_PROCTABLE];
    clientProcPtr cp;


    memset (buf, 0, (SZ_PROCTABLE*80));

    if (PROC_DEBUG) 
	fprintf (stderr, "Update Table  nclient=%d:\n", sup->numClients);

    for (i=0; i < sup->numClients; i++) {
	cp = &sup->procTable[i];

	if (cp->active) 
	    active[nactive++] = i;

        memset (tid, 0, 10);  			/* fmt the msgbus tid 	*/
	strcpy (tid, "N/A");
	if (cp->tid > 0 && cp->tid != 999)
	    sprintf (tid, "%-6d",  cp->tid);

        memset (port, 0, 10);  			/* fmt the inet port	*/
	strcpy (port, "N/A");
	if (cp->port > 0 && cp->port != 999)
	    sprintf (port, "%-4d",  cp->port);

        memset (line, 0, 160);			/* fmt the whole line	*/
	sprintf (line, fmt, cp->name, cp->host, cp->pid, tid, port,
	    cp->date, cp->status);

        if (PROC_DEBUG) 
	    fprintf (stderr, "Update %d: '%s'\n", i, line);

	strcat (buf, line);
    }

    if (PROC_DEBUG) 
	fprintf (stderr, "\nUpdate Msg: '%s'\n\n", buf);
	

    /* Send it to the GUI.
     */
    sup_message (sup, "procTab", buf);

    /* Update the activity display.
     */
    if (sup->showActivity) {
        for (i=0; i < nactive; i++) {
	    memset (line, 0, 160);
	    sprintf (line, "%d", active[i]);
            sup_message (sup, "procActive", line);
        }
    }
}


/*  PROCUPDATETRANSFERTABLE -- Update the GUI with the new transfer table.
*/
void
procUpdateTransferTable (supDataPtr sup)
{
    char line[160], buf[(SZ_PROCTABLE * 80)], *units;
    char *fmt  = " {%10s:%-6d->%10s:%-6d %.2f %s %.2f} ";
    char *vfmt = " {%10s:%-6d->%10s:%-6d %.2f %s %.2f %s %s} ";

    register int i;
    double   size, total;
    panConnPtr    pc;


    memset (buf, 0, (SZ_PROCTABLE*80));
    for (i=0; i < sup->numPanConns; i++) {
	pc = &sup->connTable[i];

        memset (line, 0, 160);			/* fmt the whole line	*/

	size = pc->bytesTransferred / (1024.*1024.);
	total = pc->bytesTotal;
	units = "/";
/*
	if (size <= 1024) {
	    units = "Bytes";
	} else if (size <= (1024*1024)) {
	    size /= 1024.;
	    units = "KB";
	} else {
	    size /= (1024.*1024.);
	    units = "MB";
	}

	if (pc->inUse && pc->pan && pc->collector) {
*/
	if (pc->pan && pc->collector) {
	    if (sup->verbose) {
	        sprintf (line, vfmt, 
	            pc->pan->host, pc->pan->pid, 
	            pc->collector->host, pc->collector->pid, 
	            size, units, total,
	            pc->timestamp, pc->obsSetID);
	    } else {
	        sprintf (line, fmt, 
	            pc->pan->host, pc->pan->pid, 
	            pc->collector->host, pc->collector->pid, 
	            size, units, total);
	    }
	    strcat (buf, line);
	}
    }

    /* Send it to the GUI.
     */
    sup_message (sup, "transTab", buf);
}


/*  PROCBREAKOUTCLIENTID -- The "client id" sent in a message is of the
**  form:
**		<name>'@'<host>':'<pid>
** 
**  Our job here is to break this out into the constituent components.  We'll
**  remove the domain information from the host and supply defaults if
**  is found.
*/
void
procBreakoutClientID (char *id, char *name, char *host, int *pid)
{
    char *ip = id, *op;


    if (id == NULL) {				/* ultimate default	*/
	strcpy (name, "unknown");
	strcpy (host, "unknown");
	*pid = 0;
	return;
    }

    /* Get the client name.
     */
    for (op=name; *ip && *ip != '@'; ip++, op++) {
	*op = *ip;
    }
    if (op == name)				/* no value, set default */
	strcpy (name, "unknown");

    /* Get the host name.
     */
    ip++;
    for (op=host; *ip && *ip != ':'; ip++, op++) {
	if (*ip != '.') {
	    *op = *ip;
	} else {
	    *op = '\0';				/* skip the domain info	*/
	    while (*ip && *ip != ':')
		ip++;
	    break;
	}
    }
    if (op == host)				/* no value, set default */
	strcpy (name, "unknown");

    /* Get the process id.
     */
    *pid = atoi (++ip); 
}


/*  PROCTIMESTAMP -- Return the time in the form 12:34:56
*/
char *
procTimestamp ()
{
    time_t clock = time (0);
    char   *tstr = ctime (&clock);

    tstr = &tstr[11];
    tstr[8] = '\0';

    return (tstr);
}
