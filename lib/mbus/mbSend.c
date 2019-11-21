/*  
**  MBSEND.C -- Send a message.
**
**
**           stat = mbusSend (to, host, subject, msg)
**               mbusPackMsg (ack, to, host, subject, msg)
**      stat = mbusGetMsgAck (tid, tag)
**
**           tid = mbTidCode (appname) 			//  Private procedure
*/

#include <stdio.h>
#include <string.h>
#include <pvm3.h>

#define _LIBMBUS_SOURCE_
#include "mbus.h"



/*---------------------------------------------------------------------------
**  MBUSSEND --  Send a message to another application or the Supervisor
*/
int
mbusSend (char *to, char *host, int subject, char *msg)
{
    int ack = ERR, info;
    int to_tid = mbTidCode(to);
    


    if (MB_DEBUG) 
	fprintf (stderr, "mbSend: to='%s'(%d) host='%s' subject=%d msg='%s'\n",
	    to, to_tid, host, subject, msg);

    if (host && (strcmp(host,ANY) != 0)) {
	fprintf (stderr, "mbSend: host-specific msg not yet implemented\n");
	return (ERR);

        /*  Host-specific messages require an ACK from the target app.
	return ( mbusGetMsgAck (to_tid, subject) );
         */

    }


    /* Format the message.  The elements of the message are packed into 
     * the current send buffer we actually deliver below.
     */
    if (to_tid == ERR) {
	/* Receiver is not available on the bus.  Mogrify the message
	 * and send it to the Super instead so it can be logged/ignored.
	 */
	char *buf = malloc (200 + strlen (msg));

	sprintf (buf, "Orphan {From: %d  To: %s  Subj: %d -- (%s)}",
	    mbAppGet(APP_TID), to, subject, msg);

        mbusPackMsg (1, SUPERVISOR, "any", MB_ERR, buf);
	to_tid = mbAppGet(APP_STID);

	free ((char *)buf);

    } else
        mbusPackMsg (isSupervisor(to), to, host, subject, msg);


    if ((info = pvm_send (to_tid, subject)) < 0) {
	switch (info) {
	case PvmBadParam:
	    fprintf (stderr, "Send: %d fails, bad tid/msgtag\n", to_tid);
	    return (ERR);
	case PvmSysErr:
	    fprintf (stderr, "Send: %d fails, pvmd not responding\n", to_tid);
	    return (ERR);
	case PvmNoBuf:
	    fprintf (stderr, "Send: %d fails, no active buffer\n", to_tid);
	    return (ERR);
	}
    } 


    /*  Require a message ACK only from the Supervisor.
     */
    if (USE_ACK)
        ack = (isSupervisor(to) ? mbusGetMsgAck (to_tid, subject) : OK);
    else
        ack = 0;

    return (ack);
}



/*---------------------------------------------------------------------------
** Format the message.  All messages are of the form:
**
**  <ack> <from_tid> <to_tid> <subject> <host_len> [<host>] <msg_len> [<msg>]
**
** where
**	   <ack>	int	ACK required?		    (Reply_To:)
**	   <from_tid>	int	tid of sending process 	    (To:)
**	   <to_tid>	int	tid of receiving process    (From:)
**	   <subject>	int	reason for message 	    (Subject:)
**	   <host_len>	int	dest machine of message	    (Host:)
**	   <msg_len>	int	text message itself 	    (Msg Body)
**
**  If the where/what lengths are non-zero they are immediate followed
**  by a NULL-terminated string of bytes.
**
*/
void 
mbusPackMsg (int ack, char *to, char *host, int subject, char *msg)
{
    char   czero = 0;
    int    len = 0, izero = 0;

    int      to_tid = mbTidCode(to);
    int    from_tid = mbAppGet (APP_TID);


    pvm_initsend (PvmDataDefault);
    if (USE_ACK) {
        ack = 0;
        pvm_pkint (&ack, 1, 1);
    } else {
        ack = 0;
        pvm_pkint (&ack, 1, 1);
    }
    pvm_pkint (&from_tid, 1, 1);
    pvm_pkint (&to_tid, 1, 1);
    pvm_pkint (&subject, 1, 1);

    if (host) {
	len = strlen (host) + 1;
	pvm_pkint (&len, 1, 1);
	pvm_pkbyte (host, len, 1);
	pvm_pkbyte (&czero, 1, 1);
    } else
	pvm_pkint (&izero, 1, 1);

    if (msg) {
	len = strlen (msg) + 1;
	pvm_pkint (&len, 1, 1);
	pvm_pkbyte (msg, len, 1);
	pvm_pkbyte (&czero, 1, 1);
    } else
	pvm_pkint (&izero, 1, 1);
}


/*---------------------------------------------------------------------------
**  Get an ACK in response to a particular message.
*/
int
mbusGetMsgAck (int tid, int tag)
{
    int bufid = 0, ack = ERR;

    if (! USE_ACK)
	return (0);

	    
fprintf (stderr, "getAck: SHOULDN'T BE HERE....\n");
    /* Get the ACK.
    if ((bufid = pvm_recv (tid, tag)) < 0) {
    */
	    
    if ((bufid = pvm_recv (-1, tag)) < 0) {
	switch (bufid) {
	case PvmBadParam:
	    fprintf (stderr, "Recv: %d fails, bad tid/msgtag\n", tid);
	    return (ERR);
	case PvmSysErr:
	    fprintf (stderr, "Recv: %d fails, pvmd not responding\n", tid);
	    return (ERR);
	}
    } else
	pvm_upkint (&ack, 1, 1);

    if (MB_DEBUG) 
	fprintf (stderr, "mbGetAck: ack=%d\n", ack);

    return (ack);
}




/*---------------------------------------------------------------------------
**  MBTIDCODE -- Lookup an application name and convert to a tid.
*/
int
mbTidCode (char *appname) 
{
    int data, cc;
    
    return (((cc = pvm_lookup (appname, -1, &data)) < 0) ? -1 : data);
}
