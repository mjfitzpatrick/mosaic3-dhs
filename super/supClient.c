#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Tcl/tcl.h>
#include <Obm.h>
#include <ObmW/Gterm.h>


#include "dhsCmds.h"
#include "super.h"


/*
** SUPCLIENT.C -- The Supervisor "client" object.  This code implements an OBM
** client and responds to messages sent to the client object by the GUI code
** executing under the object manager.
**
**	     sup_clientOpen (sup)
**	    sup_clientClose (sup)
**	  sup_clientExecute (sup, objname, key, cmd)
**
** The clientExecute callback is called by the GUI code in the object manager
** to execute client commands.
**
** Client commands:
**
**		  InitCache 
**		     Update
**
**	         procUpdate
**	          procClean
**	          smcUpdate
**	        smcProcNext
**	         smcProcAll
**
**		  setOption  option value [args]
**
**		       Help
**		      Reset
**		       Quit
*/


static int Quit(supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv);
static int Help(supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv);
static int InitCache(supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv);
static int Update(supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv);
static int setOption(supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv);
static int supUpdateProcs(supClientPtr xc,Tcl_Interp *tcl,int argc,char **argv);
static int supCleanProcs(supClientPtr xc,Tcl_Interp *tcl,int argc,char **argv);
static int supUpdateSMC(supClientPtr xc,Tcl_Interp *tcl,int argc,char **argv);
static int supProcNext(supClientPtr xc,Tcl_Interp *tcl,int argc,char **argv);
static int supProcAll(supClientPtr xc,Tcl_Interp *tcl,int argc,char **argv);
static int supUpdateRTD(supClientPtr xc,Tcl_Interp *tcl,int argc,char **argv);


char 	strbuf[SZ_LINE];

extern  int    dca_done;
extern  double atof ();



/* sup_clientOpen -- Initialize the client code.
 */
void
sup_clientOpen (supDataPtr sup)
{
    char config[SZ_LINE], host[SZ_FNAME];
    register supClientPtr xc;
    register Tcl_Interp *tcl;

    xc = (supClientPtr) XtCalloc (1, sizeof(supClient));
    sup->clientPrivate = (int *)xc;

    xc->sup = sup;
    xc->tcl = tcl = Tcl_CreateInterp();
    ObmAddCallback (sup->obm, OBMCB_clientOutput|OBMCB_preserve,
        sup_clientExecute, (XtPointer)xc);

    Tcl_CreateCommand (tcl, "Help", 
	(Tcl_CmdProc *) Help, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "Quit", 
	(Tcl_CmdProc *) Quit, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "InitCache", 
	(Tcl_CmdProc *) InitCache, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "Update", 
	(Tcl_CmdProc *) Update, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "procUpdate", 
	(Tcl_CmdProc *) supUpdateProcs, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "procClean", 
	(Tcl_CmdProc *) supCleanProcs, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "smcUpdate", 
	(Tcl_CmdProc *) supUpdateSMC, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "smcProcNext", 
	(Tcl_CmdProc *) supProcNext, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "smcProcAll", 
	(Tcl_CmdProc *) supProcAll, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "rtdUpdate", 
	(Tcl_CmdProc *) supUpdateRTD, (ClientData)xc, NULL);
    Tcl_CreateCommand (tcl, "setOption", 
	(Tcl_CmdProc *) setOption, (ClientData)xc, NULL);


    /*  Connect to the message bus as the Supervisor.
     */
    if ((sup->mytid = mbusConnect (SUPERVISOR, "super", TRUE)) < 0) {
	fprintf (stderr, "Cannot connect to message bus, restart PVM\n");
	exit (1);
    }



    /*  Open the SMC interface.  Note we currently only allow the cache
     *  file root name as an option but in later versions we can add other
     *  options to be used in the config string.
     */
    if (sup->use_smc) {
	sup_status ("Opening cache....");
	memset (config, 0, SZ_LINE);
	sprintf (config, "cache_file=%s", sup->cache_file);

	if ((sup->sup_smc._smc = smcOpen (config)) == (smCache_t *)NULL) {
	    sup_error ("Error opening cache, invalid file?");
	    fprintf (stderr, "Error opening cache, invalid file?\n");
	}
    }


    /*  Open the CDL interface.
     */
    if (sup->use_cdl) {
    	if (!(sup->sup_cdl = cdl_open ((char *)getenv("IMTDEV"))) ) {
            sup_error ("ERROR: cannot connect to display server!");
            fprintf (stderr, "ERROR: cannot open CDL\n");
    	}
    }

    /*  Update the GUI with the default state.
     */
    sup_message (sup, "disp_enable", (sup->rtd.disp_enable ? "True" : "False"));
    sup_message (sup, "stat_enable", (sup->rtd.stat_enable ? "True" : "False"));
    sup_message (sup, "rot_enable",  (sup->rtd.rot_enable  ? "True" : "False"));
    sup_message (sup, "otf_enable",  (sup->rtd.otf_enable  ? "True" : "False"));
    sup_message (sup, "par_enable",  (sup->rtd.par_enable  ? "True" : "False"));
    sup_message (sup, "lo_gain",     (sup->rtd.lo_gain     ? "True" : "False"));

    sup_message (sup, "rdirname", sup->rawDir);
    sup_message (sup, "pdirname", sup->procDir);
    sup_message (sup, "froot", sup->fileRoot);

    sup_msgi (sup, "frame",          sup->rtd.disp_frame);
    sup_msgi (sup, "stdimage",       sup->rtd.stdimage);
    sup_msgi (sup, "smc_interval",   sup->sup_smc.poll_interval);
    sup_msgi (sup, "disp_interval",  sup->rtd.poll_interval);

    sup_msgi (sup, "seqno",  	     sup->imgSeqNo);

    /* initialize the display device string (non-writable)
    */
    sup_message (sup, "imtdev", (sup->imtdev ? sup->imtdev : "inet:5187") );


    /* Initialize the processing queue.
     */
   pqInit (sup);
    
    /* Initialize the GUI with the Supervisor as the first process.
     */
    memset (host, 0, SZ_FNAME);
    if (gethostname (host, SZ_FNAME) < 0)
        strcpy (host, "localhost");

    memset (sup->triggerHost, 0, SZ_LINE);
    strncpy (sup->triggerHost, host, SZ_LINE);

    procAddClient (sup, (clientIoChanPtr)NULL, "Supervisor", host,
	sup->port, sup->mytid, (int)getpid(), "Ready ...", TRUE);
}


/* sup_clientClose -- Shutdown the client code.
 */
void
sup_clientClose (supDataPtr sup)
{
    register supClientPtr xc = (supClientPtr) sup->clientPrivate;
    Tcl_DeleteInterp (xc->tcl);
}


/* sup_clientExecute -- Called by the GUI code to send a message to the
 * "client", which from the object manager's point of view is Supervisor 
 * itself.
 */
int
sup_clientExecute (
  supClientPtr xc,
  Tcl_Interp *tcl,			/* caller's Tcl */
  char *objname,			/* object name */
  int key,				/* notused */
  char *command 
)
{
    xc->server = tcl;
    if (strcmp (objname, "client") == 0)
        Tcl_Eval (xc->tcl, command);

    return (TCL_OK);
}




/*
 * Supervisor CLIENT commands.
 * ----------------------------
 */

/* Quit -- Exit.
 *
 * Usage:	Quit
 */
static int 
Quit (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv )
{
    register supDataPtr sup = xc->sup;
    sup_shutdown (sup);
    return (TCL_OK);
}


/* Help -- Upload the online help text.
 *
 * Usage:	Help
 */

/* The builtin default help text. */
static char *help_text[] = {
    "setValue {",
#   include "super.html.h"
    "}",
    NULL
};


static int 
Help (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
        register supDataPtr sup = xc->sup;
        register char *ip, *op, *helptxt;
        register int i;

        helptxt = (char *) XtMalloc (102400);
        for (i=0,op=helptxt; (ip = help_text[i]);  i++) {
            while (*ip)
                *op++ = *ip++;
            *op++ = '\n';
        }
        *op++ = '\0';

        ObmDeliverMsg (sup->obm, "help", helptxt);
        XtFree ((char *)helptxt);

        return (TCL_OK);
}



/* InitCache -- Initialize the SMC Cache
 *
 * Usage:	InitCache
 *
 * Reset does a full power-on reset of SMC.
 */
static int 
InitCache (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv) 
{
    return (TCL_OK);
}



/* Update -- Update the SMC Cache status display.
 *
 * Usage:	Update	smc|sup
 *
 * Update the RTD or Shared Memory Cache Display
 */
static int 
Update (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    char     *option;

    if (argc < 2)
	return (TCL_ERROR);

    option = argv[1];
    if (option[0] == 'r' || option[0] == 'R')
	supUpdateRTD ((supClientPtr) xc->sup, NULL, 0, NULL);

    else if (option[0] == 's' || option[0] == 'S')
	supUpdateSMC ((supClientPtr) xc->sup, NULL, 0, NULL);

    else {
	/* Need some kind of Tcl error message ..... */
	return (TCL_ERROR);
    }

    return (TCL_OK);
}


/* setOption -- Set a client option.
**
** Usage:       setOption option value [args]
**
** Options:
**      disp_enable      true|false
**      stat_enable      true|false
**      rot_enable       true|false
**      otf_enable       true|false
**      par_enable       true|false
**      lo_gain          true|false
**      idListing        true|false
**
**      showActivity     true|false
**      verbose          true|false
**
**      seqno            <num>
**      froot            <fname_root>
**      dirname          <directory>
**      ktm          	 <ktm_file>
**      sysproc          <sysproc_file>
**      postproc         <postproc_file>
**
**      trim       	 true|false
**      frame       	 <frameno>
**      stdimg       	 <fbnum>
**
**      debug       	<flag> level
**      no-op       	
*/

static int 
setOption (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    supDataPtr sup = xc->sup;
    char      *option, *strval, buf[SZ_LINE];
    int        ch, value;


    if (argc < 3) {
        return (TCL_ERROR);

    } else {
        option = argv[1];
        strval = argv[2];

        ch = strval[0];
        if (isdigit (ch))
            value = atoi (strval);
        else if (ch == 'T' || ch == 't')
            value = 1;
        else if (ch == 'F' || ch == 'f')
            value = 0;
    }


    if (strcmp (option, "no-op") != 0)
	printf ("setOption:  %s = %s\n", option, strval);

    memset (buf, 0, SZ_LINE);
    if (strcmp (option, "disp_enable") == 0) {
	sprintf (buf, "option disp_enable %d\n", value);
	sup->rtd.disp_enable = value;
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "stat_enable") == 0) {
	sprintf (buf, "option stat_enable %d\n", value);
	sup->rtd.stat_enable = value;
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "rot_enable") == 0) {
	sprintf (buf, "option rot_enable %d\n", value);
	sup->rtd.rot_enable = value;
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "otf_enable") == 0) {
	sprintf (buf, "option otf_enable %d\n", value);
	sup->rtd.otf_enable = value;
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "par_enable") == 0) {
	sprintf (buf, "option use_threads %d\n", value);
	sup->rtd.par_enable = value;
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "lo_gain") == 0) {
	sprintf (buf, "option lo_gain %d\n", value);
	sup->rtd.lo_gain = value;
        mbusBcast ("SMCMgr", buf, MB_SET);


    } else if (strcmp (option, "idListing") == 0) {
	sprintf (buf, "option idListing %d\n", value);
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "no-op") == 0) {
        mbusBcast ("Collector", "no-op", MB_SET);
        mbusBcast ("PXF", "no-op", MB_SET);
        mbusBcast ("SMCMgr", "no-op", MB_SET);

    } else if (strcmp (option, "seqno") == 0) {
	sup->imgSeqNo = value;
	sprintf (buf, "option seqno %d\n", value);
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "trim") == 0) {
	sprintf (buf, "option trim %d\n", value);
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "froot") == 0) {
	sprintf (buf, "option froot %s\n", strval);
	strcpy (sup->fileRoot, strval);
        mbusBcast ("SMCMgr", buf, MB_SET);

    } else if (strcmp (option, "ktm") == 0) {
	dhsSetKTMFile (sup->dca_tid, sup->imgSeqNo, strval);
	sup_message (sup, "ktm_p", strval);

    } else if (strcmp (option, "sysproc") == 0) {
	sup_message (sup, "sysproc_p", strval);

    } else if (strcmp (option, "postproc") == 0) {
	sup_message (sup, "postproc_p", strval);

    } else if (strcmp (option, "rdirname") == 0) {
	fprintf (stderr, "**** RAWDIR set to '%s'\n", strval);
	strcpy (sup->rawDir, strval);
	sprintf (buf, "option dirname %s\n", strval);
        mbusBcast ("SMCMgr", buf, MB_SET);
	sup_message (sup, "rdirname", strval);

    } else if (strcmp (option, "pdirname") == 0) {
	fprintf (stderr, "**** PROCDIR set to '%s'\n", strval);
	strcpy (sup->procDir, strval);
	sup_message (sup, "pdirname", strval);

    } else if (strncmp (option, "debug", 5) == 0) {
        /* Send it to everyone, let the app decide which
        ** messages means anything to them.
        */
	char flag[32];

	memset (flag, 0, 32);
        if (argc > 3) {
            if (DEBUG) printf ("setOption:  %s %s (argv)\n", strval, argv[3]);
	    sprintf (flag, "%s %s", strval, argv[3]);
        } else {
            if (DEBUG) printf ("setOption:  %s 1 (no-op)\n", strval);
	    sprintf (flag, "%s 1", strval);
        }
	supSetDebug (flag);			/* set in Supervisor	*/
        mbusBcast ("SMCMgr",    flag, MB_SET);	/* set in SMCMgr	*/
        mbusBcast ("NF-DCA",    flag, MB_SET);	/* set in DCA		*/

    } else if (strcmp (option, "verbose") == 0) {
        if (sup->verbose != value) {
            sup->verbose = value;
            sup_msgi (sup, "verbose", value);
        }

    } else if (strcmp (option, "showActivity") == 0) {
        if (sup->showActivity != value) {
            sup->showActivity = value;
            sup_msgi (sup, "showActivity", value);
        }

    } else if (strcmp (option, "frame") == 0) {
        if (sup->rtd.disp_frame != value) {
	    char msg[SZ_LINE];

            sup->rtd.disp_frame = value;
            sup_msgi (sup, "frame", value);

	    /* Alert the RTD of the change.
	    */
	    memset (msg, 0, SZ_LINE);
	    sprintf (msg, "option frame %d", value);
            mbusBcast ("SMCMgr", msg, MB_SET);
        }

    } else if (strcmp (option, "stdimg") == 0) {
        if (sup->rtd.stdimage != value) {
	    char msg[SZ_LINE];

            sup->rtd.stdimage = value;
            sup_msgi (sup, "stdimage", value);

	    /* Alert the RTD of the change.
	    */
	    memset (msg, 0, SZ_LINE);
	    sprintf (msg, "option frame %d", value);
            mbusBcast ("SMCMgr", msg, MB_SET);
        }
    }
    return (TCL_OK);
}



/*****************************************************************************
*
*  Private Utility Procedures.
*
*****************************************************************************/


/*  supUpdateProcs -- Update the Process table.  
**  
**  FIXME -- At the moment we simply clean the process table, we want to
**  instead ping the connected clients to request a status.
*/

static int
supUpdateProcs (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    procCleanTable (xc->sup);
    return (TCL_OK);
}


/*  supCleanProcs -- Clean the Process table, i.e. clean out the
**  disconnected processes.
*/

static int
supCleanProcs (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    procCleanTable (xc->sup);
    return (TCL_OK);
}


/*  supUpdateSMC -- Update the SMC segment table.  
*/

static int
supUpdateSMC (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    mbusBcast ("SMCMgr", "segList", MB_GET|MB_STATUS);
    return (TCL_OK);
}


/*  supProcNext -- Process the next Exposure by ID.
*/

static int
supProcNext (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    register supDataPtr sup = xc->sup;
    double expID;
    extern double pqNext();
    extern int console;
    static int retry = 0, max_count = 10;


    /* Tell the SMC Managers on each node to begin processing the given
    ** exposure.  The queue will return a zero-valued expID if there are
    ** no exposures in the queue ready for processing.
    **
    ** When that is done the SMCMgr will send back a message used to trigger
    ** the PXF to begin processing.
    */
    expID = pqNext (sup);
    if (console && expID > 0.0) {
	if (retry++ < max_count) {
	    fprintf (stderr, 
		"supProcNext:  expID=%.6lf, stat=%d, count=%d, done=%d/%d\n",
	        expID, sup->qFirst->status, sup->qCount, dca_done+1,
		sup->numProcMgrs);
	} else if ((retry++ % max_count) == 1) {
	    fprintf (stderr, "supProcNext:  expID = %.6lf, retry %d times...\n",
	        expID, retry);
	    max_count = 200;
	}
    }


    /* Begin processing the image.  The (dca_done < 0) condition is met
    ** only on the very first readout, thereafter we process when we get
    ** the final signal from all the processing managers.
    */
    if (expID > 0.0 && (dca_done < 0 || dca_done >= sup->numProcMgrs)) {
        char   msg[SZ_LINE];
	extern time_t s_time;

	pqSetStat (sup, expID, PQ_ACTIVE);

	sprintf (msg, "%lf test", expID);
        sup_message (sup, "transStat", msg);

	sup->imgSeqNo++;
	s_time = time ((time_t)NULL);
        if (console)
	    fprintf (stderr, "STARTING expID = %.6lf, seqno=%d time=%d\n",
	        expID, sup->imgSeqNo, (int) s_time);

	memset (msg, 0, SZ_LINE);
        sprintf (msg, "seqno %d", sup->imgSeqNo);
        mbusBcast ("PXF", msg, MB_SET);

	memset (msg, 0, SZ_LINE);
        sprintf (msg, "option seqno %d", sup->imgSeqNo);
        mbusBcast ("SMCMgr", msg, MB_SET);

	memset (msg, 0, SZ_LINE);
	sprintf (msg, "process %.6lf", expID);
        mbusBcast ("SMCMgr", msg, MB_START);

	retry = 0;
	dca_done -= sup->numProcMgrs;
	dca_done = (dca_done < 0 ? 0 : dca_done);
    }

    return (TCL_OK);
}


/*  supProcAll -- Process all remaining exposures.
*/

static int
supProcAll (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    mbusBcast ("SMCMgr", "process all", MB_START);
    return (TCL_OK);
}


/*  supUpdateRTD -- Update the Real-Time Display.  (NO-OP)
*/

static int
supUpdateRTD (supClientPtr xc, Tcl_Interp *tcl, int argc, char **argv)
{
    return (TCL_OK);
}
