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

/*******************************************************************************
 * dhsUtilSendPixelData ( ... )
 *******************************************************************************/ 
void dhsUtilSendPixelData (
  long *istat,    /* inherited status                                         */
  char *resp,      /* response message                                         */
  dhsHandle dhsID, /* dhs handle                                               */
  void *pxlAddr,   /* address of data block                                    */
  size_t pxlSize,  /* size of data block                                       */
  fpConfig_t *cfg, /* configuration of pixel data                              */
  double *expID,   /* exposure identifier                                      */
  char *obsetID    /* observation set identifier                               */
) {

  /* declare some variables and initialize them */
  int sizefp = DHS_OK;
  int socket = DHS_OK;
  int stat = DHS_OK;
  char line[MAXMSG];

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');
    
  /* if simulating, return */
  if ( dcaSimulator() ) return;

  /* skip if size is zero */
  if ( pxlSize == (size_t)0 ) return;

  /* do something for _netw variant */
  socket = dhsNW.collector->fd;
  (void) dcaSendMsg(socket,dcaFmtMsg(DCA_PIXEL,DHS_IAMWHO));
    
  /* send cfg data to the server */
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendPixelData: send xstart=%d, ystart=%d, xsize=%d, ysize=%d, stat=%d, istat=%ld",
   (int)cfg->xStart,(int)cfg->yStart,(int)cfg->xSize,(int)cfg->ySize,(int)stat,(long)*istat);
  MNSN_REPORT_MSG(resp,line);
  sizefp = (int)sizeof(*cfg);
  if ( (stat=dcaSend(socket,(char *)cfg,sizefp)) == DCA_ERR ) *istat = DHS_ERROR;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendPixelData: sent xstart=%d, ystart=%d, xsize=%d, ysize=%d, stat=%d, istat=%ld",
   (int)cfg->xStart,(int)cfg->yStart,(int)cfg->xSize,(int)cfg->ySize,(int)stat,(long)*istat);
  MNSN_REPORT_MSG(resp,line);

  /* send PAN data to the server */
  stat = DHS_OK;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendPixelData: send %d bytes, stat=%d, istat=%ld",(int)pxlSize,(int)stat,(long)*istat);
  MNSN_REPORT_MSG(resp,line);
  if ( (stat=dcaSend(socket,pxlAddr,pxlSize)) == DCA_ERR ) *istat = DHS_ERROR;
  (void) memset((void *)line,'\0',MAXMSG);
  (void) sprintf(line,"dhsSendPixelData: sent %d bytes, stat=%d, istat=%ld",(int)pxlSize,(int)stat,(long)*istat);
  MNSN_REPORT_MSG(resp,line);

  /* return */
  if ( STATUS_BAD(*istat) ) {
   MNSN_RET_FAILURE(resp,"dhsSendPixelData: Failed. \\\\");
  } else {
   MNSN_RET_SUCCESS(resp,"dhsSendPixelData: Success.");
  }
  return;
}
