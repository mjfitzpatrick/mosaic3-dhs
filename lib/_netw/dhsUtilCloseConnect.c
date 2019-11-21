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
 * dhsUtilCloseConnect ( ... )
 *******************************************************************************/ 
void dhsUtilCloseConnect (
  long *istat,    /* inherited status                                         */
  char *resp,      /* response message                                         */
  dhsHandle dhsID  /* dhs handle                                               */
) {

  /* declare some variables and initialize them */
  int stat = 0;
  int socket = 0;

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');

  /* if simulating, return */
  if ( dcaSimulator() ) return;

  /* check input parameter(s) */
  if ( dhsID < (dhsHandle)0 ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"bad parameter dhsID<0");
   MNSN_RPT_FAILURE(resp,"dhsUtilCloseConnect: Failed. \\\\");
   return;
  }

  /* do something for _netw variant */
  socket = dhsNW.collector->fd;
  stat = dcaSendMsg(socket,dcaFmtMsg(DCA_CLOSE|DCA_CONNECT,(int)NULL));
  socket = dhsNW.super->fd;
  stat = dcaSendMsg (socket,dcaFmtMsg(DCA_CLOSE|DCA_CONNECT,(int)NULL));
  close(dhsNW.collector->fd);
  close(dhsNW.super->fd);

  /* return */
  *istat = DHS_OK;
  MNSN_RET_SUCCESS(resp,"dhsUtilCloseConnect: Success.");
  return;
}
