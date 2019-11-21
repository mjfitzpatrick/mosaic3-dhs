#if !defined(_sockUtil_H_)
#include "sockUtil.h"
#endif
#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>      	/* inet_ntoa() to format IP address 	*/
#include <netinet/in.h>     	/* in_addr structure 			*/
#include <netdb.h>          	/* hostent struct, gethostbyname() 	*/

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

void dcaSockConnect(int *sock, int port, char *ssadr);

#define MAXPENDING 5    /* Maximum outstanding connection requests */



void
dcaOpenChannel (struct dhsChan *chan, int *s)

{
    int     sock;                     		/* socket to create 	*/


    DPRINTF (30, dhsDebug, "Open Channel: port=%d\n", chan->port);

    if (dcaUseSim()) {
	DPRINTF (30, dhsDebug, "            : node='%s(SIM)'\n", chan->node);
        (void )dcaSockConnect (&sock, chan->port, "localhost");

    } else {
	DPRINTF (30, dhsDebug, "            : node='%s'\n", chan->node);
        (void )dcaSockConnect (&sock, chan->port, chan->node);
    }

    *s = chan->fd = sock;
}


void
dcaSockConnect(int *sock, int port, char *node)
{
    /* declare some variables and initialize them */
    int istat;
    char resp[160];
    
    struct hostent *hp;      			/* host information 	*/
    struct in_addr inAddress;
    struct sockaddr_in sa;


    (void) memset (&inAddress, 0, sizeof(struct in_addr));
    (void) memset (&sa, 0, sizeof(struct sockaddr_in));

    if (isalpha(node[0])) {
	/* Using a node name.
	*/

        /* set up connection data */
        hp = gethostbyname (node);				    /* MJF */
        inAddress.s_addr = *((unsigned long *) hp->h_addr_list[0]); /* MJF */

        sa.sin_family      = AF_INET;
        sa.sin_port        = htons(port);
        sa.sin_addr.s_addr = inAddress.s_addr;

    } else {
	/* Using an IP address.
	*/
        sa.sin_family      = AF_INET;
        sa.sin_port        = htons(port);
        sa.sin_addr.s_addr = inet_addr (node);
    }


     /* open socket */
    if ( (*sock = socket (AF_INET, SOCK_STREAM, 0)) == ERROR ) {
        sprintf(resp,"dcaSockConnect: socket failed. \\\\%s",
	    strerror(errno));
        perror(resp);
        return;
    }

    if ( (istat = connect (*sock, (struct sockaddr *)&sa,
	sizeof(struct sockaddr_in))) == ERROR ) {
            (void) close(*sock);
            sprintf(resp,"dcaSockConnect: connect failed. \\\\%s",
	        strerror(errno)); 
            perror(resp);
    }

    return;
}
