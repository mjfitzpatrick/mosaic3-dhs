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
void dhsUtilCloseExp ( XLONG *istat,    /* inherited status                   */
		       char *resp,      /* response message                   */
		       dhsHandle dhsID, /* dhs handle                         */
		       double expID     /* exposure identifier                */
    ) 
{
    /* check the inherited status */
    if ( STATUS_BAD(*istat) ) return;
    MNSN_CLEAR_STR(resp,'\0');
    

    if (dcaSimulator())
       return;

    /* check input values  */
    if ( dhsID<(dhsHandle)0 || expID<=(double)0.0 ) 
    {
	*istat = DHS_ERROR;
	MNSN_RET_FAILURE(resp,"dhsCloseExp:  bad parameter. dhsHandle invalid.");
	return;
    }

    /* Send the CloseExp message to the Collector and Supervisor.
     */
    *istat = dcaSendMsg ( (int )dhsNW.collector->fd,
	dcaFmtMsg (DCA_CLOSE|DCA_EXP, DHS_IAMPAN) );
    *istat = dcaSendMsg ( (int )dhsNW.super->fd,
	dcaFmtMsg (DCA_CLOSE|DCA_EXP, DHS_IAMPAN) );

    /* for this library, this is a no-op */
    *istat = DHS_OK;
    MNSN_RET_SUCCESS(resp,"dhsCloseExp:  Success.");

    /* return */
    return;
}
