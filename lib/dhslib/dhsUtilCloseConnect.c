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
void dhsUtilCloseConnect ( XLONG *istat,    /* inherited status                */
			   char *resp,      /* response message                */
			   dhsHandle dhsID  /* dhs handle                      */
    ) 
{
    int stat;
    int socket;

    /* check the inherited status */
    if ( STATUS_BAD(*istat) ) return;
    MNSN_CLEAR_STR(resp,'\0');
    
    if (dcaSimulator())
       return;

    /* check input values */
    if ( dhsID < (dhsHandle)0 ) {
	*istat = DHS_ERROR;
	MNSN_RET_FAILURE(resp,"dhsCloseConnect: BadParam: dhsHandle invalid");
	return;
    }


    /* Send message to server to close smc SHM */
    socket = dhsNW.collector->fd;
    stat = dcaSendMsg (socket, dcaFmtMsg (DCA_CLOSE|DCA_CONNECT, (int )NULL));

    socket = dhsNW.super->fd;
    stat = dcaSendMsg (socket, dcaFmtMsg (DCA_CLOSE|DCA_CONNECT, (int )NULL));

    /* Close the File Descriptors. */
    close (dhsNW.collector->fd);
    close (dhsNW.super->fd);

    /* for this library, this is a no-op */
    *istat = DHS_OK;
    MNSN_RET_SUCCESS(resp,"dhsCloseConnect:  Success.");

    /* return */
    return;
}
