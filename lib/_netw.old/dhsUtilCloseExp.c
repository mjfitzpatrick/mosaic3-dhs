/*******************************************************************************
 * include(s):
 *******************************************************************************/
#if !defined(_dhsUtil_H_)
 #include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
 #include "dhsImplementationSpecifics.h"
#endif 
#include "dcaDhs.h"

/*******************************************************************************
 * dhsUtilCloseExp ( ... )
 *******************************************************************************/ 
void dhsUtilCloseExp (
  long *istat,    /* inherited status                                         */
  char *resp,      /* response message                                         */
  dhsHandle dhsID, /* dhs handle                                               */
  double expID     /* exposure identifier                                      */
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
   MNSN_RPT_FAILURE(resp,"dhsUtilCloseExp: Failed. \\\\");
   return;
  }
  if ( expID <= (double)0.0 ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"bad parameter expID<=0.0");
   MNSN_RPT_FAILURE(resp,"dhsUtilCloseExp: Failed. \\\\");
   return;
  }

  /* do something for _netw variant */
  *istat = dcaSendMsg((int)dhsNW.collector->fd,dcaFmtMsg(DCA_CLOSE|DCA_EXP,DHS_IAMPAN));
  *istat = dcaSendMsg((int)dhsNW.super->fd,dcaFmtMsg(DCA_CLOSE|DCA_EXP,DHS_IAMPAN));

  /* return */
  *istat = DHS_OK;
  MNSN_RET_SUCCESS(resp,"dhsUtilCloseExp: Success.");
  return;
}
