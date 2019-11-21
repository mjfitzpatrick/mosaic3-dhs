#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "dcaDhs.h"

void dcaCloseChannel (struct dhsChan *chan)

{
    int istat;

    istat = close (chan->fd);
}
