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

/**********************************************************************/
/* Send pixel data to the server machine */

void dhsSendPixelData ( 
         XLONG *istat,       /* inherited status            */
         char *resp,         /* response message            */
         dhsHandle dhsID,    /* dhs handle                  */
         void *pxlAddr,      /* address of data block       */
         size_t frmSize,     /* size of data block          */
         fpConfig_t *fpCfg,  /* configuration of pixel data */
         double *expID,      /* exposure identifier         */
         char *obsetID       /* observation set identifier  */
)

{
    int sizefp, socket, stat;
    char line[180];

    DPRINT(30, procDebug, "***STARTING dhsSendPixelData\n");

    if (dcaSimulator())
       return;


    /* Skip if size is zero */
    if (frmSize == 0)  {
        DPRINT(40, procDebug, "Size is ZERO. DONE with dhsSendPixelData\n\n");
        return;
    }

    socket = dhsNW.collector->fd;

    dcaSendMsg (socket, dcaFmtMsg (DCA_PIXEL, DHS_IAMWHO));
    
    /* Send fpCfg data to the server */
    sprintf (line, "xstart: %d, ystart: %d\n", 
        (int )fpCfg->xStart, (int )fpCfg->yStart);
    DPRINT(30, procDebug, line);
    sprintf(line, "xsize: %d, ysize: %d\n", 
        (int )fpCfg->xSize, (int )fpCfg->ySize);
    DPRINT(30, procDebug, line);
	
    sizefp= sizeof(*fpCfg);
    stat = dcaSend (socket, (char *)fpCfg, sizefp);
    if (stat == DCA_ERR)
       *istat = DHS_ERROR;

    /* Send PAN data to the server
    */
    DPRINTF(40, procDebug, "Sending frmSize: %d bytes\n",frmSize);
    stat = dcaSend (socket, pxlAddr, frmSize);
    if (stat == DCA_ERR)
       *istat = DHS_ERROR;

    sprintf(line, "Sent: %d bytes, istat=%d \n",(int)frmSize, (int)*istat);
    DPRINT(40, procDebug, line);
    DPRINT(40, procDebug, "DONE with dhsSendPixelData\n\n");

    return;
}
