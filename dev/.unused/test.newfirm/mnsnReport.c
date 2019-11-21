/*******************************************************************************
 * include(s):
 *******************************************************************************/
/*
#if !defined(_msgUtil_H_)
 #include "msgUtil.h"
#endif
*/

#if !defined(_dheUtil_H_)
#include "dheUtil.h"
#endif
/*******************************************************************************
 * mnsnReport ( ... )
 *******************************************************************************/
void mnsnReport(FILE * fd,	/* stream                    */
		char *message)
{
    /* declare some variables and initialize them */
    ulong n;
    long istat;
    char resp[MAXMSG];

    if (message == (char *) NULL || strlen(message) == 0) {
	DPRINT(0, dlibDebug, "mnsnReport: Null or Empty message\n");
	return;
    }
    return;
}
