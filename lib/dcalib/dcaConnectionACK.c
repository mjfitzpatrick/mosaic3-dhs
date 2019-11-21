#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif


#include "dcaDhs.h"

/* Simply check whether the given channel is alive.  Sending a NO-OP will
 * either get an ACK and we return OK, or fail and we return ERR.
 * int dcaValidateChannel (dhsChan *chan)
*/

int dcaConnectionACK(int socket)
{
       char resp[SZMSG];

       dcaSendMsg (socket, dcaFmtMsg (DCA_NOOP, (int )NULL));
       if (dcaRecvMsg (socket, resp, SZMSG) > 0) {
          return DCA_ALIVE;
       } else
          return DCA_ERR;
}

