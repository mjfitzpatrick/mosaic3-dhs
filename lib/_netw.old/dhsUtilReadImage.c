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
 * dhsUtilReadImage ( ... )
 *******************************************************************************/ 
void dhsUtilReadImage (
  long *istat,    /* inherited status                                         */
  char *resp,      /* response message                                         */
  dhsHandle dhsID  /* dhs handle                                               */
) {

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');

  /* if simulating, return */
  if ( dcaSimulator() ) return;
    
  /* check input parameter(s) */
  if ( dhsID < (dhsHandle)0 ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"bad parameter dhsID<0");
   MNSN_RPT_FAILURE(resp,"dhsUtilReadImage: Failed. \\\\");
   return;
  }

  /* do something for _netw variant */
  *istat = DHS_OK;

  /* return */
  MNSN_RET_SUCCESS(resp,"dhsUtilReadImage: Success.");
  return;
}
