/*  
**  MPAPP.C -- MBUS application state procedures.
**
**                 mbInitApp ()
**
**           ival = mbAppGet (what)
**        str = mbAppGetName ()
**        ptr = mbAppGetMBus ()
**
**                  mbAppSet (what, val)
**              mbAppSetName (name)
**              mbAppSetMBus (mbus)

*/

#include <stdio.h>
#include <string.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"


/*  Application state structure.
*/
static struct {
    int     super_tid;				/* Supervisor tid	    */
    int     mytid;				/* This app's tid	    */
    char    *whoAmI;				/* This app's name	    */

    int     pvm_fd;				/* fd to PVM		    */
    MBusPtr mbus;				/* backpointer to mbus	    */

    int     initialized;			/* app initialized?	    */

} App;




/*---------------------------------------------------------------------------
**  MBINITAPP -- Initialize the application state structure.
*/
void
mbInitApp ()
{
    mbAppSet (APP_TID,  0); 	/* reset the static state of this client */
    mbAppSet (APP_STID, 0);
    mbAppSet (APP_INIT, 0);
    mbAppSet (APP_FD,   0);
    mbAppSetMBus ((MBusPtr) NULL);
    mbAppSetName (NULL);
}


/*---------------------------------------------------------------------------
**  MBAPPGET -- Get a value from the app state structure.
*/
int
mbAppGet (int what)
{
    switch (what) {
    case APP_TID:    return (App.mytid);  	break;
    case APP_STID:   return (App.super_tid);  	break;
    case APP_FD:     return (App.pvm_fd); 	break;
    case APP_INIT:   return (App.initialized); 	break;
    }

    return (-1);
}


/*---------------------------------------------------------------------------
**  MBAPPGETNAME -- Return the name of the current application.
*/
char *
mbAppGetName ()
{
    return (App.whoAmI);
}


/*---------------------------------------------------------------------------
**  MBAPPGETMBUS -- Return the mbus structure pointer for this app.
*/
MBusPtr 
mbAppGetMBus ()
{
    return (App.mbus);
}


/*---------------------------------------------------------------------------
**  MBAPPSET -- Set a value in the app state structure.
*/
void
mbAppSet (int what, int val)
{
    switch (what) {
    case APP_TID:    App.mytid = val;  		break;
    case APP_STID:   App.super_tid = val;  	break;
    case APP_FD:     App.pvm_fd = val; 		break;
    case APP_INIT:   App.initialized = val;  	break;
    }
}


/*---------------------------------------------------------------------------
**  MBAPPSETNAME -- Save the name of the appication to the app state.
*/
void
mbAppSetName (char *name)
{
    if (name == (char *)NULL) { 
	if (App.whoAmI != (char *)NULL)
	    free ((char *) App.whoAmI);
        App.whoAmI =  (char *)NULL; 
    } else if (name) {
        if (App.whoAmI == (char *)NULL) 
	    App.whoAmI = (char *) calloc (1, 64);
        strncpy (App.whoAmI, name, strlen(name));
    }
}


/*---------------------------------------------------------------------------
**  MBAPPSETMBUS -- Save the mbus structure pointer to the app state.
*/
void
mbAppSetMBus (MBusPtr mbus)
{ 
    App.mbus =  mbus; 
}
