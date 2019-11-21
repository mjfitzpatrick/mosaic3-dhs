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
#define MAXTRYS 10

/*******************************************************************************
 * dhsConnectCollector ( ... )
 *******************************************************************************/ 
int dhsConnectCollector (
  long *istat, /* inherited status                                            */
  char *resp,  /* response buffer                                             */
  long whoami  /* Identifier                                                  */
) {

  /* declare some variables and initialize them */
  int ic = 0;
  int sc = 0;
  MNSN_CLEAR_STR(resp,'\0');

  /* loop until we get a valid collector connection */
  for ( ic=0; ic<MAXTRYS; ic++ ) {
   if ( (sc=dcaInitChannel(&dhsNW,DCA_COLLECTOR)) == DCA_OK ) {
       (void) dcaSendMsg((int)dhsNW.collector->fd,dcaFmtMsg(DCA_INIT, (XLONG)whoami));
    (void) sprintf(resp,"connected to collector, super->fd=%d collector->fd=%d",(int)dhsNW.super->fd,(int)dhsNW.collector->fd);
    MNSN_RPT_SUCCESS(resp,"dhsConnectCollector: Success. \\\\");
    return DHS_OK;
   }
  }

  /* can't get a collector connection, notify the super */
  (void) sprintf(resp,"failed to connect to collector, status=%d",sc);
  MNSN_RPT_FAILURE(resp,"dhsConnectCollector: Failed. \\\\");
  (void) dcaSendMsg((int)dhsNW.super->fd,dcaFmtMsg(DCA_FAIL|DCA_CONNECT,whoami));
  return DHS_ERROR;
}

/*******************************************************************************
 * dhsUtilOpenConnect ( ... )
 *******************************************************************************/ 
void dhsUtilOpenConnect (
  long *istat,     /* inherited status                                        */
  char *resp,       /* response message                                        */
  dhsHandle *dhsID, /* returned handle                                         */
  long whoami,     /* identifier for mid-level system                         */
  fpConfig_t *cfg   /* focal plane configuration                               */
) {

  /* declare some variables and initialize them */
  int stat = 0;
  int socket_super = 0;
  char line[MAXMSG];
  (void) memset((void *)line,'\0',MAXMSG);

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');

  /* reset value(s) */
  *istat = DHS_OK;
  *dhsID = 0;

  /* if simulating, return */
  if ( dcaSimulator() ) return;
    
  /* check input parameter(s) */
  /* COMMENT OUT AS WE DON'T NEED THESE CHECKS!
    if ( dhsID == (dhsHandle *)NULL ) {
     *istat = DHS_ERROR;
     (void) sprintf(resp,"bad parameter dhsID==NULL");
     MNSN_RPT_FAILURE(resp,"dhsOpenConnect: Failed. \\\\");
     return;
    }
  */
  /* COMMENT OUT AS WE DON'T NEED THESE CHECKS!
    if ( whoami==DHS_IAMOCS || whoami==DHS_IAMMSL ) {
     *istat = DHS_ERROR;
     (void) sprintf(resp,"bad parameter whoami=%ld",(long)whoami);
     MNSN_RPT_FAILURE(resp,"dhsOpenConnect: Failed. \\\\");
     return;
    }
  */

  /* do something for _netw variant */
  (void) sprintf(line,"dhsOpenConnect: validating supervisor channel");
  MNSN_REPORT_MSG(resp,line);
  if ( dhsNW.super != NULL ) {
   if ( (stat=dcaValidateChannel(dhsNW.super)) == DCA_ERR ) {
    (void) sprintf(resp,"error validating channel to supervisor, status=%d",stat); 
    MNSN_RPT_FAILURE(resp,"dhsOpenConnect: Failed. \\\\");
    if ( (stat=dcaInitChannel(&dhsNW, DCA_SUPERVISOR)) == DCA_ERR ) {
     *istat = DHS_ERROR;
     (void) sprintf(resp,"error initializing channel to supervisor, status=%d",stat); 
     MNSN_RPT_FAILURE(resp,"dhsOpenConnect: Failed. \\\\");
     return;
    }
   }
  } else if ( (stat=dcaInitChannel(&dhsNW,DCA_SUPERVISOR)) == DCA_ERR ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"error connecting to supervisor, status=%d",stat); 
   MNSN_RPT_FAILURE(resp,"dhsOpenConnect: Failed. \\\\");
   return;
  }
  (void) sprintf(line,"dhsOpenConnect: super=%p",(int *)dhsNW.super);
  MNSN_REPORT_MSG(resp,line);
  (void) sprintf(line,"dhsOpenConnect: super->fd=%d",(int)dhsNW.super->fd);
  MNSN_REPORT_MSG(resp,line);
  (void) sprintf(line,"dhsOpenConnect: fpConfig size=%d",(int)sizeof(*cfg));
  MNSN_REPORT_MSG(resp,line);

  /* send fpConfig to the supervisor */
  socket_super = (int)dhsNW.super->fd;
  (void) sprintf(line,"dhsOpenConnect: sending fpConfig");
  MNSN_REPORT_MSG(resp,line);
  if ( (stat=dcaSendfpConfig(socket_super,whoami,(char *)cfg,sizeof(*cfg))) == DCA_ERR ) { 
   *istat = DHS_ERROR;
   (void) sprintf(resp,"error sending fpConfig to supervisor, status=%d",stat); 
   MNSN_RPT_FAILURE(resp,"dhsOpenConnect: Failed. \\\\");
   return;
  }
  (void) sprintf(line,"dhsOpenConnect: sent fpConfig");
  MNSN_REPORT_MSG(resp,line);

  /* send the openConnect to the supervisor */
  stat = dcaSendMsg(socket_super,dcaFmtMsg(DCA_OPEN|DCA_CONNECT,(int)NULL));

  /* get a valid collector connection */
  (void) dhsConnectCollector(istat,resp2,whoami);
  if ( STATUS_BAD(*istat) ) MNSN_RPT_FAILURE(resp2,"dhsOpenConnect: Failed. \\\\");

  /* see if we already have a connection made by dhsOpenSys via dcaInitChannel */
  if ( dhsNW.collector != (struct dhsChan *)NULL ) {
   *istat = DHS_OK;
   MNSN_RET_SUCCESS(resp,"dhsOpenConnect: Success.");
  } else {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"cannot connect to collector");
   MNSN_RET_FAILURE(resp,"dhsOpenConnect: Failed. \\\\");
  }

  /* return */ 
  return;
}
