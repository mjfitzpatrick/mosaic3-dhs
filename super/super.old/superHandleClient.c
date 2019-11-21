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

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */

#define NO  0

struct DCADev {
    char *node;
    int   port;
    int   inUse;
} defDCA[] = {
     {"tucana.tuc.noao.edu", 2001, NO}, /* Collector */ 
     {"elqui.tuc.noao.edu", 2005, NO}    /* Supervisor */
     };


#define SZMSG    120
#define RCVBUFSIZE 1024   /* Size of receive buffer */

void superHandleClient(int socket)
{
    char buffer[RCVBUFSIZE];      /* Buffer for echo string */
    int recvMsgSize;              /* Size of received message */
    int curlen = RCVBUFSIZE;
    int tot=0;
    char *dev=NULL;
    struct msgHeader *msgh;       /* msgHeader msgh ; */
    int mtype, size, msize;
    fpConfig_t fpCfg;


   /* Receives message code */
        /*if ((recvMsgSize = read (socket, &buffer[0], SZMSG)) < SZMSG) { */
        if (dcaRecv (socket, &buffer[0], SZMSG) != SZMSG) {
	   printf ("superHandleClient: Error receiving message\n");
	   return;
        }

msize = SZMSG;
while(msize > 0) {
  
    msgh = (struct msgHeader  *)buffer;

    mtype = msgh->type;
printf("mtype: %d, DCA_SET: %d, DCA_NOOP: %d\n",mtype,DCA_SET,DCA_NOOP);
    if (mtype & DCA_SET) {
        printf("DCA_SET received\n");
        if (mtype & DCA_FP_CONFIG) {
           size = sizeof(fpCfg);
           if (dcaRecv (socket, (char *)&fpCfg, size) != size) {
	       printf ("DCA_FP_CONFIG error \n");
           }
           printf("DCA_FP_CONFIG received: %d bytes \n",recvMsgSize);
	   printf("fpCfg.(x,y)Start: (%d,%d)\n",fpCfg.xStart,fpCfg.yStart);
	   printf("fpCfg.(x,y)Size: (%d,%d)\n",fpCfg.xSize,fpCfg.ySize);
        }  
    } else if (mtype & DCA_NOOP) {

        printf("DCA_NOOP \n");
        /* send ACK only */
        msgh->type = DCA_ACK;
        msgh->whoami = DCA_SUPERVISOR;
        if (dcaSend (socket, (char *)msgh, SZMSG) != SZMSG) {
            printf("DCA_NOOP error\n");
        }

    } else if (DCA_DCADEV & mtype) {

        sprintf ((dev= (char *)malloc(SZMSG)), "%s:%d", defDCA[0].node, defDCA[0].port);
printf("DCADEV sending: %s, on socket: %d\n",dev,socket);
        if (dcaSend (socket, dev, SZMSG) != SZMSG) {
            printf("DCADEV sending error\n");
        }
        defDCA[0].inUse = YES;

    } else {

printf("handling default\n");
        msgh->type = DCA_FAIL;
        msgh->whoami = DCA_SUPERVISOR;

    }

    /* recvMsgSize = read (socket, &buffer[0], SZMSG); */
    msize = dcaRecv (socket, &buffer[0], SZMSG); 
}
    /* Close client socket */
    close(socket);

}
