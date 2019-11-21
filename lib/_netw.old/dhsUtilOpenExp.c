/*******************************************************************************
 * include(s):
 *******************************************************************************/
#if !defined(_dhsUtil_H_)
 #include "dhsUtil.h"
#endif
#if !defined(_sockUtil_H_)
 #include "sockUtil.h"
#endif
#if !defined(_dhsImpl_H_)
 #include "dhsImplementationSpecifics.h"
#endif 
#include "dcaDhs.h"

int dhsConnectCollector ( long *istat, char *resp, long whoami );

/*******************************************************************************
 * dhsUtilOpenExp ( ... )
 *******************************************************************************/ 
void dhsUtilOpenExp (
  long *istat,    /* inherited status                                         */
  char *resp,      /* response message                                         */
  dhsHandle dhsID, /* dhs handle                                               */
  fpConfig_t *cfg, /* focal plane configuration                                */
  double *expID,   /* exposure identifier                                      */
  char *obsetID    /* observation set identifier                               */
) {

  /* declare some variables and initialize them */
  int socket = 0;
  int stat = 0;
  int szt = 0;
  char line[MAXMSG];
  (void) memset((void *)line,'\0',MAXMSG);

  (void) sprintf(line,"dhsOpenExp: starting istat=%ld",*istat);
  MNSN_REPORT_MSG(resp,line);

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');
    
  /* if simulating, return */
  if ( dcaSimulator() ) return;

  /* check input parameter(s) */
  if ( cfg == (fpConfig_t *)NULL ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"bad parameter cfg==NULL");
   MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
   return;
  }
  if ( expID == (double *)NULL ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"bad parameter expID==NULL");
   MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
   return;
  }
  if ( obsetID == (char *)NULL ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"bad parameter obsetID==NULL");
   MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
   return;
  }
  if ( dhsID < (dhsHandle)0 ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"bad parameter dhsID<0");
   MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
   return;
  }

  /* validate the connection to supervisor */
  (void) sprintf(line,"dhsOpenExp: dhsNW.super->fd=%d",(int)dhsNW.super->fd);
  MNSN_REPORT_MSG(resp,line);
  (void) sprintf(line,"dhsOpenExp: dhsNW.collector->fd=%d",(int)dhsNW.collector->fd);
  MNSN_REPORT_MSG(resp,line);
  if ( (stat=dcaValidateChannel(dhsNW.super)) == DCA_ERR ) {
   (void) sprintf(resp,"failed to validate channel to supervisor");
   MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
   if ( (stat=dcaInitChannel(&dhsNW,DCA_SUPERVISOR))==DCA_ERR || (stat=dcaValidateChannel(dhsNW.super))==DCA_ERR ) {
    *istat = DHS_ERROR;
    (void) sprintf(resp,"failed to initialize channel to supervisor");
    MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
    return;
   }
  }
  (void) sprintf(line,"dhsOpenExp: valid connection to supervisor");
  MNSN_REPORT_MSG(resp,line);

  /* set value(s) */
  dhs.expID = *expID;
  dhs.obsSetID = obsetID;

  /* validate channel to collector */
  if ( (stat=dcaValidateChannel(dhsNW.collector)) != DCA_ERR ) {
retry:
   socket = (int)dhsNW.collector->fd;
   (void) sprintf(line,"dhsOpenExp: sending fpConfig to fd=%d",socket);
   MNSN_REPORT_MSG(resp,line);
   /* send an OpenExposure to the collector and supervisor */
   stat = dcaSendMsg(dhsNW.collector->fd,dcaFmtMsg(DCA_OPEN|DCA_EXP,DHS_IAMPAN));
   stat = dcaSendMsg(dhsNW.super->fd,dcaFmtMsg(DCA_OPEN|DCA_EXP,DHS_IAMPAN));
   if ( (stat=dcaSendfpConfig(socket,DHS_IAMPAN,(char *)cfg,sizeof(*cfg))) == DCA_ERR ) {
    *istat = DHS_ERROR;
    (void) sprintf(resp,"failed to send fpConfig to collector");
    MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
    return;
   }
   /* send message to collector with expID and obsSetID */
   stat = dcaSendMsg(socket,dcaFmtMsg(DCA_EXP_OBSID,(int)NULL));
  } else {
   if ( (stat=dhsConnectCollector(istat,resp2,DHS_IAMPAN)) == DHS_ERROR ) {
    MNSN_RPT_FAILURE(resp2,"dhsOpenExp: Failed. \\\\");
    *istat = DHS_ERROR;
    (void) sprintf(resp,"failed to validate channel to collector");
    MNSN_RPT_FAILURE(resp,"dhsOpenExp: Failed. \\\\");
    return;
   } else {
    goto retry;  /* NASTY!! */
   }
  }

  /* send message to collector with DIR and FILENAME */
  szt = 256;
  stat = dcaSendMsg(socket,dcaFmtMsg(DCA_DIRECTORY,(int)NULL));
  stat = dcaSend(socket,dhsUtilDIR,szt);
  szt = 256;
  stat = dcaSendMsg(socket,dcaFmtMsg(DCA_FNAME,(int)NULL));
  stat = dcaSend(socket,dhsUtilFILENAME,szt);
  (void) sprintf(line,"dhsOpenExp: ending istat=%ld",*istat);
  MNSN_REPORT_MSG(resp,line);

  /* return */
  *istat = DHS_OK;
  MNSN_RET_SUCCESS(resp,"dhsOpenExp: Success.");
  return;
}
