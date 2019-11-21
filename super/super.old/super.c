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

/*****************************
     SUpervisor PROGRAM.  Listen for connection from a client (elqui)

     Uses port 2005.
*/
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define MAXPENDING 5    /* Maximum outstanding connection requests */

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleClient(int clntSocket);
int CreateTCPServerSocket(unsigned short port);

int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in ClntAddr;    /* Client address */
    int ServPort;
    unsigned int clntLen;          /* Length of client address data structure */
    int size;
    long status=0L;
    char key[20]={"keysm1"};
    int  create = TRUE;


    ServPort = (unsigned short)2005;
    servSock = CreateTCPServerSocket (ServPort);

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(ClntAddr);

        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, 
                               &clntLen)) < 0)
            DieWithError("accept() failed");

        /* clntSock is connected to a client! */

        printf("Handling client %s\n", inet_ntoa(ClntAddr.sin_addr));

        superHandleClient (clntSock);
        printf("AFTER Handling client %s\n", inet_ntoa(ClntAddr.sin_addr));
    }
    /* NOT REACHED */
}
