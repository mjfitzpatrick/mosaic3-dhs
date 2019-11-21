/*  
**  MBHANDLE.C  -- Input handler procedures.  These procedures can be used
**  to register functions to be called when input is available of a given
**  file descriptor, typically some sort of local processing for input coming
**  from the stdin or an open socket, and a handler for msgbus requests.
**  Only one handler is permitted on each file descriptor.
**                 
**                 
**               mbusAddInputHandler (fd, handler(), client_data)
**            mbusRemoveInputHandler (fd)
**                 
**	  ninputs = mbusInputHandler (timeout)
**                   mbusAppMainLoop (context)
**
**	 	 fd_set = mbInputSet ()			// Private procedures
**                  fd = mbGetMBusFD ()
**                n = mbGetNHandlers ()
**               fd = mbGetHandlerFD (index)
**              stat = mbCallHandler (index)
*/


#include <stdio.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"


#define	SELWIDTH	FD_SETSIZE




/*---------------------------------------------------------------------------
**  MBUSADDINPUTHANDLER -- Add an input/event handler on the given file
**  descriptor.
*/
int
mbusAddInputHandler (int fd, mbFunc handler, void *client_data)
{
    MBusPtr mbus = mbAppGetMBus();
    int     next = 0;


    if (mbus == (MBusPtr)NULL) {
	fprintf (stderr, "Error: NULL mbus pointer.\n");
	return (ERR);
    }

    if (mbus->nhandlers == MAX_HANDLERS)
	return (ERR);

    if (MB_DEBUG)
	fprintf (stderr, "mbusAddInputHandler: fd=%d func=0x%x data=0x%x\n",
	    fd, (int) handler, (int) client_data);

    next = mbus->nhandlers;
    mbus->handlers[next].fd = fd;
    mbus->handlers[next].func = handler;
    mbus->handlers[next].client_data = client_data;

    mbus->nhandlers++;			/* increment the counter	*/

    return (OK);
}


/*---------------------------------------------------------------------------
**  MBUSREMOVEINPUTHANDLER -- Remove an input/event handler on the given file
**  descriptor.
*/
int
mbusRemoveInputHandler (int fd)
{
    MBusPtr mbus = mbAppGetMBus();
    int     i, j, nhandlers;


    if (mbus == (MBusPtr)NULL) {
	fprintf (stderr, "Error: NULL mbus pointer.\n");
	return (ERR);
    }

    nhandlers = mbus->nhandlers;

    if (MB_DEBUG)
	fprintf (stderr, "mbusRemoveInputHandler: fd=%d\n", fd);

    for (i=0; i < nhandlers; i++) {
	/* Look for the handler to remove. */
        if (mbus->handlers[i].fd == fd) {

	    /* Shift remaining array downwards. */
	    if (i == (nhandlers-1)) {
		/* special case -- last element */
		j = i;
	    } else {
	        j = i + 1;
	        while ( j < (nhandlers-1)) {
    		    mbus->handlers[j].fd = mbus->handlers[j+1].fd;
    		    mbus->handlers[j].func = mbus->handlers[j+1].func;
    		    mbus->handlers[j].client_data = 
			mbus->handlers[j+1].client_data;
		    j++;
	        }
	    }
    	    mbus->handlers[j].fd   = 0;	/* zero last element		*/
    	    mbus->handlers[j].func = (mbFunc) NULL;
    	    mbus->handlers[j].client_data = (void *) NULL;

    	    mbus->nhandlers--;		/* decrement the counter	*/
	}
    }

    return (OK);
}


/*---------------------------------------------------------------------------
**  MBUSINPUTHANDLER -- Process any pending input, wait at most 'timeout'
**  milliseconds for input before returning.
*/
int
mbusInputHandler (int timeout)
{
    fd_set allset, mbInputSet();
    int    i, nselect, nfd, fd;
    struct timeval tv;


    allset = mbInputSet ();		/* setup the handler descriptors */
    nfd = mbGetNHandlers ();

    if (MB_DEBUG) 
	fprintf (stderr, "mbusInputHandler: nfd=%d timeout=%d\n", nfd, timeout);

    if (timeout < 0)
	timeout = 0;
    if (timeout >= 1000) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout - tv.tv_sec) * 1000;
    } else {
        tv.tv_sec = 0;
        tv.tv_usec = (timeout * 1000);
    }

    if ((nselect = select(SELWIDTH, &allset, NULL, NULL, &tv)) > 0) {

        /* Loop over the handlers, call the functions where we
         * we have data waiting.
         */
        for (i=0; i < nfd; i++) {
            fd = mbGetHandlerFD (i);
            if (FD_ISSET(fd, &allset)) {
                if (MB_DEBUG) 
		    fprintf (stderr, "Data on descriptor %d\n", fd);
                mbCallHandler (i);
            }
        }
    }

    if (MB_DEBUG) 
	fprintf (stderr, "mbusInputHandler: nselect=%d\n", nselect);

    return (nselect);
}


/*---------------------------------------------------------------------------
**  MBUSAPPMAINLOOP --  Main application input handler loop.  This procedure
**  never returns and should be used by tasks which intend to continuously
**  process input from one or more sources.
*/
void
mbusAppMainLoop (void *context)
{
    fd_set fds, allset, mbInputSet();
    int    i, n, nfd, fd, mbGetNHandlers();


    while (1) {				/* loop for input events	 */
        allset = mbInputSet ();		/* setup the handler descriptors */
        nfd = mbGetNHandlers ();
        fds = allset;

        if (MB_DEBUG) {
	    fprintf (stderr, "mbusAppMainLoop: nfd=%d\n", nfd);
	    fprintf (stderr, "mbusAppMainLoop: waiting for input....\n");
	}

        if ((n = select(SELWIDTH, &fds, NULL, NULL, NULL)) > 0) {

            /* Loop over the handlers, call the functions where we
             * we have data waiting.
             */
            for (i=0; i < nfd; i++) {
                fd = mbGetHandlerFD (i);
                if (FD_ISSET(fd, &fds)) {
                    if (MB_DEBUG) 
			fprintf (stderr, "Data on descriptor %d\n", fd);
                    mbCallHandler (i);
                }
            }
        }
    }
}



/*---------------------------------------------------------------------------
**  Private Interface procedures.
**-------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
**  MBINPUTSET -- Return a file descriptor set made up of the currently
**  installed handler file descriptors.
*/
fd_set
mbInputSet ()
{
    fd_set  allset;
    MBusPtr mbus = mbAppGetMBus();
    int     i;

    FD_ZERO(&allset);

    if (MB_DEBUG) 
	printf ("mbInputSet: nhandlers = %d\n", mbus->nhandlers);
    for (i=0; i < mbus->nhandlers; i++) {
	if (MB_DEBUG) 
	    printf ("mbInputSet: %d = %d\n", i, mbus->handlers[i].fd);
	FD_SET(mbus->handlers[i].fd, &allset);
    }

    return (allset);
}


/*---------------------------------------------------------------------------
** MBGETMBUSFD -- Get the file descriptor from this task to the pvmd.
*/
int
mbGetMBusFD ()
{
    int    *pvm_fds, nfds, i;

    if ((nfds = pvm_getfds (&pvm_fds)) == 0) {
        return (ERR);

    } else {
	/*  fd[0] is always the descriptor from the task to the pvmd.
	 */

	if (MB_DEBUG) {
	    for (i=1; i <= nfds; i++)
		fprintf (stderr, "mbGetMBusFD: %d/%d = %d\n",
		    i, nfds, pvm_fds[i-1]);
	}
        return (pvm_fds[0]);    
    }
}


/*---------------------------------------------------------------------------
** MBGETNHANDLERS - Get the number of input handlers currently registered.
*/
int
mbGetNHandlers ()
{
    MBusPtr mbus = mbAppGetMBus();
    return (mbus->nhandlers);
}


/*---------------------------------------------------------------------------
** MBGETHANDLEFD -- Get the file descriptor given an index number.
*/
int
mbGetHandlerFD (int index)
{
    MBusPtr mbus = mbAppGetMBus();
    return (mbus->handlers[index].fd);
}


/*---------------------------------------------------------------------------
** MBCALLHANDLER -- Call the input handler given the index number.
*/
int
mbCallHandler (int index)
{
    MBusPtr mbus = mbAppGetMBus();
    int     stat, fd;
    void    *client_data;


    fd = mbus->handlers[index].fd;
    client_data = mbus->handlers[index].client_data;

    /* Call the handler function.  Note that the client_data may be
     * a pointer to another handler function we'll call to process the
     * message, e.g. a "standard" handler that may process certain system
     * messages but then leave others for the user-defined handler.
     */
    stat  = (int) (*(mbus->handlers[index].func)) (fd, client_data);

    return (stat);
}
