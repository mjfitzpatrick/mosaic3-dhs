#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>

#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_FITSIO_H)
#include "fitsio.h"
#endif

#include "smCache.h"
#include "pxf.h"


extern int	console, verbose;
extern int	procDebug;
extern int	noop;


void pxfFileOpen();
void pxfSendPixelData();
void pxfSendMetaData();



/*  NOTE:  Assumes an integer data array.
 */
void
procFITSData (smcPage_t *p, fitsfile *fd)
{
    int  *idata;
    XLONG data_size;
    XLONG istat;
    double expID;
    smcSegment_t *seg = (smcSegment_t *) NULL;
    fpConfig_t *fpCfg = smcGetFPConfig (p);
    char obsetID[80], resp[80];

    seg = SEG_ADDR(p);
    data_size = seg->dsize;

    if (console)
	fprintf (stderr, "Processing DATA page: 0x%x size=%d\n",
	    (int)seg, data_size);

    istat = 0;
    strcpy(obsetID,smcGetObsetID(p));
    idata = (int *) smcGetPageData (p);

    expID=smcGetExpID(p);
/*  strcpy(obsetID,smcGetObsetID(p)); */
    pxfSendPixelData(&istat, resp, (void *)idata, (size_t)data_size,
	(fpConfig_t *)fpCfg, &expID, obsetID, fd);

    DPRINTF(30,procDebug,"%s\n",resp);
}


void
procFITSMetaData (smcPage_t *p, fitsfile *fd)
{
    int   data_size;
    char *cdata, *ip, *edata;
    smcSegment_t *seg = (smcSegment_t *) NULL; 
    mdConfig_t *mdCfg = smcGetMDConfig (p);
    double expID;
    char obsetID[80], resp[80];
    XLONG istat;


    seg = SEG_ADDR(p);
    data_size = seg->dsize;

    if (console)
	fprintf (stderr, "Processing META page: 0x%x size=%d\n", 
	    (int) seg, data_size);

    cdata = ip = (char *) smcGetPageData (p);
    edata = cdata + data_size;

    expID = smcGetExpID(p);
    strcpy (obsetID,smcGetObsetID(p));

    pxfSendMetaData (&istat, resp, (char *)cdata, (size_t)data_size,
	(mdConfig_t *)mdCfg, &expID, obsetID, fd);
    ip = cdata;				  /* initialize			*/
    edata = (cdata + data_size);	  /* find addr of end of data	*/

    DPRINTF(30,procDebug,"%s\n",resp);
}


/******************************************************************************
** pxfFileOpen ( ... )
******************************************************************************/
void pxfFileOpen(XLONG *istat,		/* inherited status 		*/
		 char *resp,		/* response message             */
		 double *expID,		/* exposure identifier          */
		 smCache_t * smc, fitsfile ** fd)
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


/*******************************************************************************
 * pxfSendMetaData ( ... )
 ******************************************************************************/
void pxfSendMetaData(XLONG *istat,	/* inherited status                */
		     char *resp,	/* response message                */
		     void *blkAddr,	/* address of data block           */
		     size_t blkSize,	/* size of data block              */
		     mdConfig_t * cfg,	/* configuration of meta data      */
		     double *expID,	/* exposure identifier             */
		     char *obsetID,	/* observation set identifier      */
		     fitsfile * fitsID)
{
    /* declare some variable and initialize them */
    int nr = 0;
    int fc = 0;
    int cpos = 0;
    char *op;
    int fitsStatus = 0;
    char *avpName[] = { "Name", "Value", "Comment" };
    char *avpFormat[] = { "A32", "A32", "A64" };
    char *avpUnits[] = { "string", "string", "string" };
    char avpCol1[DHS_AVP_NAMESIZE] = { '\0' };
    char avpCol2[DHS_AVP_VALSIZE] = { '\0' };
    char avpCol3[DHS_AVP_COMMENT] = { '\0' };
    char *avpRecord[4] = { avpCol1, avpCol2, avpCol3, (char *) NULL };
    char fitsRecord[DHS_FITS_RECLEN] = { '\0' };


    /* checkSaver/ the inherited status */
    if (STATUS_BAD(*istat))
	return;
    MNSN_CLEAR_STR(resp, '\0');

    /* check input parameters */
    if (cfg == (mdConfig_t *) NULL || expID == (double *) NULL) {
	*istat = DHS_ERROR;
	sprintf(resp, "pxfSendMetaData: bad parameter, %s",
		((cfg ==
		  (mdConfig_t *) NULL)) ? "MetaData configuration==NULL" :
		"expID==NULL");
	return;
    }

    /* check address/size */
    if (blkAddr == (void *) NULL && blkSize != (size_t) 0) {
	*istat = DHS_ERROR;
	strcpy(resp,
	       "pxfSendMetaData: bad parameters BlkAddr==NULL and size!=0");
	return;
    }

    DPRINTF(15, procDebug, "pxfSendMetaData: blkSize = %ld\n", blkSize);
    /* return OK if size is 0 */
    if (blkSize == (size_t) 0) {
	*istat = DHS_OK;
	strcpy(resp, "pxfSendMetaData: Success. blkSize==0");
	return;
    }


    /* write headers here */
    switch (cfg->metaType) {
    case DHS_MDTYPE_FITSHEADER:	/* FITS header records */
	op = (char *) blkAddr;
	nr = (int) blkSize / DHS_FITS_RAWLEN;
	DPRINTF(200, procDebug, "pxfSendMetaData: NumRec= %ld\n", nr);
	for (fc = 0; fc < nr; fc++) {
	    DPRINTF(200, procDebug, "pxfSendMetaData: RecCount = %d\n",
		    fc);
	    DPRINTF(200, procDebug, "rec=>%-74s<\n", op);

	    /* initialize record */
	    cpos = 0;
	    (void) memset((void *) &fitsRecord[cpos], '\0',
			  (size_t) DHS_FITS_RECLEN);
	    /* copy name and increment pointers/counters */
	    (void) memmove((void *) &fitsRecord[cpos], (void *) op,
			   (size_t) DHS_FITS_NAMESIZE);
	    op += DHS_FITS_NAMESIZE;
	    cpos += DHS_FITS_NAMESIZE;
	    /* copy first separator and increment pointers/counters */
	    (void) memmove((void *) &fitsRecord[cpos], "= ",
			   (size_t) DHS_FITS_SEP1);
	    cpos += DHS_FITS_SEP1;
	    /* copy value and increment pointers/counters */
	    (void) memmove((void *) &fitsRecord[cpos], (void *) op,
			   (size_t) DHS_FITS_VALSIZE);
	    op += DHS_FITS_VALSIZE;
	    cpos += DHS_FITS_VALSIZE;
	    /* copy second separator and increment pointers/counters */
	    (void) memmove((void *) &fitsRecord[cpos], " / ",
			   (size_t) DHS_FITS_SEP2);
	    cpos += DHS_FITS_SEP2;
	    /* copy comment and increment pointers/counters */
	    (void) memmove((void *) &fitsRecord[cpos], (void *) op,
			   (size_t) DHS_FITS_COMMENT);
	    op += DHS_FITS_COMMENT;
	    DPRINTF(200, procDebug, "rec=>%-80s<\n", fitsRecord);
	    /* now write out the record */
	    if (ffprec(fitsID, fitsRecord, &fitsStatus)) {
		*istat = DHS_OK;
		sprintf(resp,
			"pxfSendMetaData: fits write failed at record %d with fitsStatus = %d",
			fc + 1, fitsStatus);
	    }
	}
	break;
    case DHS_MDTYPE_AVPAIR:	/* AVP header records as FITS ASCII table */
	nr = (int) blkSize / DHS_AVP_RAWLEN;
	/* create an ascii table */
	if (obsetID == (char *) NULL || strlen(obsetID) <= (size_t) 1) {
	    ffcrtb(fitsID, ASCII_TBL, nr, cfg->numFields,
		   (char **) &avpName, (char **) &avpFormat,
		   (char **) &avpUnits, "PAN AV-Pairs List", &fitsStatus);
	} else {
	    ffcrtb(fitsID, ASCII_TBL, nr, cfg->numFields,
		   (char **) &avpName, (char **) &avpFormat,
		   (char **) &avpUnits, obsetID, &fitsStatus);
	}
	if (fitsStatus) {
	    *istat = DHS_ERROR;
	    sprintf(resp,
		    "pxfSendMetaData:  fits ascii table create failed.  fitsStatus = %d",
		    fitsStatus);
	    break;
	}
	(void) ffpkyj(fitsID, "EXTTYPE", cfg->metaType, "meta data type",
		      &fitsStatus);
	/* populate the ascii table */
	op = (char *) blkAddr;
	for (fc = 0; fc < nr; fc++) {
	    (void) memmove((void *) &avpCol1[0], (void *) op,
			   (size_t) DHS_AVP_NAMESIZE);
	    op += DHS_AVP_NAMESIZE;
	    if (ffpcls
		(fitsID, 1, fc + 1, 1, 1, (char **) &avpRecord[0],
		 &fitsStatus)) {
		*istat = DHS_ERROR;
		sprintf(resp,
			"pxfSendMetaData:  fits ascii table write1 failed. fitsStatus = %d",
			fitsStatus);
		break;
	    }
	    (void) memmove((void *) &avpCol2[0], (void *) op,
			   (size_t) DHS_AVP_VALSIZE);
	    op += DHS_AVP_VALSIZE;
	    if (ffpcls
		(fitsID, 2, fc + 1, 1, 1, (char **) &avpRecord[1],
		 &fitsStatus)) {
		*istat = DHS_ERROR;
		sprintf(resp,
			"pxfSendMetaData:  fits ascii table write2 failed. fitsStatus = %d",
			fitsStatus);
		break;
	    }
	    (void) memmove((void *) &avpCol3[0], (void *) op,
			   (size_t) DHS_AVP_COMMENT);
	    op += DHS_AVP_COMMENT;
	    if (ffpcls
		(fitsID, 3, fc + 1, 1, 1, (char **) &avpRecord[2],
		 &fitsStatus)) {
		sprintf(resp,
			"pxfSendMetaData:  fits ascii table write3 failed. fitsStatus = %d",
			fitsStatus);
		break;
	    }
	}
	break;

    case DHS_MDTYPE_SHIFTLIST:	/* MONSOON Pixel shift list */
	*istat = DHS_OK;
	strcpy(resp,
	       "pxfSendMetaData:  metadata type SHIFTLIST not yet implemented");
	break;

    case DHS_MDTYPE_ARRAYDATA:	/* MONSOON data array ??? */
	*istat = DHS_OK;
	strcpy(resp,
	       "pxfSendMetaData:  metadata type ARRAYDATA not yet implemented");
	break;

    default:
	break;
    }

    /* flush and close */
    (void) ffflus(fitsID, &fitsStatus);
    fitsStatus = 0;
/*
    if ( fits_close_file(fitsID,&fitsStatus) ) 
    {
	*istat = DHS_ERROR;
	sprintf(resp,"pxfSendMetaData:  fits close failed. fitsStatus = %d", fitsStatus);
	return;
    }
*/
    /* return */
    strcpy(resp, "pxfSendMetaData:  Success");
    return;
}


/*******************************************************************************
 * pxfSendPixelData ( ... )
 *******************************************************************************/
void pxfSendPixelData(XLONG *istat,	/* inherited status               */
		      char *resp,	/* response message               */
		      void *pxlAddr,	/* address of data block          */
		      size_t blkSize,	/* size of data block             */
		      fpConfig_t * cfg,	/* configuration of pixel data    */
		      double *expID,	/* exposure identifier            */
		      char *obsetID,	/* observation set identifier     */
		      fitsfile * fitsID)
{
    /* declare some variable and initialize them */
    int   nx, ny, bitpix = 32, fs = 0;
    XLONG nelms = 0L;
    XLONG naxis = 2L;
    XLONG naxes[2] = { 0L, 0L };
    char fitsFile[DHS_MAXSTR] = { '\0' };
    struct stat sts;
    XLONG *ip;


    (void) memset((void *) &sts, 0, sizeof(struct stat));
    (void) memset((void *) &fitsFile[0], '\0', DHS_MAXSTR);

    /* check the inherited status */
    if (STATUS_BAD(*istat))
	return;
    MNSN_CLEAR_STR(resp, '\0');

    /* check input parameters */
    if (pxlAddr == (void *) NULL || expID == (double *) NULL) {
	*istat = DHS_ERROR;
	sprintf(resp, "dhsSendPixelData: Bad parameter. %s",
		((expID ==
		  (double *) NULL) ? "expID ptr==NULL." :
		 "FP config ptr == NULL."));
	return;
    }

    DPRINTF(10, procDebug, "dhsSendPixelData: fitsFile=>%s<\n", fitsFile);

    ip = (XLONG *) pxlAddr;
    nx = cfg->xSize;
    ny = cfg->ySize;

    naxes[0] = nx;
    naxes[1] = ny;
    nelms = naxes[0] * naxes[1];

    /* make it a FITS mage file */
    if (fits_create_img(fitsID, bitpix, naxis, (long *)naxes, &fs)) {
	*istat = DHS_ERROR;
	sprintf(resp,
		"dhsSendPixelData:  fits image file create failed, fitsStatus = %d",
		fs);
	(void) fits_close_file(fitsID, &fs);
	return;
    }


    /*  FLIP the images */
    /*  If we are in Pan A, Array 2 needs to be rotated in 90 degrees 
     *  clockwise.
     */

    /* write data as XLONG integers */
    if (fits_write_img(fitsID, TLONG, 1L, nelms, pxlAddr, &fs)) {
	*istat = DHS_ERROR;
	sprintf(resp,
		"dhsSendPixelData:  fits write image file failed, fitsStatus = %d",
		fs);
	(void) fits_close_file(fitsID, &fs);
	return;
    }

    fits_write_date(fitsID, &fs);
    fits_update_key(fitsID, TDOUBLE, "EXPID", expID, "dhs exposure ID",
		    &fs);
    fits_update_key(fitsID, TSTRING, "OBSETID", (char *) obsetID,
		    "Observation ID", &fs);

    /* flush and close */
    (void) ffflus(fitsID, &fs);

/*
    if ( fits_close_file(fitsID,&fs) ) 
    {
	*istat = DHS_ERROR;
	 sprintf(resp,"dhsSendPixelData: fits close failed, fitsStatus = %d", fs);
	return;
    }
*/
    *istat = DHS_OK;
    /* return */
    sprintf(resp, "dhsSendPixelData: (%s) Success.", fitsFile);
    return;
}
