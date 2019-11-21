#include <stdio.h>
#include "mbus.h"


int main (int argc, char **argv)
{
    int tid = 0;

    if ((tid = mbusConnect ("poker", "client", FALSE))) {
        mbusBcast ("SMCMgr", "process all", MB_START);
        mbusBcast ("SMCMgr", "segList", MB_GET|MB_STATUS);
    }

    mbusDisconnect (tid);
}
