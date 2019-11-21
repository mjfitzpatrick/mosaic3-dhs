/*******************************************************************************
 * include(s):
 *******************************************************************************/
#if !defined(_sockUtil_H_)
#include "sockUtil.h"
#endif
#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

#include "dcaDhs.h"

void dhsUtilSysOpen ( XLONG *istat,     /* inherited status                   */
                      char *resp,       /* response message                   */
                      dhsHandle *dhsID, /* returned handle                    */
                      XLONG whoami      /* identifier for top-level system    */
    )
{

    char *p, *pc;
    int  port;

    *dhsID = 0;
    /* check the inherited status */
    if ( STATUS_BAD(*istat) ) return;
        MNSN_CLEAR_STR(resp,'\0');

    if (dcaSimulator())
       return;


    /* Read environment variable with DHS Supervisor machine name 
     * MONSOON_DHS syntax is "machineName:port".
     */
    p = (char *)getenv ("MONSOON_DHS");
    if (p == NULL) {
        MNSN_RET_FAILURE(resp,
	  "dhsSysOpen: 'MONSOON_DHS' environment var not set.");
        *istat = DHS_ERROR ;
        return;
    } 

    pc=index(p,':');
    if (pc == NULL) {
        MNSN_RET_FAILURE(resp,
	 "dhsSysOpen: 'MONSOON_DHS' environment var needs to be: machine:port");
        *istat = DHS_ERROR ;
        return;
    } 
   (void) memset(dhs.dhsSuperNode,'\0', SZNODE);
    strncpy(dhs.dhsSuperNode, p, pc-p);

    port = atoi(pc+1);
    dhs.dhsSuperPort = port;

   (void) memset(dhs.dhsName,'\0', SZNODE);
   sprintf (dhs.dhsName, "PAN");

					/* Initialize the collector params */
   (void) memset(dhs.dhsCollectorNode,'\0', SZNODE);
    dhs.dhsCollectorPort = -1;


    /* Establish the connection to supervisor.
     */
    if (dhsNW.super != NULL) {
        if (dcaValidateChannel (dhsNW.super) == DCA_ERR) {
            if (dcaInitChannel (&dhsNW, DCA_SUPERVISOR) == DCA_ERR) {
                MNSN_RET_FAILURE(resp,
                  "dhsOpenConnect: Error connecting to Supervisor machine"); 
                *istat = DHS_ERROR ;
                return;
            }
        }
    } else if (dcaInitChannel (&dhsNW, DCA_SUPERVISOR) == DCA_ERR) {
        printf("dhsSysOpen: Error connecting to Supervisor machine\n"); 
        *istat = DHS_ERROR ;
        return;
    }


    /*
        ...format a SYS_OPEN message
        ...send it to the supervisor
        ...request state parameters from supervisor
        ...read result and update structure
    */


    /*  Standard return.
    */
    *istat =  DHS_OK;
    return;


}
