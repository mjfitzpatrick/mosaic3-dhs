#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_FITSIO_H)
#include "fitsio.h"
#endif

#include "smCache.h"
#include "pxf.h"



/*******************************************************************************
 * pxfFileOpen ( ... )
 *******************************************************************************/
void pxfFileOpen (XLONG *istat,		/* inherited status             */
		  char *resp,		/* response message             */
		  double *expID,	/* exposure identifier          */
		  smCache_t * smc, 	/* shared memory cache          */
		  fitsfile ** fd)	/* FITS file struct             */
{
    /* declare some variable and initialize them */
    int fitsStatus = 0;
    char *fitsDir = (char *) NULL;
    char fitsFile[DHS_MAXSTR] = { '\0' };
    char tFName[DHS_MAXSTR] = { '\0' };
    struct stat fitsExists;
    int stat, seqnum;

    pxfFileCreated = 0;

    (void) memset((void *) &fitsExists, 0, sizeof(struct stat));
    (void) memset((void *) &fitsFile[0], '\0', DHS_MAXSTR);

    switch (pxfFLAG) {
    case 1:
	fitsDir =
	    ((fitsDir =
	      getenv("MONSOON_DATA")) ==
	     (char *) NULL ? (char *) "./" : fitsDir);
	if (fitsDir[strlen(fitsDir) - 1] == '/')
	    fitsDir[strlen(fitsDir) - 1] = '\000';
	(void) strncpy(pxfDIR, fitsDir, DHS_MAXSTR);
	pxfFLAG = 3;
	(void) sprintf(tFName, "%s/%s", fitsDir, pxfFILENAME);
	break;
    case 2:
	(void) sprintf(tFName, "%s/%f", pxfDIR, *expID);
	break;
    case 3:
	(void) sprintf(tFName, "%s/%s", pxfDIR, pxfFILENAME);
	break;
    case 0:
    default:
	{
	    fitsDir =
		((fitsDir =
		  getenv("MONSOON_DATA")) ==
		 (char *) NULL ? (char *) "./" : fitsDir);
	    if (fitsDir[strlen(fitsDir) - 1] == '/')
		fitsDir[strlen(fitsDir) - 1] = '\000';

	    (void) sprintf(tFName, "%s/%f", fitsDir, *expID);
	}
    }

    DPRINTF(10, procDebug, "pxfFileOpen: tFName=>%s<\n", tFName);
    /*seqnum = smcGetSeqNo(smc); */
    seqnum = 0;

    sprintf(fitsFile, "%s%04d.fits", tFName, seqnum);

    stat = access(fitsFile, F_OK);
    if (stat != 0) {
	if (fits_create_file(fd, fitsFile, &fitsStatus)) {
	    DPRINTF(0, procDebug, "CANNOT CREATE FILE (%s). Try in '/tmp'",
		fitsFile);
	    if (fitsStatus == 105)	/* cannot create file. Put it in /tmp */
		sprintf(fitsFile, "/tmp/%s.fits", pxfFILENAME);
	    fitsStatus = 0;
	    if (fits_create_file(fd, fitsFile, &fitsStatus)) {
		*istat = ERROR;
		sprintf(resp, "pxfFileOpen: Could not create (%s)",
		    fitsFile);
	    }
	    sprintf(resp, "pxfFileOpen: **FILE (%s) CREATED in /tmp",
		    fitsFile);
	    return;
	}
    } else {
	/* the file exists from a previous exposure find a new name 
	   and create or open it.
	 */
	while (access(fitsFile, F_OK) == 0) {
	    sprintf(fitsFile, "%s%04d.fits", tFName, seqnum++);
	}
	/*smcSetSeqNo(smc, seqnum);*/
	if (fits_create_file(fd, fitsFile, &fitsStatus)) {
	    *istat = ERROR;
	    sprintf(resp,
		"pxfFileOpen: fits container file create2 failed, status=%d.\n",
		fitsStatus);
	    return;
	}
	strcpy(resp, "pxfFileOpen: Success.");
	return;
    }

    sprintf(resp, "pxfFileOpen: (%s) Success.", fitsFile);
    return;
}
