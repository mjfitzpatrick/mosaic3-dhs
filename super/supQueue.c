#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
/* Function prototypes */
#ifdef __STDC__
#include <stddef.h>
#include <stdlib.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Tcl/tcl.h>
#include <Obm.h>

#include "super.h"


/*  Process Queue Interface --
**
**                  pqInit (sup)
**                   pqAdd (sup, expID)
**          expID = pqNext (sup)
**               pqSetStat (sup, expID, status)
**        stat = pqGetStat (sup, expID)
**    stat = pqDataWaiting (sup)
**
*/


extern char   *procTimestamp();

extern int dca_done;


#define	Q_INDEX(__i__)		(SZ_PROC_QUEUE % __i__)

#define Q_DEBUG     (getenv("Q_DEBUG")!=NULL||access("/tmp/Q_DEBUG",F_OK)==0)


/*  PQINIT -- Initialize the processing queue.  This is stored as an
**  array of fixed length, but we'll set the 'next' pointers so we can
**  traverse it as a circular list.  The queue capacity is exceeded when
**  the 'last' pointer passes the 'first'.
*/
void
pqInit (supDataPtr sup)
{
    int i;

    for (i=0; i < (SZ_PROC_QUEUE-1); i++) {
	sup->queue[i].next = &sup->queue[i+1];
	sup->queue[i].expID = 0.0;
	sup->queue[i].status = PQ_AVAILABLE;
    }
    sup->queue[i].next = &sup->queue[0];

    sup->qFirst = &sup->queue[0];
    sup->qLast  = &sup->queue[SZ_PROC_QUEUE-1];
    sup->qLast->next = sup->qFirst;
    sup->qCount = 0;
}


/*  PQADD - Add a new entry to the queue.  We add an entry at the tail and
**  set an initial status.
*/
void
pqAdd (supDataPtr sup, double expID)
{
    pqNodePtr p = sup->qLast->next;


    if (Q_DEBUG) {
	fprintf (stderr, "===========================================\n");
	fprintf (stderr,"pqAdd: qFirst=0x%x next=0x%x stat=%d expID=%.6lf\n",
  	    (int) sup->qFirst, (int) sup->qFirst->next, sup->qFirst->status, 
	    sup->qFirst->expID);
	fprintf (stderr,"pqAdd: qLast=0x%x next=0x%x stat=%d expID=%.6lf\n",
  	    (int) sup->qLast, (int) sup->qLast->next, sup->qLast->status, 
	    sup->qLast->expID);
	pqPrintQ (sup);
	fprintf (stderr, "===========================================\n");
    }

    if (sup->qCount == SZ_PROC_QUEUE) {
	fprintf (stderr, "pqAdd:  Queue length exceeded\n\n");
	exit (1);
    }

    p->expID  = expID;
    p->status = PQ_START;

    strcpy (p->t_in,   procTimestamp());
    memset (p->t_out,  0, SZ_PDATE);
    memset (p->t_done, 0, SZ_PDATE);

    sup->qLast = p;			/* Update last pointer		*/
    sup->qCount++;

    if (sup->qCount > 0)
	sup_message (sup, "dataFlag", "waiting 1");

    if (sup->qCount > SZ_PROC_QUEUE) {
	fprintf (stderr, "procQueue overflow\n");
	exit (1);
    }

}


/* PQNEXT -- Return the ExpID of the next exposure to the processed.
*/
double
pqNext (supDataPtr sup)
{
    double expID;


    if (Q_DEBUG) {
	fprintf (stderr,
	    "pqNext: count=%d qFirst=0x%x stat=%d  expID=%.6lf done=%d\n", 
		sup->qCount, (int) sup->qFirst, sup->qFirst->status, 
		sup->qFirst->expID, dca_done);
/*
	fprintf (stderr, "===========================================\n");
	fprintf (stderr,"pqNext: qFirst=0x%x  next=0x%x stat=%d  expID=%.6lf\n",
  	    sup->qFirst, sup->qFirst->next, sup->qFirst->status, 
	    sup->qFirst->expID);
	fprintf (stderr,"pqNext: qLast=0x%x  next=0x%x  stat=%d  expID=%.6lf\n",
  	    sup->qLast, sup->qLast->next, sup->qLast->status, 
	    sup->qLast->expID);
	pqPrintQ (sup);
	fprintf (stderr, "===========================================\n");
*/
    }

    /*  If first==last the queue is empty, return 0.
    if (sup->qCount > 0 && sup->qFirst->status == PQ_READY) {
    if (sup->qLast->next == sup->qFirst || sup->qFirst->status == PQ_READY) {
    */
    if (sup->qFirst->status == PQ_READY) {
        expID = sup->qFirst->expID;

        /* If we get to this point, the exposure is about to be processed, 
        ** so set the status flag to indicate it is active.
        pqSetStat (sup, expID, PQ_ACTIVE);
        */
        return (expID);

    } else
	return ((double)0.0);
}


/* PQSETSTAT -- Set the status for the element identified by the ExpID.
*/
void
pqSetStat (supDataPtr sup, double expID, int status)
{
    pqNodePtr p, old_first;
    int i;


    if (Q_DEBUG) {
	fprintf (stderr,"pqSetStat: expID=%.6lf  status=%d\n", expID, status);
/*
	fprintf (stderr, "===========================================\n");
	fprintf (stderr,"pqSetStat: qFirst=0x%x next=0x%x stat=%d exp=%.6lf\n",
  	    sup->qFirst, sup->qFirst->next, sup->qFirst->status, 
	    sup->qFirst->expID);
	fprintf (stderr,"pqSetStat: qLast=0x%x next=0x%x stat=%d expID=%.6lf\n",
  	    sup->qLast, sup->qLast->next, sup->qLast->status,sup->qLast->expID);
	pqPrintQ (sup);
	fprintf (stderr, "===========================================\n");
*/
    }

    i = 0;
/*
    for (p=sup->qFirst; p != sup->qLast->next; p=p->next) {
*/
    for (p=sup->qFirst; i++ < (SZ_PROC_QUEUE-1); p=p->next) {

	if (smcEqualExpID(p->expID,expID)) {
	    p->status = status;
	    if (status == PQ_DONE) {
    		old_first = sup->qFirst;
		old_first->expID = 0.0;
		old_first->status = PQ_AVAILABLE;

    		sup->qFirst = sup->qFirst->next;
    		sup->qCount--;

		if (sup->qCount == 0)
		    sup_message (sup, "dataFlag", "waiting 0");
	    }
	    break;
	}
    }

    if (sup->qCount < 0) {
	fprintf (stderr, "procQueue underflow\n");
	exit (1);
    }
}


/* PQGETSTAT -- Get the status for the element identified by the ExpID.
*/
int
pqGetStat (supDataPtr sup, double expID)
{
    pqNodePtr p;

    for (p = sup->qFirst; p != sup->qLast->next; p = p->next) {
	if (smcEqualExpID(p->expID,expID)) {
	    return (p->status);
	}
    }

    return (0);
}


/* PQDATAWAITING -- Peek at the queue to see whether the next image is
** ready for processing.
*/
int
pqDataWaiting (supDataPtr sup)
{
    if (sup->qCount > 0 && sup->qFirst->status == PQ_READY)
	return (1);
    else
	return (0);
}


void
pqPrintQ (supDataPtr sup)
{
    pqNodePtr p;
    int i = 0;

    fprintf (stderr, "Queue Count:  %d\n", sup->qCount);
    for (p = sup->qFirst; p != sup->qLast->next; p = p->next) {
	if (p) {
	    fprintf (stderr, "%d:  0x%x  0x%x  expID=%.6lf  status=%d  %s\n",
	        i, (int) p, (int) p->next, p->expID, p->status,
	        (p == sup->qFirst ? "First" : 
	        (p == sup->qLast ? "Last" : " ")) ); 
	} else {
	    fprintf (stderr, "%d:  NULL\n", i);
	}
	i++;
    }
}


void
pqTest (supDataPtr sup)
{
    double id;

    printf ("Initialized....Adding\n");

    pqAdd (sup, 1.0);
    pqAdd (sup, 1.1);
    pqAdd (sup, 1.2);
    pqAdd (sup, 1.3);

    printf ("First read...\n");
    while ((id = pqNext(sup)) > 0)
 	printf ("id = %f\n", id);
    printf ("Second read...\n");
    pqSetStat (sup, 1.0, PQ_READY);
    pqSetStat (sup, 1.1, PQ_READY);
    pqSetStat (sup, 1.2, PQ_READY);
    while ((id = pqNext(sup)) > 0)
 	printf ("id = %f\n", id);

    pqAdd (sup, 2.0);
    pqAdd (sup, 2.1);
    printf ("Third read...\n");
    while ((id = pqNext(sup)) > 0)
 	printf ("id = %f\n", id);
    printf ("Done\n");
}
