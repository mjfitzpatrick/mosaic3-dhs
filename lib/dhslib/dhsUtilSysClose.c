/*******************************************************************************
 * include(s):
 *******************************************************************************/
#if !defined(_dhsUtil_H_)
 #include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
 #include "dhsImplementationSpecifics.h"
#endif 

/*******************************************************************************
 * dhsUtilSysClose ( ... )
 *******************************************************************************/ 
void dhsUtilSysClose ( XLONG *istat,    /* inherited status                    */
		       char *resp,      /* response message                    */
		       dhsHandle dhsID  /* dhs handle                          */
    ) 
{
    /* check the inherited status */
    if ( STATUS_BAD(*istat) ) return;
    MNSN_CLEAR_STR(resp,'\0');
    
    if (dcaSimulator())
       return;

    /* check input values */
    if ( dhsID < (dhsHandle)0 ) 
    {
	*istat = DHS_ERROR;
	MNSN_RET_FAILURE(resp,"dhsSysClose:  bad parameter, dhsHandle invalid");
	return;
    }

    /* for this library, this is a no-op */
    *istat = DHS_OK;
    MNSN_RET_SUCCESS(resp,"dhsSysClose:  Success.");
    /* return */
    return;
}
