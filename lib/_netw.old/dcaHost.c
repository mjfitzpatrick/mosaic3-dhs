#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>          /* inet_ntoa() to format IP address */
#include <netinet/in.h>         /* in_addr structure */

/* Function prototypes */
#ifdef __STDC__
#include <stddef.h>
#include <stdlib.h>
#endif


#define SZ_FNAME	64

#define bzero(b,len) ((void)memset((void *)(b),'\0',(len)),(void)0)

typedef struct {
    char actual[SZ_FNAME];              /* Actual host name 		*/
    char simulated[SZ_FNAME];           /* Simulated host name 		*/
    char domain[SZ_FNAME];              /* Domain name 			*/
    char ip_addr[SZ_FNAME];             /* IP addr of host we're using 	*/
    int  use_sim;                       /* Use simulated name?  	*/
} DCAHost, *DCAHostPtr;


void	dcaInitDCAHost(void),  dcaSetDCAHost(void), dcaFreeDCAHost(void);
void	dcaSetSimHost(char *simHost, int flag);
char   *dcaGetDCAHost(void);
int	dcaUseSim(void);
char   *dcaActualHost(void);
char   *dcaSimHost(void);
char   *dcaDomain(void);


DCAHostPtr	dcaHost;



/* Mini-interface for managing the host name of a message sender.  By default
** we'll always use the actual host name, however we can set a simulated name
** and deliver that instead.  Note that we can call the routine to initialize
** and set the simulated host before connecting.
*/

void 
dcaInitDCAHost ()
{
    static int initialized = 0;

    if (initialized)
        return;

    dcaHost = calloc (1, sizeof (DCAHost));

    bzero (dcaHost->actual, SZ_FNAME); 
    bzero (dcaHost->simulated, SZ_FNAME); 
    dcaHost->use_sim = 0;
    initialized++;

    dcaSetDCAHost ();
}


void 
dcaFreeDCAHost ()
{
    free (dcaHost);
}


void
dcaSetDCAHost ()
{
    char  hname[SZ_FNAME], *ip;
    struct hostent *host;               /* host information */
    struct in_addr x_addr;              /* internet address */


    if (!dcaHost)
 	dcaInitDCAHost();

    bzero (hname, SZ_FNAME); 
    gethostname (hname, SZ_FNAME);

    for (ip=hname;  *ip; ip++) {        /* truncate the doman name, if any */
        if (*ip == '.') {
            *ip = '\0';
            break;
        }
    }

    if ((host = gethostbyname(hname)) == (struct hostent *)NULL) {
        printf("dcaSetHost(); nslookup failed on '%s'\n", hname);
        exit(1);
    }
    x_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
    bzero (dcaHost->ip_addr, SZ_FNAME); 
    sprintf (dcaHost->ip_addr, "%s", inet_ntoa(x_addr));

    bzero (dcaHost->actual, SZ_FNAME); 
    strncpy (dcaHost->actual, hname, strlen(hname));
}


void
dcaSetSimHost (char *simhost, int flag)
{
    if (!dcaHost)
 	dcaInitDCAHost();

    bzero (dcaHost->simulated, SZ_FNAME); 
    strncpy (dcaHost->simulated, simhost, strlen(simhost));

    bzero (dcaHost->ip_addr, SZ_FNAME); 
    strcpy (dcaHost->ip_addr, "127.0.0.1");

    dcaHost->use_sim = flag;
}


char *
dcaGetDCAHost ()
{
    if (!dcaHost)
 	dcaInitDCAHost();

    return (dcaHost->use_sim ? dcaHost->simulated : dcaHost->actual);
}


int	
dcaUseSim()  	
{ 
    if (!dcaHost)
 	dcaInitDCAHost();

    return dcaHost->use_sim; 	
}

char   *
dcaActualHost()	
{ 
    if (!dcaHost)
 	dcaInitDCAHost();

    return (dcaHost->actual ? dcaHost->actual : "None"); 
}

char   *
dcaSimHost()	
{ 
    if (!dcaHost)
 	dcaInitDCAHost();

    return (dcaHost->simulated ? dcaHost->simulated : "None"); 
}

char   *
dcaDomain()	
{ 
    if (!dcaHost)
 	dcaInitDCAHost();

    return (dcaHost->domain ? dcaHost->domain : "None"); 
}
