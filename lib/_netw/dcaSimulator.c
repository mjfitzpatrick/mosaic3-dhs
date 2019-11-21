#include <stdio.h>
/* Function prototypes */
#ifdef __STDC__
#include <stddef.h>
#include <stdlib.h>
#endif


/***************************************************************************
**
**  DCA Supervisor Connection code -- Two simple interface procedures that 
**  open and negotiate a connection to the Supervisor, and then close it.
**  The Supervisor is located on a 'device' specified as 'hostname:port'
**  where the hostname us can be a simple host name, a fully-qualified
**  host name, or an IP address.
**
**	fd = dcaSupOpenConn (host, port, name)	// returns NULL on failure
**	   dcaSupSendStatus (fd, msg);		// send status message
**	    dcaSupCloseConn (fd);		// close connection
**
***************************************************************************/

#define SIM_DEBUG (getenv("SIM_DEBUG")!=NULL||access("/tmp/SIM_DEBUG",F_OK)==0)


static int simulator 	= 0;
static int initialized 	= 0;


int dcaSimulator (void);
int dcaSetSimMode (int mode);
int dcaGetSimMode (void);


int
dcaSimulator () 
{
    return (simulator);
}



int
dcaSetSimMode (int mode) 
{
    if (initialized++)
	return 0;
    else
	simulator = mode;     
    return 0;
}


int
dcaGetSimMode () 
{
    return (dcaSimulator());
}


