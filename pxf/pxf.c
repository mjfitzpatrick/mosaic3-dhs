#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>


/*  Implementation headers.
 */
#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif
#if !defined(_FITSIO_H)
#include "fitsio.h"
#endif


#include "smCache.h"
#include "mbus.h"
#include "pxf.h"

extern smCache_t *smcOpen();
extern smcPage_t *smcGetPage();

smCache_t    *smc    	= (smCache_t *) NULL;
smcPage_t    *page   	= (smcPage_t *) NULL;
smcSegment_t *seg 	= (smcSegment_t *) NULL;

char *procPages	  	= (char *) NULL;


/*  Prototypes
*/
void  pxfFileOpen (XLONG *istat, char *resp, double *expID, smCache_t *smc,
			fitsfile **fd);
int   mbusMsgHandler (int fd, void *client_data); 	/* I/O handler  */


int	save_fits	= FALSE;	/* Output stream type		*/
int	save_mbus	= TRUE;		
					/* Message bus variables	*/
int	use_mbus	= TRUE;		
int	console		= FALSE;
int	verbose		= FALSE;
int	noop		= FALSE;

int	mb_fd		= 0;
int	mb_tid		= 0;
int	dca_tid		= 0;
int	seqno		= 0;

int	debug		= 0;


/**
 *  Program main.
 */
int
main (int argc, char **argv) 
{
    char     fname[64], savex[32], config[128], resp[80];
    int      i, index, len, dirflg=0, fiflg=0, fs=0;
    double   expID;
    XLONG    istat, lexpID, saveExpID;
    fitsfile *fd = (fitsfile *)NULL; 


    procDebug = 500;    /* Debug flag for DPRINT macro */

    if (argc == 2) {
       printf ("Usage: pxf -dir <Output FITS dir> -froot <FITS root name>\n");
       printf ("           -debug_level 'int >= 10'\n");
       return (0);
    }

    /* Initialize and process arguments.
     */
    memset (pxfDIR, 0, DHS_MAXSTR); 	 strcpy (pxfDIR,"/home/data");
    memset (pxfFILENAME, 0, DHS_MAXSTR); strcpy (pxfFILENAME, "image");

    for (i=1; i < argc; i++) {
        if (strncmp (argv[i], "-dir", 3) == 0) {
            strcpy (pxfDIR,  argv[++i]),      dirflg = 1;

        } else if (strncmp (argv[i], "-froot", 3) == 0) {
            strcpy (pxfFILENAME, argv[++i]),  fiflg = 1;

        } else if (strncmp (argv[i], "-console", 3) == 0) {
            console++;

        } else if (strncmp (argv[i], "-verbose", 3) == 0) {
            verbose++;

        } else if (strncmp (argv[i], "-host", 3) == 0) {
            mbInitMBHost ();
            if ((argc - i) > 1 && (argv[i+1][0] != '-'))
                mbSetSimHost (argv[++i], 1);
            else {
                fprintf (stderr, "Error: '-host' requires an argument\n");
                exit (1);
            }

        } else if (strncmp (argv[i], "-debug", 3) == 0) {
            procDebug = atoi (argv[++i]);

        } else if (strncmp (argv[i], "-noop", 2) == 0) {
            noop = TRUE;

        } else if (strncmp (argv[i], "-mbus", 3) == 0) {
            use_mbus  = TRUE;
            save_mbus = TRUE;
            save_fits = FALSE;

        } else if (strncmp (argv[i], "-proc", 3) == 0) {
	    procPages = argv[++i];

        } else if (strncmp (argv[i], "-fits", 3) == 0) {
            save_mbus = FALSE;
            save_fits = TRUE;

        } else if (strncmp (argv[i], "-nombus", 5) == 0) {
            use_mbus  = FALSE;
            save_mbus = FALSE;
            save_fits = TRUE;

        } else {
            printf ("Unrecognized option: %s\n", argv[i]);
	    printf ("Usage: pxf -dir <Output dir> -froot <root name>\n");
	    printf ("           -debug_level 'int >= 10'\n");
	    return (0);
        }
    }
    memset (config, 0, 128);	sprintf (config, "debug=%d", procDebug);
    

    /*  Open/Attach to the cache.
     */
    if ((smc = smcOpen (config)) == (smCache_t *)NULL)
	fprintf (stderr, "Error opening cache, invalid file?\n");


    if (use_mbus) {
        /* Open a connection on the message bus.
        */
        if ((mb_tid = mbusConnect ("PXF", "PXF", FALSE)) <= 0) {
            fprintf (stderr, "ERROR: Can't connect to message bus.\n");
            exit (1);
        }

        if ((mb_fd = mbGetMBusFD ()) >= 0) {     /* Add the input handlers. */
            mbusAddInputHandler (mb_fd, mbusMsgHandler, NULL);

        } else {
            fprintf (stderr, "ERROR: Can't install MBUS input handler.\n");
            exit (1);
        }
    }


    /*  Send initial status to the Supervisor for display.
    */
    if (use_mbus) {
	if (console)
            fprintf (stderr, "Waiting for input...\n");
        mbusSend (SUPERVISOR, ANY, MB_STATUS, "Waiting for data...");

	/*  Get the initial sequence number. 	*/
	/*  Get the initial Directory path.  	*/
	/*  Get the initial FName root. 	*/

        /*  Begin processing I/O events.  Note: This never returns....
        */
        mbusSend (SUPERVISOR, ANY, MB_READY, "READY PXF");
        mbusAppMainLoop (NULL);

    } else {

	/* If we're not using the message bus, the 'save_fits' flag has
	** already been set, so don't check for it here.  We'll never use
	** this task as a one-off feed to the message bus.
	*/

        smcSetSeqNo  (smc, 0);	/* Initialize sequence number		*/

        if (dirflg == 0) {   	/* Get Dirname from share memory 	*/
            len = min (250, strlen ((char *)smcGetDir(smc)));
            memmove (pxfDIR, (char *)smcGetDir (smc), len);
        }
        if (fiflg == 0) {   	/* Get Filename from share memory 	*/
            len = min (250, strlen ((char *)smcGetFRoot (smc)));
            memmove (pxfFILENAME, (char *)smcGetFRoot (smc), len);
        }
    
        pxfFLAG = 3;   /* indicates that DIR and FILENAME are defined. 	*/


        /*  Simulate the readout sequence
         */
        i = 0;
        index = 1;  
        savex[0] = ' ';
        saveExpID = 0;

	memset (fname, 0, 128);   		/* save root filename  */
        strcpy (fname, pxfFILENAME);

        while ((page = smcNextPage (smc, 1000))) {
	    smcAttach (smc, page); 		/* Attach to the page  */
	    switch (page->type) { 		/* Process the data    */
	    case TY_VOID:
	        procFITSData (page, fd);
	        break;
	    case TY_DATA:
	        procFITSData (page, fd);
	        break;
	    case TY_META:
	        expID = smcGetExpID (page);
	        lexpID = expID; 
                lexpID = (expID - lexpID) * 100000;
                if (lexpID != saveExpID) {
                     if (fd != (fitsfile *) NULL) {
		        if (fits_close_file (fd, &fs) ) {
		            DPRINTF (10, procDebug,
                                 "pxf: fits close failed, fitsStatus = %d", fs);
			    exit (0);
		        }
                     }
		     len = min (250, strlen ((char *) smcGetPageDir (page)));
		     if (dirflg == 0 && len > 0)
		         memmove (pxfDIR, (char *) smcGetPageDir (page), len);
		     len = min (250, strlen ((char *) smcGetPageFRoot (page)));
		     if (fiflg == 0 && len > 0) {
		         memmove (pxfFILENAME, (char *) smcGetPageFRoot (page),
			    len);
		     } 
 
	             pxfFileOpen (&istat, resp, &expID, smc, &fd); 
    	             DPRINTF (10, procDebug, "%s\n", resp);
                     saveExpID = lexpID;
                 }

	        procFITSMetaData (page, fd);
	        break;
	    default:
	        break;
	    }

	    i++;
	    smcMutexOn ();
	    smcDetach (smc, page,TRUE); 	/*  Detach from the page.  */
	    smcMutexOff ();
        }

        if (fits_close_file (fd, &fs) ) {
    	    DPRINTF (10, procDebug, "pxf: fits close failed, status = %d", fs);
	    exit (0);
        }

        /*
        if (smc && !interactive) 
	    smcClose (smc, clear);
        */
    }

    return (0);
}
