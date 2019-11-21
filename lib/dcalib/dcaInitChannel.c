#if !defined(_dhsUtil_H_)
#include "dhsUtil.h"
#endif
#if !defined(_dhsImpl_H_)
#include "dhsImplementationSpecifics.h"
#endif


#include "dcaDhs.h"

#define DYNAMIC_COLLECTOR



/* DCAINITCHANNEL -- Initialize a channel to either the supervisor or a
 * collector process.  If the requested channel is not already connected,
 * or the destination specification has changed since it was opened, we will
 * open a new connection here.  Otherwise, we verify the connection is still
 * working and simply reset counters,
 */
int dcaInitChannel(dhsNetw * dhsID, XLONG type)
{
    struct dhsChan *chan, *collChan;
    int status;
    int sock, socket;
    char line[120];


    if (type == DCA_SUPERVISOR) {
	if (dhsID->super == NULL) {
	    chan = (struct dhsChan *) malloc(sizeof(struct dhsChan));

	    /*  Defining a default node and port, we do not
	    **  need the node:port definition as well.
	    */
	    dhsID->super = chan;

	    /* We get the node and and port# from an environment variable
	    ** that the client code (panSaver) reads.
	    */
	    chan->node = dhs.dhsSuperNode;
	    chan->port = dhs.dhsSuperPort;
	    chan->initialized = 1;
	    chan->connected = NO;
	    DPRINT(40, procDebug,
		   "dcaInitChannel DCA_SUPERVISOR: Default channel setting\n");

	} else {
	    chan = dhsID->super;
	}

	if (chan->connected > 0) {

	    /*  We're already connected to Supervisor, test connection by
	    **  sending a NO-OP to get the ACK.
	    */
	    DPRINT(40, procDebug,
		   "dcaInitChannel DCA_SUPERVISOR: connected\n");
	    if (dcaSendMsg(chan->fd,dcaFmtMsg(DCA_NOOP,(XLONG)NULL))==DCA_OK) {
		chan->nerrs = 0; 	/* connection fine, just return. */
		chan->nresends = 0;
		chan->connected = DCA_OK;
		return DCA_OK;

	    } else {
		/* Close current connection and try to reopen below.
		*/
		DPRINT(40, procDebug,
		       "dcaInitChannel DCA_SUPERVISOR: Close chan\n");
		close(chan->fd);
	    }
	}

	/* Get a connection string for node:port for the channel 
	   chan->device  = (char *)dcaGetSuperDev (dhsID);
	   chan->node    = dcaGetNode (chan->device);
	   chan->port    = dcaGetPort (chan->device);
	 */
	chan->node = dhs.dhsSuperNode;
	chan->port = dhs.dhsSuperPort;
	chan->name = dhs.dhsName;
	chan->nerrs = 0;
	chan->nresends = 0;

	/* Open a channel to the Supervisor.
	 */
	chan->fd = dcaSupOpenConn (chan->node, chan->port, chan->name);
	if (chan->fd > 0) {
	    DPRINT(40, procDebug,
		   "dcaInitChannel DCA_SUPERVISOR: Connected\n");
	    dhsID->nopen++;

	} else {
	    fprintf(stderr, "Error connecting to Supervisor machine: %s\n",
		    chan->node);
	    return DCA_ERR;
	}
	dhsID->super = chan;

	return DCA_OK;

    } else if (type == DCA_COLLECTOR) {

	/* Initialize a channel to the Supervisor if it isn't already open.
	*/
	if (dhsID->super == NULL) {
	    DPRINT(40, procDebug, "Reinitializeing SUPER connection\n");
	    dcaInitChannel(dhsID, DCA_SUPERVISOR);
	}


	/* Send a message to supervisor asking for collector device string.
	 */
	if (dhsID->collector == NULL) {

	    collChan = (struct dhsChan *) malloc(sizeof(struct dhsChan));
	    dhsID->collector = collChan;
#ifdef DYNAMIC_COLLECTOR
	    collChan->device  = (char *)dcaGetDCADev (dhsID->super->fd);
	    collChan->node    = dcaGetNode (collChan->device);
	    collChan->port    = dcaGetPort (collChan->device);
#else
	    collChan->node = dhs.dhsCollectorNode;
	    collChan->port = dhs.dhsCollectorPort;
	    sprintf (collChan->device, "%s:%d\0",
		collChan->node, collChan->port);
#endif
fprintf (stderr, "initChannel:  dev='%s'  node='%s' port=%d\n",
    collChan->device, collChan->node, collChan->port);
	    collChan->name = dhs.dhsName;
	    sprintf(line, "COLLECTOR: node: %s, port: %d\n",
		    collChan->node, collChan->port);
	    DPRINT(40, procDebug, line);

	    collChan->initialized = 1;
	    collChan->connected = NO;

	} else {
	    collChan = dhsID->collector;
	}

	if (collChan->connected > 0) {
	    /* We're already connected to a collector.
	     */
	    DPRINT(40, procDebug, "Collector: We are already connected\n");
	    if ((status = dcaConnectionACK(collChan->fd)) == DCA_ALIVE) {

		collChan->nerrs = 0;	/* connection fine, just return.  */
		collChan->nresends = 0;
		collChan->connected = DCA_CON;
		DPRINT(40, procDebug, "Collector: connection alive\n");
		return DCA_OK;

	    } else {
		/* Close current connection and try to reopen below.
		 */
		DPRINT(40, procDebug, "Collector: close connection - retry\n");
		socket = collChan->fd;
		collChan->fd = 0;
		close(socket);
	    }
	}


	/* Open a socket to the Collector.
	 */
	DPRINT(40, procDebug, "Collector: Try to open again\n");
	(void) dcaOpenChannel(collChan, &sock);
	if (sock > 0) {
	    DPRINT(40, procDebug, "Collector: opened successfully\n");
	        collChan->connected++;
	} else {
	    DPRINT(40, procDebug, "Collector: open failed\n");
	        collChan->connected=0;
	    fprintf(stderr, "Error connecting to Collector machine: %s:%d\n",
		    collChan->node, collChan->port);
	    return DCA_ERR;
	}
	dhsID->collector = collChan;
	return DCA_OK;

    } else {
	fprintf(stderr, "ERROR: dcaInitChannel: 'type' not supported");
	return DCA_ERR;
    }
}
