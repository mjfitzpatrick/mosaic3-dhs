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

#define bzero(b,len) ((void)memset((void *)(b),'\0',(len)),(void)0)

/*******************************************************************************
 * dhsUtilSendMetaData ( ... )
 *******************************************************************************/ 
void dhsUtilSendMetaData (
  long *istat,    /* inherited status                                         */
  char *resp,      /* response message                                         */
  dhsHandle dhsID, /* dhs handle                                               */
  void *blkAddr,   /* address of data block                                    */
  size_t blkSize,  /* size of data block                                       */
  mdConfig_t *cfg, /* configuration of meta data                               */
  double *expID,   /* exposure identifier                                      */
  char *obsetID    /* observation set identifier                               */
) {

  /* declare some variables and initialize them */
  int socket = DHS_OK;
  int stat = DHS_OK;
  int szt = 120;
  char buffer[szt];
  char line[MAXMSG];
  (void) memset((void *)line,'\0',MAXMSG);
  (void) memset((void *)buffer,'\0',szt);
  (void) memmove(buffer,(char *)&blkSize,sizeof(size_t));

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');
    
  /* if simulating, return */
  if ( dcaSimulator() ) return;

  /* set value(s) */
  *istat = DHS_OK;
  socket = dhsNW.collector->fd;

  /* skip if blksize is zero */
  if ( blkSize == (size_t)0 ) return;

  /* do something for _netw variant */
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendMetaData: starting \"%s\", expID=%lf, stat=%d, istat=%ld",obsetID,*expID,stat,*istat);
  MNSN_REPORT_MSG(resp,line);
  (void) dcaSendMsg(socket,dcaFmtMsg(DCA_META,(int)NULL));

  /* send blkSize first */
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendMetaData: send %p, size=%d, stat=%d, istat=%ld",blkAddr,(int)szt,stat,*istat);
  MNSN_REPORT_MSG(resp,line);
  if ( (stat=dcaSend(socket,buffer,szt)) == DCA_ERR ) *istat = DHS_ERROR;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendMetaData: sent %p, size=%d, stat=%d, istat=%ld",blkAddr,(int)szt,stat,*istat);
  MNSN_REPORT_MSG(resp,line);

  /* send data block */
  stat = DHS_OK;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendMetaData: send metadata size=%d, stat=%d, istat=%ld",(int)blkSize,stat,*istat);
  MNSN_REPORT_MSG(resp,line);
  if ( (stat=dcaSend(socket,(char *)blkAddr,blkSize)) == DCA_ERR ) *istat = DHS_ERROR;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendMetaData: sent metadata size=%d, stat=%d, istat=%ld",(int)blkSize,stat,*istat);
  MNSN_REPORT_MSG(resp,line);

  /* send md block */
  stat = DHS_OK;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendMetaData: send mdConfig %p, size=%d, stat=%d, istat=%ld",cfg,(int)sizeof(*cfg),stat,*istat);
  MNSN_REPORT_MSG(resp,line);
  if ( (stat=dcaSend(socket,(char *)cfg,sizeof(*cfg))) == DCA_ERR ) *istat = DHS_ERROR;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendMetaData: sent mdConfig %p, size=%d, stat=%d, istat=%ld",cfg,(int)sizeof(*cfg),stat,*istat);
  MNSN_REPORT_MSG(resp,line);

  /* return */
  if ( STATUS_BAD(*istat) ) {
   MNSN_RET_FAILURE(resp,"dhsSendMetaData: Failed. \\\\");
  } else {
   MNSN_RET_SUCCESS(resp,"dhsSendMetaData: Success.");
  }
  return;
}
