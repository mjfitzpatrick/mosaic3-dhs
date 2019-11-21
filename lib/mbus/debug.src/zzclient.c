#include <stdio.h>
#include "mbus.h"


int main (int argc, char **argv)
{
    int i, tid = 0, sup_tid = 0;

    tid = mbusConnect (COLLECTOR, "client", FALSE);
    sup_tid = mbusSuperTid ();

    printf ("Super:   tid = %d\n", sup_tid);
    printf ("Client:  tid = %d\n", tid);


    printf ("sending msgs ....\n");
    for (i=0; i < 3; i++)
	mbusSend (SUPERVISOR, ANY, MB_STATUS, "This is a test");

    mbusSend ("foo", ANY, MB_STATUS, "This is a test to foo@ANY");
    mbusSend ("foo", "localhost", MB_STATUS, "This is a test to foo@localhost");

    mbusDisconnect (tid);
}
