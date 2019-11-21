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

extern int dhsConnectCollector (XLONG *istat, char *resp, XLONG whoAmI);



void dhsUtilOpenExp ( XLONG *istat,    		/* inherited status           */
                      char *resp,      		/* response message           */
                      dhsHandle dhsID, 		/* dhs handle                 */
                      fpConfig_t *fpCfg, 	/* focal plane configuration  */
                      double *expID,   		/* exposure identifier        */
                      char *obsetID    		/* observation set identifier */
    )

{
        int socket, stat;
	int szt;


	/* check the inherited status */
	if ( STATUS_BAD(*istat) ) return;
	    MNSN_CLEAR_STR(resp,'\0');

        if (dcaSimulator())
            return;

	/* check input parameters */
	if ( fpCfg==(fpConfig_t *)NULL || expID==(double *)NULL ||
	     obsetID==(char *)NULL )
	{
	    *istat = DHS_ERROR;
	    sprintf(resp,"dhsOpenExp:  bad parameter, %s ",
                    ((fpCfg==(fpConfig_t*)NULL) ? "FP fpCfg pointer==NULL":
                    ((expID == (double *)NULL)?"expID==NULL":"obsetID==NULL")));
	    MNSN_RET_FAILURE(resp,"");
	    return;
	}

        /* check input values */
        if ( dhsID < (dhsHandle )0 )
        {
            *istat = DHS_ERROR;
            MNSN_RET_FAILURE(resp,
               "dhsOpenExp: Bad parameter, dhsHandle invalid.");
            return;
        }
        /* END CHECKING INPUTS */

        DPRINTF(30, procDebug,
	    "**** dhsUtilOpenExp: dhsNW.super->fd: %d\n",dhsNW.super->fd);
        DPRINTF(30, procDebug,
	    "**** dhsUtilOpenExp: dhsNW.collector->fd: %d\n",
	    dhsNW.collector->fd);

    	/* Validate the connection to supervisor */
    	if (dcaValidateChannel (dhsNW.super) == DCA_ERR) {

	    /* Try to reconnect to the supervisor before failing.
	    */
            if (dcaInitChannel (&dhsNW, DCA_SUPERVISOR) == DCA_ERR ||
    	        dcaValidateChannel (dhsNW.super) == DCA_ERR) {

                *istat = DHS_ERROR;
                MNSN_RET_FAILURE(resp,
                    "dhsOpenExp: Cannot validate connection to Supervisor");
	        return;
            }
	}
        DPRINTF(30, procDebug,
	     "**** dhsUtilOpenExp: valid connection on %s.\n", "super");

        dhs.expID = *expID;    	/*dcaFmtMsg will read these 2 values */
	dhs.obsSetID = obsetID;
        DPRINT(30, procDebug,
	     "**** dhsUtilOpenExp: validating connection to collector...\n");

    	if (dcaValidateChannel (dhsNW.collector) != DCA_ERR) {
retry:
    	    socket = (int )dhsNW.collector->fd;

            DPRINTF(30, procDebug,
	         "**** dhsUtilOpenExp: sending fpConfig to fd=%d.\n", socket);

	    /* Send an OpenExposure to the Collector and Supervisor.
	     */
	    stat = dcaSendMsg (dhsNW.collector->fd, dcaFmtMsg (DCA_OPEN|DCA_EXP,
		DHS_IAMPAN));
	    stat = dcaSendMsg (dhsNW.super->fd, dcaFmtMsg (DCA_OPEN|DCA_EXP,
		DHS_IAMPAN));

            if (dcaSendfpConfig 
                (socket, DHS_IAMPAN, (char *)fpCfg,sizeof(*fpCfg)) == DCA_ERR) {
                MNSN_RET_FAILURE(resp,
                    "dhsOpenExp: Cannot send fpConfig to Collector");
                *istat = DHS_ERROR ;
                 return;
            }
	    /* Send message to collector with expID and obsSetID */
	    stat = dcaSendMsg (socket, dcaFmtMsg (DCA_EXP_OBSID, (int )NULL));

        } else {
	    if (dhsConnectCollector (istat, resp, DHS_IAMPAN) == DHS_ERROR) {
                *istat = DHS_ERROR;
                MNSN_RET_FAILURE(resp,
                   "dhsOpenExp: Cannot validate connection to Collector");
                return;
	    } else
		goto retry;
         }

	 /* Send message to collector with DIR and FILENAME */
	 szt = 256;
	 stat = dcaSendMsg (socket, dcaFmtMsg (DCA_DIRECTORY, (int )NULL));
	 stat = dcaSend(socket, dhsUtilDIR, szt);

	 szt = 256;
	 stat = dcaSendMsg (socket, dcaFmtMsg (DCA_FNAME, (int )NULL));
	 stat = dcaSend(socket, dhsUtilFILENAME, szt);

         DPRINTF(30, procDebug, "**** dhsUtilOpenExp: END: istat:%d\n",*istat);
         *istat =  DHS_OK;
	 return;
}
