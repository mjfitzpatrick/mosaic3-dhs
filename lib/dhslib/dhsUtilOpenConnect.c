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

#define MAXTRYS  10


int dhsConnectCollector (XLONG *istat, char *resp, XLONG whoAmI);



void dhsUtilOpenConnect (
       XLONG *istat,         			/* inherited status           */
       char *resp,          			/* response buffer            */
       dhsHandle *dhsID,    			/* socket file descriptor     */
       XLONG whoAmI,        			/* Identifier                 */
       fpConfig_t *pixConfig        		/* fpConfig                   */
    )
{ 
     int  stat, socket_super;


     *istat = DHS_OK;		/* initialize status		*/
     *dhsID = 0;    		/* Reset for next iteration 	*/

     DPRINT(30, procDebug, "**** dhsOpenConnect\n");

     if (dcaSimulator())
	return;

     /* Validate the connection to supervisor.
     */
     if (dhsNW.super != NULL) {
        DPRINT(30, procDebug, "**** dhsOpenConnect validating super chan\n");
        if (dcaValidateChannel (dhsNW.super) == DCA_ERR) {
            if (dcaInitChannel (&dhsNW, DCA_SUPERVISOR) == DCA_ERR) {
                MNSN_RET_FAILURE(resp,
	          "dhsOpenConnect: Error connecting to Supervisor machine"); 
	        *istat = DHS_ERROR ;
                return;
            }
        }

     } else if (dcaInitChannel (&dhsNW, DCA_SUPERVISOR) == DCA_ERR) {
        printf("dhsOpenConnect: Error connecting to Supervisor machine\n"); 
	*istat = DHS_ERROR ;
        return;
     }


      DPRINTF(30, procDebug, "**** dhsOpenConnect super:0x%x\n",
	(int *)dhsNW.super);
      DPRINTF(30, procDebug, "**** dhsOpenConnect super->fd:%d\n",
	(int)dhsNW.super->fd);
      DPRINTF(30, procDebug, "**** dhsOpenConnect fpConfig size:%d\n",
	(int)sizeof (*pixConfig));

      socket_super = (int )dhsNW.super->fd;

      /* Send a SET_FOCAL_PLANE message to the super.
      */
      DPRINT(30, procDebug, "**** dhsOpenConnect sending fpCfg\n");
      if (dcaSendfpConfig (socket_super, whoAmI, 
          (char *)pixConfig, sizeof(*pixConfig)) == DCA_ERR) { 
              MNSN_RET_FAILURE(resp, 
	          "dhsOpenConnect: Cannot send fpConfig to Super");
	      *istat = DHS_ERROR ;
	      return;
      }
      DPRINT(30, procDebug, "**** dhsOpenConnect done sending fpCfg\n");


      /* Send the openConnect to the Supervisor.
      */
      stat = dcaSendMsg(socket_super, 
	  dcaFmtMsg(DCA_OPEN|DCA_CONNECT,(int)NULL));

      /* Get a valid collector connection.
      */
      dhsConnectCollector (istat, resp, whoAmI);

      /* See if we already have a connection made by dhsOpenSys
      ** via dcaInitChannel.
      */
      if (dhsNW.collector != (struct dhsChan *) NULL) {
         return;
      } else {
         MNSN_RET_FAILURE(resp, "dhsOpenConnect: Cannot connect to Collector");
         *istat = DHS_ERROR ;
	 return;
      }
}

int 
dhsConnectCollector (
    XLONG *istat,         			/* inherited status           */
    char *resp,          			/* response buffer            */
    XLONG whoAmI        			/* Identifier                 */
)
{
     int  i;


      /* Loop MAXTRY times until we get a valid collector connection.
      */
      for (i = 0; i < MAXTRYS; i++) {
          DPRINT(30, procDebug, 
                "**** dhsOpenConnect init new collector channel..\n");
          if (dcaInitChannel (&dhsNW, DCA_COLLECTOR) == DCA_OK) {
              /* Send an INIT to the collector.
              */
              DPRINTF(30, procDebug, "**** dhsOpenConnect: coll node='%s'\n",
                 dhsNW.collector->node);
              DPRINTF(30, procDebug, "**** dhsOpenConnect: coll port=%d\n",
                 dhsNW.collector->port);
              DPRINTF(30, procDebug, "**** dhsOpenConnect: whoami=%d\n",whoAmI);
              DPRINT(30, procDebug, "**** dhsOpenConnect sending init...\n");
              dcaSendMsg ((int)dhsNW.collector->fd, dcaFmtMsg(DCA_INIT,whoAmI));

              DPRINTF(30, procDebug, 
                  "**** dhsOpenConnect at dcaInit super fd:%d\n",
                  (int)dhsNW.super->fd);
              DPRINTF(30, procDebug, 
                  "**** dhsOpenConnect at dcaInit collector fd:%d\n",
                  (int)dhsNW.collector->fd);

	      return DHS_OK;

          } else {
              /* Can't get a Collector connection, notify the Super.
              */
              DPRINT(30, procDebug, "**** dhsOpenConnect getCollector FAILS\n");
              dcaSendMsg ((int )dhsNW.super->fd, 
                  dcaFmtMsg (DCA_FAIL|DCA_CONNECT, whoAmI));

	      return DHS_ERROR;
          }
      }

      return DHS_OK;
}
