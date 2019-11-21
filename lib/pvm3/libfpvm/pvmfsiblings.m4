
/* $Id: pvmfsiblings.m4,v 1.1 1997/06/26 19:39:57 pvmsrc Exp $ */

#include "pvm3.h"
#include "pvm_consts.h"

void
FUNCTION(pvmfsiblings) ARGS(`nsiblings, sibidx, stid')
int *nsiblings, *sibidx, *stid;
{
	static int nsib = -1;
	static int *sibs;
	if (nsib == -1)
		nsib = pvm_siblings(&sibs);

	*nsiblings = nsib;

	if (*sibidx >= 0 && *sibidx< nsib)
	{
		*stid = sibs[*sibidx];
	}
	else
		*stid = PvmNoTask;
}

