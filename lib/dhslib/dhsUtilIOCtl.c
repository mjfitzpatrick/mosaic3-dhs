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
void dhsUtilIOCtl ( XLONG *istat,        /* inherited status                   */
		    char *resp,          /* response message                   */
		    dhsHandle dhsID,     /* dhs handle                         */
		    XLONG ioctlFunction, /* ioctl function                     */
		    double *expID,       /* exposure identifier                */
		    char *obsetID,       /* observation set identifier         */
		    ...                  /* variable argument list              */
    ) 
{
    va_list args;
    char *fname, *dirname;
 
    /* check the inherited status */
    if ( STATUS_BAD(*istat) ) return;
    MNSN_CLEAR_STR(resp,'\0');
    
    /* check input values   Cannot check for < 0 since it is an structure
                            See this later
    if ( dhsID < (dhsHandle)0 ) 
    {
	*istat = DHS_ERROR;
	MNSN_RET_FAILURE(resp,"dhsUtilIOCtl:  bad parameter dhsHandle invalid.");
	return;
    }
*/
    va_start(args,obsetID); 

    /* switch on ioctlFunction */
    switch (ioctlFunction) 
    {
      case DHS_IOC_SETFILENAME:			/* set new base file name */
	  fname = va_arg(args,char *);          /* get the new name from the argument */
	  strncpy(dhsUtilFILENAME,fname, DHS_MAXSTR);
	  dhsUtilFLAG = (dhsUtilFLAG > 0) ? 3 : 1;
	  sprintf(resp,"dhsUtilIOCtl: Success filename set too %s.",fname);
	  MNSN_RET_SUCCESS(resp, "");
	  *istat = DHS_OK;
	  break;
      case DHS_IOC_SETDIR:			/* set new directory name */
	  dirname = va_arg(args,char *);
	  strncpy(dhsUtilDIR, dirname, DHS_MAXSTR);
	  dhsUtilFLAG = (dhsUtilFLAG > 0) ? 3 : 2;
	  sprintf(resp,"dhsUtilIOCtl: Success directory set too %s.",dirname);
	  *istat = DHS_OK;
	  MNSN_RET_SUCCESS(resp, "");
	  break;
/*       case DHS_LAB_VERSION: */
/* 	  dhsLabVersion = va_arg(args,int); */
/* 	  break; */
      case DHS_IOC_OBSCONFIG:
      case DHS_IOC_MDCONFIG:
      case DHS_IOC_FPCONFIG:
      case DHS_IOC_KEYWORD_TRANS:
      case DHS_IOC_DEBUG_LVL:
      case DHS_IOC_SIMULATION:
	  *istat = DHS_OK;
	   MNSN_RET_SUCCESS(resp,"dhsUtilIOCtl: function Not Implemented.");
	  break;
      default:
	  *istat = DHS_ERROR;
	  sprintf(resp,"dhsUtilIOCtl: Failed, bad ioctl function %d",(int)ioctlFunction);
	  MNSN_RET_FAILURE(resp,"");
	  break;
    }

    va_end(args);
    /* return */
    return;
}
