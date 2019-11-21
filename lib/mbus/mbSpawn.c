/*
**  MBSPAWN.C -- Spawn a process on the message bus.
**
*/

#include <stdio.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"


/*---------------------------------------------------------------------------
**  MBUSSPAWN -- Spawn a process on the requested node of the virtual machine.
*/
int
mbusSpawn (char *task, char **argv, char *where, int *tid)
{
    int stat = ERR, flag;
    char *host = where;

    if (where)
	flag = PvmTaskHost;
    else {
	flag = PvmTaskDefault;
	host = "localhost:/tmp/";
    }

   
    /* Spawn the task. */
    stat = pvm_spawn (task, argv, flag, host, 1, tid);

    switch (*tid) {
    case PvmBadParam:
	fprintf (stderr, "mbSpawn: Invalid argument to pvm_spawn()\n");
	return (ERR);
    case PvmNoHost:
	fprintf (stderr, "mbSpawn: Specified host not in virtual machine\n");
	return (ERR);
    case PvmNoFile:
	fprintf (stderr, "mbSpawn: executable not found.\n");
	return (ERR);
    case PvmNoMem:
	fprintf (stderr, "mbSpawn: Not enough memory on host.\n");
	return (ERR);
    case PvmSysErr:
	fprintf (stderr, "mbSpawn: pvmd not responding\n");
	return (ERR);
    case PvmOutOfRes:
	fprintf (stderr, "mbSpawn: out of resources\n");
	return (ERR);
    }

    if (MB_DEBUG) {
	printf ("mbSpawn:  task='%s' on '%s' returns %d (tid=%d)\n",
	    task, host, stat, *tid);
    }

    return (stat);
}
