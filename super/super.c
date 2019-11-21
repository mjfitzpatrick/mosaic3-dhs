#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Tcl/tcl.h>
#include <Obm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define  SUPER_MAIN
#include "../build.h"
#include "super.h"

static void supUsage (void);
static void sup_printOption (char *st);




/*  SUPER -- The Monsoon DHS Supervisor with integrated GUI status
 *  monitor.
 */


/* The builtin default GUI. */
char *defgui_text[] = {
#   include "super.gui.h"
    NULL
};


/* Data. */
XtAppContext app_context;
static char  server[] = "server";

int 	sv_argc;			/* Saved argv context	*/
char 	**sv_argv;

int	console = 0;

/*
static void output();
extern void exit();
*/


/* MAIN -- The main program.
 */
int
main (int argc, char *argv[])
{
	supDataPtr sup = &super_data;
	Widget 	  toplevel;
	XtPointer obm;
	register  int  i;


        /* Initialize the application defaults.  We do this now so the 
	 * command-line and resource flags can override the defaults.
	 */
        sup_initialize (sup);

        /* Loop over the command line options and preprocess the ones that
         * are widget/GUI resources we want to make available more easily.
         * To do this we'll tweak the argument list so it appears to be a
         * "-xrm" resource setting, this means the X initialization code
         * below will do all the real work.
         */
        for (i=1; i < argc; i++) {
            if (strcmp (argv[1], "-help") == 0) {
                supUsage ();
                exit (1);

            } else if (strcmp (argv[1], "-defgui") == 0) {
                for (i=0;  defgui_text[i];  i++)
                    printf ("%s\n", defgui_text[i]);
                exit (0);
            }
	}


        /* Get local copy of argc and argv. 
	 */
        if ((sv_argc = argc) > 0) {
            sv_argv = (char **) XtMalloc (argc * sizeof(char *));
            memmove (sv_argv, argv, argc * sizeof(char *));
        } else
            sv_argv = argv;

	/* Initialize applications context. 
	 */
	sup->toplevel = toplevel = XtAppInitialize (&app_context, "supConsole",
	    (XrmOptionDescList) NULL, 0, &sv_argc, sv_argv,
	    (String *) NULL, (ArgList) NULL, 0);

	/* Free saved arglist.
	 */
	free ((char *)sv_argv);

        /* Get application resources. */
	if (!toplevel)
	    exit (1);
	else {
            XtVaGetApplicationResources (sup->toplevel, sup,
        	resources, XtNumber(resources),
        	/* Add any resource overrides here */
        	NULL);
	}


	/* Initialize the GUI object manager. 
	 */
	if (!(obm = sup->obm = (XtPointer) ObmOpen (app_context, argc, argv)))
	    exit (2);
	/*
	ObmAddCallback (obm, OBMCB_clientOutput|OBMCB_preserve, output, obm);
	*/


        /* Loop over the command line options. The default sup structure
         * should be defined at this point so the command options can be
         * used to override them.
         */
        for (i=1; i < argc; i++) {

            if (strcmp (argv[i], "-cache") == 0) {
                strcpy (sup->cache_file, argv[++i]); /* cache file	   */

            } else if (strcmp (argv[i], "-gui") == 0) {
                sup->gui = argv[++i]; 		     /* alternate GUI file */

            } else if (strcmp (argv[i], "-host") == 0) {
                sup->host = argv[++i];		     /* host name 	   */

            } else if (strcmp (argv[i], "-rawDir") == 0) {
		memset (sup->rawDir, 0, SZ_FNAME);
                strcpy (sup->rawDir, argv[++i]);     /* user directory str */

            } else if (strcmp (argv[i], "-procDir") == 0) {
		memset (sup->procDir, 0, SZ_FNAME);
                strcpy (sup->procDir, argv[++i]);     /* user directory str */

            } else if (strcmp (argv[i], "-fileRoot") == 0) {
		memset (sup->fileRoot, 0, SZ_FNAME);
                strcpy (sup->fileRoot, argv[++i]);   /* user directory str */

            } else if (strcmp (argv[i], "-userStr") == 0) {
		memset (sup->userStr, 0, SZ_FNAME);
                strcpy (sup->userStr, argv[++i]);    /* user directory str */

            } else if (strcmp (argv[i], "-console") == 0) {
                sup->use_console++;		     /* use the console	   */
		console++;

            } else if (strcmp (argv[i], "-cdl") == 0) {
                sup->use_cdl++;			     /* use the CDL	   */

            } else if (strcmp (argv[i], "-config") == 0) {
                sup->config = argv[++i];     	     /* set config file	   */

            } else if (strcmp (argv[i], "-smc") == 0) {
                sup->use_smc++;			     /* use the SMC	   */

            } else if (strcmp (argv[i], "-port") == 0) {
                sup->port = atoi (argv[++i]);	     /* public connection  */

            } else if (strcmp (argv[i], "-nclients") == 0) {
                sup->nclients = atoi (argv[++i]);     /* public connection  */

            } else if (strcmp (argv[i], "-debug") == 0) {
                sup->debug++;	     		     /* debug level	   */

            /* Skip any standard X toolkit flags, they're handled above. 
             */
            } else if (strcmp (argv[i], "-bg") == 0) { 		i++;
            } else if (strcmp (argv[i], "-fg") == 0) { 		i++;
            } else if (strcmp (argv[i], "-iconic") == 0) { 	   ;
            } else if (strcmp (argv[i], "-display") == 0) { 	i++;
            } else if (strcmp (argv[i], "-geometry") == 0) { 	i++;
            } else if (strcmp (argv[i], "-title") == 0) { 	i++;
            } else if (strcmp (argv[i], "-xrm") == 0) { 	i++;

            } else {
                fprintf (stderr, "Unrecognized flag '%s'\n", argv[i]);
                supUsage();
                exit (1);
            }

	}


        /* Load the Supervisor GUI.  If the GUI name is "default" the builtin
         * default GUI is used.  This is stored as an array of text lines,
         * which we must append newlines to and concatenate together to
         * form the GUI message.
         */
        if (strcmp (sup->gui, "default") == 0 ||
            (ObmDeliverMsgFromFile (obm, server, sup->gui) != 0)) {

            register char *ip, *op;
            char *message;
            int i;

            message = (char *) malloc (200000);
            for (i=0, op=message; (ip = defgui_text[i]);  i++) {
                while (*ip)
                    *op++ = *ip++;
                *op++ = '\n';
            }
            *op++ = '\0';

            ObmDeliverMsg (obm, server, message);
            free ((char *)message);
        }


        /*  Initialize the (GUI) client code. 
	 */
        sup_clientOpen (sup);

	/*  Open the public connection port and setup the message bus
	 *  communications.  We'll use the XtAppMainLoop to process the
	 *  input events on the various i/o handlers.
	 */
	supInitIO (sup);

	/*  Configure the system, i.e. start the child processes named in
	 *  the config file.
	 */
	supConfigure (sup);

	/*  Initialize the various input handlers.
	 */

	/*  Activate the GUI.  
	 */
	ObmActivate (obm);


        /*  Initialize the (GUI) state.  All this does at the moment
	 *  is update the SMC segment listing to show any data still in
	 *  the SMC.
	 */
	mbusBcast ("SMCMgr", "segList", MB_GET|MB_STATUS);


	/*  EXECUTE (never returns).
	 */
	XtAppMainLoop (app_context);

	return (0);
}


/*  sup_initialize -- Initialize the SUPER application.
 */
void
sup_initialize (supDataPtr sup)
{
    /*  Set the application defaults.
     */
    sup->rtd.disp_enable	= TRUE;		/* initial display options */
    sup->rtd.stat_enable	= TRUE;
    sup->rtd.rot_enable		= TRUE;
    sup->rtd.otf_enable		= TRUE;

    sup->port			= DEF_PORT;
    sup->config			= DEF_CONFIG;
    sup->gui			= DEF_GUI;

    sup->debug			= TRUE;
    sup->showActivity		= TRUE;
    sup->verbose		= TRUE;

    sup->use_cdl		= FALSE;
    sup->use_smc		= FALSE;
    sup->use_console		= TRUE;

    sup->imgSeqNo		= supSeqNo (-1);
    strcpy (sup->fileRoot, supSeqName (NULL));
    strcpy (sup->rawDir, DEF_RDIR);
    strcpy (sup->procDir, DEF_PDIR);
    memset (sup->userStr, 0, SZ_FNAME);

    sup->stat_interval		= DEF_INTERVAL;	/* status interval	   */

    sup->imtdev			= getenv ("RTD");
    if (!sup->imtdev)
        sup->imtdev		= getenv ("IMTDEV");

    if (!sup->cache_file) {
	/* No default cache file   */
	sup->cache_file = (char *) calloc (1, SZ_PATH);
    }
}


/*  SUP_STATUS -- Send a status message to the GUI.
 */
void
sup_status (char *msg)
{
    printf ("%s\n", msg);
}

/*  SUP_ERROR -- Send an error status message to the GUI.
 */
void
sup_error (char *msg)
{
    fprintf (stderr, "%s\n", msg);
}



/* SUP_MESSAGE -- Send a message to the user interface.
 */
void
sup_message (
  register supDataPtr sup,
  char *object,
  char *message
)
{
    char *msgbuf = calloc (1, strlen(message)+32);

    sprintf (msgbuf, "setValue {%s}", message);
    ObmDeliverMsg (sup->obm, object, msgbuf);

    free ((char *)msgbuf);
}


/* SUP_MSGI -- Like sup_message, but the message is an integer value.
 */
void
sup_msgi (supDataPtr sup, char *object, int value)
{
    char msgbuf[SZ_LINE];

    memset (msgbuf, 0, SZ_LINE);
    sprintf (msgbuf, "setValue {%d}", value);
    ObmDeliverMsg (sup->obm, object, msgbuf);
}


/* SUP_MESSAGEFROMFILE -- Send contents of a file to the UI object.
 */
void
sup_messageFromFile (supDataPtr sup, char *object, char *fname)
{
    struct stat fs;
    char *message = NULL;
    int status, nchars, fd = -1;


    if (stat (fname, &fs) >= 0) {
        nchars = fs.st_size;
        if ((message = (char *) XtMalloc (nchars + 1)) == NULL)
            goto err;
        if ((fd = open (fname, 0)) < 0)
            goto err;
        if (read (fd, message, nchars) != nchars)
            goto err;

        message[nchars] = '\0';
        status = ObmDeliverMsg (sup->obm, object, message);

        close (fd);
        XtFree ((char *)message);
	return;
    }
err:
    printf ("cannot access file %s\n", fname);
    if (fd >= 0)
        close (fd);
    if (message)
        XtFree ((char *)message);
}


/* SUP_ALERT -- Issue an alert to the server.  The message text input will
 * be displayed and either the ok (proceed) or cancel action will be taken,
 * causing the action text input to be sent back to the client to be
 * executed as a command.  This is used to alert the server (i.e. user) of
 * unusual circumstances and determine whether or not the server wants to
 * proceed.  An alert with no actions is a warning.
 */
void
sup_alert (
  supDataPtr sup,
  char *text,                     /* message text */
  char *ok_action,                /* command sent back to client for "ok" */
  char *cancel_action             /* command sent back to client for "cancel" */
)
{
    char msgbuf[SZ_LINE];
    sprintf (msgbuf, "setValue {{%s} {%s} {%s}}", text,
        ok_action ? ok_action : "", cancel_action ? cancel_action : "");
    ObmDeliverMsg (sup->obm, "alert", msgbuf);
}


/******************************************************************************/

void
supSetDebug (char *msg)
{
    char *ip, *op, flag[32];
    int level;


    memset (flag, 0, 32);
    for (ip=msg, op=flag; *ip && !isspace(*ip); )
	*op++ = *ip++;
    level = atoi (++ip);

    if (strcmp (flag, "super") == 0) {
	supDebugFile ("/tmp/SUP_DEBUG", level);
    } else if (strcmp (flag, "io") == 0) {
	supDebugFile ("/tmp/IO_DEBUG", level);
    } else if (strcmp (flag, "socket") == 0) {
	supDebugFile ("/tmp/SOCK_DEBUG", level);
    } else if (strcmp (flag, "sendrec") == 0) {
	supDebugFile ("/tmp/SR_DEBUG", level);
    } else if (strcmp (flag, "conf") == 0) {
	supDebugFile ("/tmp/CONF_DEBUG", level);
    } else if (strcmp (flag, "proc") == 0) {
	supDebugFile ("/tmp/PROC_DEBUG", level);
    } else if (strcmp (flag, "queue") == 0) {
	supDebugFile ("/tmp/QUEUE_DEBUG", level);
/*  } else if (strcmp (flag, "collector") == 0) {
	supDebugFile ("/tmp/COL_DEBUG", level);
    } else if (strcmp (flag, "pxf") == 0) {
	supDebugFile ("/tmp/PXF_DEBUG", level);
    } else if (strcmp (flag, "smc") == 0) {
	supDebugFile ("/tmp/SMC_DEBUG", level); */
    } else if (strcmp (flag, "mbus") == 0) {
        supDebugFile ("/tmp/MB_DEBUG", level);
    } else if (strcmp (flag, "all") == 0) {
        supDebugFile ("/tmp/SUP_DEBUG", level);
        supDebugFile ("/tmp/IO_DEBUG", level);
        supDebugFile ("/tmp/SOCK_DEBUG", level);
        supDebugFile ("/tmp/SR_DEBUG", level);
        supDebugFile ("/tmp/CONF_DEBUG", level);
        supDebugFile ("/tmp/PROC_DEBUG", level);
        supDebugFile ("/tmp/QUEUE_DEBUG", level);
/* 	supDebugFile ("/tmp/COL_DEBUG", level);
        supDebugFile ("/tmp/PXF_DEBUG", level);
        supDebugFile ("/tmp/SMC_DEBUG", level); */
        supDebugFile ("/tmp/MB_DEBUG", level);

    }

}


void
supDebugFile (char *file, int value)
{
    extern int errno;
    int  fd;

    if (value) {
        if (access (file, W_OK) < 0)
            fd = open (file, O_WRONLY | O_CREAT);
    } else {
        if (access (file, F_OK) == 0)
            unlink (file);
    }
}




/******************************************************************************
** SUP_SHUTDOWN -- Shutdown the Supervisor.
*/
void
sup_shutdown (supDataPtr sup)
{
    if (sup->cache_file)
	free ((void *) sup->cache_file);


    sup_status ("Supervisor shutting down....");
    mbusDisconnect (sup->mytid);


    sup_status ("Have a nice day.\n");
    exit (0);
}


/******************************************************************************
** SUPGETSEQNO -- Get/Set the current image sequence number.  If the value
** is negative, read the current value and return it, otherwise set the
** value.  The image sequence number is stored in the file $HOME/.seqno
*/
void 
initSeqNo (char *seqfile)
{
    FILE  *fd;

    if (access (seqfile, R_OK|W_OK) != 0)
        if ((fd = fopen (seqfile, "w+"))) {
	    fprintf (fd, "0 %s", DEF_ROOT);
	    fclose (fd);
        }
}

char *
nameOfSeqFile ()
{
    static char *home, seqfile[SZ_PATH];

    memset (seqfile, 0, SZ_PATH);
    if ((home = getenv ("HOME"))) {
        sprintf (seqfile, "%s/.seqno", home);
    } else {
	fprintf (stderr, "Warning: No '$HOME/.seqno' found, initializing...\n");
        strcpy (seqfile, "/tmp/.seqno");
    }

    return (seqfile);
}

int
supSeqNo (int seqnum)
{
    char  *seqfile;
    FILE  *fd;
    int  seqno = 0;


    /*  Initialize
     */
    initSeqNo ( (seqfile = nameOfSeqFile ()) );

    if (seqnum < 0) {
        if ((fd = fopen (seqfile, "r+")))
            fscanf (fd, "%d\n", &seqno);
    } else {
        if ((fd = fopen (seqfile, "w+")))
            fprintf (fd, "%d\n", seqnum);
    }
    if (fd) fclose (fd);

    return (seqno);
}

char *
supSeqName (char *name)
{
    static char  *seqfile, root[SZ_PATH];
    FILE  *fd;
    int  seqno = 0;


    /*  Initialize
     */
    initSeqNo ( (seqfile = nameOfSeqFile ()) );

    /*  Read existing values.
     */
    memset (root, 0, SZ_PATH);
    if ((fd = fopen (seqfile, "r+"))) {
        fscanf (fd, "%d %s\n", &seqno, root);
    	fclose (fd);
    }
    

    if (name) {
        if ((fd = fopen (seqfile, "w+"))) {
            fprintf (fd, "%d %s\n", seqno, name);
	    fclose (fd);
	}
        return (name);
    }

    return (root);
}


/*  USAGE -- Print a list of command-line options.
 */
static void
supUsage ()
{
    fprintf (stderr, "Usage:\n\n");
    sup_printOption ("    super");
    sup_printOption ("[-defgui]");                  /* Print default GUI      */
    sup_printOption ("[-gui <file>]");              /* GUI file               */
    sup_printOption ("[-help]");                    /* Print help             */
    sup_printOption ("[-host <name>]");             /* display host machine   */
    sup_printOption ("[-port <num>]");              /* inet port              */
    fprintf (stderr,"\n");
}


/* PRINTOPTION -- Pretty-print an option string.
 */
static int cpos = 0;

static void
sup_printOption (char *st)
{
        if (strlen(st) + cpos > 78) {
            fprintf (stderr,"\n\t");
            cpos = 8;
        }
        fprintf (stderr,"%s ",st);
        cpos = cpos + strlen(st) + 1;
}
