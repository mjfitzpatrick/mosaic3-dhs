#include <stdlib.h>

#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

#include "dcaDhs.h"

#define SZ_LINE 	80


/* Return a device string for the DHS Supervisor.  Format of the string is
 *
 *      <node_name> ':' <port_num>
 *
 * If connection defined in environment, use it, otherwise use default to
 * the node/port specified in the Super struct.  Super is hardwired (or at
 * least somehow initialized when the library is initialized but may be
 * changed with an IOCTL later to request a different supervisor.
 */
char *
dcaGetSuperDev(dhsNetw *dhsID)
{
    char *dev = NULL;
    dhsChan Super;

    Super = *dhsID->super;

    /* Initialize to the default value. */
    sprintf ((dev=calloc(1,SZ_LINE)), "%s:%d", Super.node, Super.port);

    /* Only look for the environment value the first time we are called.  */
    if (!Super.initialized++ && (dev = getenv (DHS_SUPERVISOR)) == (char *)NULL)
      ;

    return (dev);  /* Simply return device. <validate the form as well?>  */       
}


/* dcaGetNode -- Pull out the nodename part of a device string.
 */
char *
dcaGetNode (char *device)
{
    char *node, *ip, *op;

    op = node = calloc(1,SZ_LINE);

    ip = device;
    while (*ip && *ip != ':')
	*op++ = *ip++;

    return (node);
} 


/* dcaGetPort -- Pull out the nodename part of a device string.
 */
int
dcaGetPort (char *device)
{
    char *sm;
    int  port;

    sm = strchr (device, ':');
    port = atoi(++sm);
    return (port);
}


/* Simply check whether the given channel is alive.  Sending a NO-OP will
 * either get an ACK and we return OK, or fail and we return ERR.
 */
int dcaValidateChannel (dhsChan *chan)
{
    char resp[SZMSG];

    if (dcaSendMsg (chan->fd, dcaFmtMsg (DCA_NOOP, (int )NULL)) == DCA_ERR) {
	return (DCA_ERR);
    } else {
        return (dcaRecvMsg (chan->fd, resp, SZMSG));
    }
}


/* Request a collector from the supervisor. 
*/
char *
dcaGetDCADev (int socket)
{
     int istat, size;
     char *dev;

     /*  Send message to the Supervisor requesting a Collector
     **  node and port.
     */
     dev= (char *)calloc(1,SZMSG);

     istat = dcaSendMsg(socket,dcaFmtMsg ((XLONG )DCA_GET|DCA_DCADEV,
	(XLONG )NULL));
     size = dcaRecvMsg (socket, dev, SZMSG);

     return (dev);
}
