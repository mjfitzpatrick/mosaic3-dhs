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
 * dhsUtilSysOpen ( ... )
 *******************************************************************************/ 
void dhsUtilSysOpen (
  long *istat,     /* inherited status                                        */
  char *resp,       /* response message                                        */
  dhsHandle *dhsID, /* returned handle                                         */
  long whoami      /* identifier for top-level system                         */
) {

  /* declare some variables and initialize them */
  char *pc = (char *)NULL;
  char *p  = (char *)NULL;
  int port = 0;
  int stat = 0;
  *dhsID = 0;

  /* check the inherited status */
  if ( STATUS_BAD(*istat) ) return;
  MNSN_CLEAR_STR(resp,'\0');
    
  /* if simulating, return */
  if ( dcaSimulator() ) return;

  /* do something for _netw variant */
  if ( (p=getenv("MONSOON_DHS")) == (char *)NULL ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"MONSOON_DHS not set");
   MNSN_RPT_FAILURE(resp,"dhsUtilSysOpen: Failed. \\\\");
   return;
  } 
  if ( (pc=index(p,':')) == (char *)NULL ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"MONSOON_DHS not machine:port");
   MNSN_RPT_FAILURE(resp,"dhsUtilSysOpen: Failed. \\\\");
   return;
  } 

  /* initialize parameters */
  (void) memset((void *)dhs.dhsSuperNode,'\0',SZNODE);
  (void) strncpy(dhs.dhsSuperNode, p, pc-p);
  port = atoi(pc+1);
  dhs.dhsSuperPort = port;
  (void) memset((void *)dhs.dhsName,'\0',SZNODE);
  (void) sprintf(dhs.dhsName,"PAN");
  (void) memset((void *)dhs.dhsCollectorNode,'\0',SZNODE);
  dhs.dhsCollectorPort = -1;

  /* establish the connection to supervisor */
  if (dhsNW.super != NULL) {
   if ( (stat=dcaValidateChannel(dhsNW.super)) == DCA_ERR ) {
    (void) sprintf(resp,"failed to validate channel to supervisor");
    MNSN_RPT_FAILURE(resp,"dhsUtilSysOpen: Failed. \\\\");
    if ( (stat=dcaInitChannel(&dhsNW,DCA_SUPERVISOR)) == DCA_ERR ) {
     *istat = DHS_ERROR;
     (void) sprintf(resp,"failed to initialize channel to supervisor");
     MNSN_RPT_FAILURE(resp,"dhsUtilSysOpen: Failed. \\\\");
     return;
    }
   }
  } else if ( (stat=dcaInitChannel(&dhsNW,DCA_SUPERVISOR)) == DCA_ERR ) {
   *istat = DHS_ERROR;
   (void) sprintf(resp,"failed to connect to supervisor");
   MNSN_RPT_FAILURE(resp,"dhsUtilSysOpen: Failed. \\\\");
   return;
  }

  /* return */ 
  *istat = DHS_OK;
  MNSN_RET_SUCCESS(resp,"dhsUtilSysOpen: Success. \\\\");
  return;
}
