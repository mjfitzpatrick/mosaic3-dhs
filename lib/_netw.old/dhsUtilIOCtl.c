/*******************************************************************************
 * include(s):
 *******************************************************************************/
#if !defined(_dhsUtil_H_)
 #include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
 #include "dhsImplementationSpecifics.h"
#endif 

/*******************************************************************************
 * dhsUtilIOCtl ( ... )
 *******************************************************************************/ 
void dhsUtilIOCtl (
  long *istat,        /* inherited status                                     */
  char *resp,          /* response message                                     */
  dhsHandle dhsID,     /* dhs handle                                           */
  long ioctlFunction, /* ioctl function                                       */
  double *expID,       /* exposure identifier                                  */
  char *obsetID,       /* observation set identifier                           */
  ...                  /* variable argument list                               */
) {

  /* declare soime variables and initialize them */
  va_list args;
  char *fname = (char *)NULL;
  char *dirname = (char *)NULL;

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');
    
  /* check input parameter(s) */
  /* COMMENTED OUT BECAUSE WE DON'T NEED THESE CHECKS!
    if ( expID == (double *)NULL ) {
     *istat = DHS_ERROR;
     (void) sprintf(resp,"bad parameter expID==NULL");
     MNSN_RPT_FAILURE(resp,"dhsUtilIOCtl: Failed. \\\\");
     return;
    }
  */
  /* COMMENTED OUT BECAUSE WE DON'T NEED THESE CHECKS!
    if ( obsetID == (char *)NULL ) {
     *istat = DHS_ERROR;
     (void) sprintf(resp,"bad parameter obsetID==NULL");
     MNSN_RPT_FAILURE(resp,"dhsUtilIOCtl: Failed. \\\\");
     return;
    }
  */
  /* COMMENTED OUT BECAUSE WE DON'T NEED THESE CHECKS!
    if ( dhsID < (dhsHandle)0 ) {
     *istat = DHS_ERROR;
     (void) sprintf(resp,"bad parameter dhsID<0");
     MNSN_RPT_FAILURE(resp,"dhsUtilIOCtl: Failed. \\\\");
     return;
    }
  */

  /* start va args */
  va_start(args,obsetID); 

  /* switch on ioctlFunction */
  switch (ioctlFunction) {
   case DHS_IOC_SETFILENAME:
    fname = va_arg(args,char *);
    (void) strncpy(dhsUtilFILENAME,fname,DHS_MAXSTR);
    dhsUtilFLAG = (dhsUtilFLAG > 0) ? 3 : 1;
    *istat = DHS_OK;
    (void) sprintf(resp,"filename set to %s",fname);
    MNSN_RET_SUCCESS(resp,"dhsUtilIOCtl: Success. \\\\");
    break;
   case DHS_IOC_SETDIR:
    dirname = va_arg(args,char *);
    (void) strncpy(dhsUtilDIR,dirname,DHS_MAXSTR);
    dhsUtilFLAG = (dhsUtilFLAG > 0) ? 3 : 2;
    *istat = DHS_OK;
    (void) sprintf(resp,"directory set to %s",dirname);
    MNSN_RET_SUCCESS(resp,"dhsUtilIOCtl: Success. \\\\");
    break;
   case DHS_IOC_OBSCONFIG:
   case DHS_IOC_MDCONFIG:
   case DHS_IOC_FPCONFIG:
   case DHS_IOC_KEYWORD_TRANS:
   case DHS_IOC_DEBUG_LVL:
   case DHS_IOC_SIMULATION:
    *istat = DHS_OK;
    (void) sprintf(resp,"ioctlFunction=%ld not implemented",(long)ioctlFunction);
    MNSN_RET_SUCCESS(resp,"dhsUtilIOCtl: Success. \\\\");
    break;
   default:
    *istat = DHS_ERROR;
    (void) sprintf(resp,"unknown ioctlFunction=%ld",(long)ioctlFunction);
    MNSN_RPT_FAILURE(resp,"dhsUtilIOCtl: Failed. \\\\");
    break;
  }

  /* end va args */
  va_end(args);

  /* return */
  return;
}
