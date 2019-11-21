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

void dhsSendMetaData ( 
      XLONG *istat,        /* inherited status 		 */
      char *resp,          /* response message           */
      dhsHandle dhsID,     /* dhs handle                 */
      void *blkAddr,       /* address of data block      */
      size_t blkSize,      /* size of data block (bytes) */
      mdConfig_t *mdCfg,   /* configuration of meta data */
      double *expID,       /* exposure identifier        */
      char *obsetID        /* observation set identifier */
)
{
	int socket, szt, stat;
	char buffer[120];


        if (dcaSimulator())
            return;

        *istat = DHS_OK;
	socket = dhsNW.collector->fd;


        /* Skip if blksize is zero */
	if (blkSize == 0)
            return;

        DPRINTF(30, procDebug,
	   "** Starting dhsSendMetaData. obsetID: %s\n",obsetID);

        dcaSendMsg (socket, dcaFmtMsg (DCA_META, (int )NULL));

	/* Send blkSize 1st*/
        szt = sizeof(size_t);
	szt = 120;
	bzero (buffer, szt);
	memmove (buffer, (char *)&blkSize, sizeof(size_t));
	stat = dcaSend(socket, buffer, szt);

	/* Send data block */
	stat = dcaSend (socket, (char *)blkAddr, blkSize);
        DPRINTF(60, procDebug, "Sent MetaData block: %d bytes\n", blkSize);
	if (stat == DCA_ERR)
	   *istat = DHS_ERROR;

	/* Send md block */
        stat =  dcaSend (socket, (char *)mdCfg, sizeof(*mdCfg));
        DPRINTF(60, procDebug, "Sent mdCfg: %d bytes\n", sizeof(*mdCfg));
	if (stat == DCA_ERR)
	   *istat = DHS_ERROR;

	/* Send md block */
        
        DPRINTF(30, procDebug,
	   "** Ending dhsSendMetaData: %s\n",obsetID);        

        return ;
}
