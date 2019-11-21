
static char rcsid[] = 
	"$Id: message.c,v 4.50 1998/04/09 21:11:41 kohl Exp $";

/*
 *         XPVM version 1.1:  A Console and Monitor for PVM
 *           Oak Ridge National Laboratory, Oak Ridge TN.
 *           Authors:  James Arthur Kohl and G. A. Geist
 *                   (C) 1994 All Rights Reserved
 *
 *                              NOTICE
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted
 * provided that the above copyright notice appear in all copies and
 * that both the copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Neither the Institution, Oak Ridge National Laboratory, nor the
 * Authors make any representations about the suitability of this
 * software for any purpose.  This software is provided ``as is''
 * without express or implied warranty.
 *
 * XPVM was funded by the U.S. Department of Energy.
 */


/* XPVM Headers */

#include "xpvm.h"

#include "globals.h"


void
send_message( ST, src, dst, msgtag, msgctx, nbytes, sec, usec )
TASK ST;
int src;
int dst;
int msgtag;
int msgctx;
int nbytes;
int sec;
int usec;
{
	MSG_GRID G;

	TASK DT;

	MSG M;

	int found;

	DT = get_task_tid( dst );

	found = 0;

	if ( ST != NULL && DT != NULL )
	{
		G = PENDING_MSG_GRID[ ST->index ][ DT->index ];

		while ( G != NULL && !found )
		{
			/* printf( "G" ); fflush( stdout ); */

			M = G->msg;

			if ( M->msgtag == msgtag
				&& ( msgctx == -1 || M->msgctx == msgctx )
				&& M->sendtime.tv_sec == -1
				&& M->sendtime.tv_usec == -1 )
			{
				found++;
			}

			else
				G = G->next;
		}
	}

	else
	{
		M = PENDING_MSG_LIST;

		while ( M != NULL && !found )
		{
			/* printf( "M" ); fflush( stdout ); */

			if ( M->dst_tid == dst && M->src_tid == src
				&& M->msgtag == msgtag
				&& ( msgctx == -1 || M->msgctx == msgctx )
				&& M->sendtime.tv_sec == -1
				&& M->sendtime.tv_usec == -1 )
			{
				found++;
			}

			else
				M = M->next;
		}
	}

	/* Found It */

	if ( found )
	{
		M->ST = ST;

		M->sendtime.tv_sec = sec;
		M->sendtime.tv_usec = usec;

		if ( M->prev != NULL )
			M->prev->next = M->next;

		else
			PENDING_MSG_LIST = M->next;

		if ( M->next != NULL )
			M->next->prev = M->prev;

		msg_grid_del( M );

		update_message( M );

		net_del_message( M, FALSE );
	}

	/* Didn't Find Matching Recv, Make New Message */

	else
	{
		M = create_msg();

		if ( PENDING_MSG_LIST != NULL )
		{
			M->next = PENDING_MSG_LIST;

			PENDING_MSG_LIST->prev = M;
		}

		PENDING_MSG_LIST = M;

		M->ST = ST;
		M->DT = DT;

		M->src_tid = src;
		M->dst_tid = dst;

		M->msgtag = msgtag;
		M->msgctx = msgctx;

		M->nbytes = nbytes;

		if ( nbytes != -1 )
			M->assbytes = nbytes;

		else
			M->assbytes = MSG_DEFAULT_NBYTES;

		M->sendtime.tv_sec = sec;
		M->sendtime.tv_usec = usec;

		msg_grid_add( M );

		net_add_message( M );

		queue_message( M, TRUE );
	}
}


void
recv_message( DT, dst, src, msgtag, msgctx, nbytes, sec, usec )
TASK DT;
int dst;
int src;
int msgtag;
int msgctx;
int nbytes;
int sec;
int usec;
{
	MSG_GRID G;

	TASK ST;

	MSG M;

	int found;

	ST = get_task_tid( src );

	found = 0;

	if ( ST != NULL && DT != NULL )
	{
		G = PENDING_MSG_GRID[ ST->index ][ DT->index ];

		while ( G != NULL && !found )
		{
			/* printf( "g" ); fflush( stdout ); */

			M = G->msg;

			if ( M->msgtag == msgtag
				&& ( msgctx == -1 || M->msgctx == msgctx )
				&& M->recvtime.tv_sec == -1
				&& M->recvtime.tv_usec == -1 )
			{
				found++;
			}

			else
				G = G->next;
		}
	}

	else
	{
		M = PENDING_MSG_LIST;

		while ( M != NULL )
		{
			/* printf( "m" ); fflush( stdout ); */

			if ( M->dst_tid == dst && M->src_tid == src
				&& M->msgtag == msgtag
				&& ( msgctx == -1 || M->msgctx == msgctx )
				&& M->recvtime.tv_sec == -1
				&& M->recvtime.tv_usec == -1 )
			{
				found++;
			}

			else
				M = M->next;
		}
	}

	/* Found It */

	if ( found )
	{
		M->DT = DT;

		M->nbytes = nbytes;

		M->recvtime.tv_sec = sec;
		M->recvtime.tv_usec = usec;

		if ( M->prev != NULL )
			M->prev->next = M->next;

		else
			PENDING_MSG_LIST = M->next;

		if ( M->next != NULL )
			M->next->prev = M->prev;

		msg_grid_del( M );

		update_message( M );

		net_del_message( M, TRUE );

		dequeue_message( M, TRUE );

		M->assbytes = nbytes;
	}

	/* Didn't Find Matching Send, Make New Message */

	else
	{
		M = create_msg();

		if ( PENDING_MSG_LIST != NULL )
		{
			M->next = PENDING_MSG_LIST;

			PENDING_MSG_LIST->prev = M;
		}

		PENDING_MSG_LIST = M;

		M->DT = DT;
		M->ST = ST;

		M->src_tid = src;
		M->dst_tid = dst;

		M->msgtag = msgtag;
		M->msgctx = msgctx;

		M->nbytes = nbytes;
		M->assbytes = nbytes;

		M->recvtime.tv_sec = sec;
		M->recvtime.tv_usec = usec;

		msg_grid_add( M );
	}
}


void
update_message( M )
MSG M;
{
	TRC_HOST H;

	int delta;

	REFRESH_GLOBAL( ST_ACTIVE );

	if ( !strcmp( CHAR_GLOBVAL( ST_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD
		&& !tflag )
	{
		place_message( M );
	}

	else
		NEEDS_REDRAW_ST = TRUE;

	/* Add New Message to Comm List */

	if ( MSG_LIST != NULL )
		M->next = MSG_LIST;

	else
		M->next = (MSG) NULL;

	MSG_LIST = M;

	/* Check for Tachyons */

	/*
	if ( TIME_COMPARE( M->recvtime.tv_sec, M->recvtime.tv_usec,
		M->sendtime.tv_sec, M->sendtime.tv_usec ) < 0 )
	{
		H = M->DT->host;

		delta = TIME_ELAPSED( M->recvtime, M->sendtime ) + 1000;

		printf( "Tachyon for %s delta=%d\n", H->name, delta );

		printf( "\tsync before %d/%d\n",
			H->delta.tv_sec, H->delta.tv_usec );

		H->delta.tv_usec += delta;

		if ( H->delta.tv_usec > 1000000 )
		{
			H->delta.tv_usec -= 1000000;

			(H->delta.tv_sec)++;
		}

		printf( "\tsync after %d/%d\n",
			H->delta.tv_sec, H->delta.tv_usec );
	}
	*/
}


void
place_message( M )
MSG M;
{
	TASK ST, DT;

	char cmd[1024];

	int x1, y1, x2, y2;
	int sy, dy;
	int new;

	REFRESH_GLOBAL( ST_COMM_COLOR );
	REFRESH_GLOBAL( SCALE );

	/* Compute Source & Dest Y-Coords */

	ST = M->ST;
	DT = M->DT;

	sy = ST->y1;
	dy = DT->y1;

	if ( sy < dy )
	{
		y1 = ST->y2;
		y2 = dy;
	}

	else
	{
		y1 = sy;
		y2 = DT->y2;
	}

	x1 = X_OF_TIME( M->sendtime.tv_sec, M->sendtime.tv_usec,
		INT_GLOBVAL( SCALE ) );

	CK_X_TIME( x1, M->sendtime.tv_sec, M->sendtime.tv_usec );

	x2 = X_OF_TIME( M->recvtime.tv_sec, M->recvtime.tv_usec,
		INT_GLOBVAL( SCALE ) );

	CK_X_TIME( x2, M->recvtime.tv_sec, M->recvtime.tv_usec );

	new = 0;

	if ( M->line == NULL )
	{
		M->line = create_gobj();

		new++;
	}

	X1_COORD( M->line ) = x1;
	Y1_COORD( M->line ) = y1;

	X2_COORD( M->line ) = x2;
	Y2_COORD( M->line ) = y2;

	if ( !new )
	{
		SET_COORDS( interp, ST_C, M->line );
	}

	else
	{
		sprintf( cmd, "%s create line %d %d %d %d -fill %s",
			ST_C, x1, y1, x2, y2, CHAR_GLOBVAL( ST_COMM_COLOR ) );

		Tcl_Eval( interp, cmd );

		M->line->id = atoi( interp->result );
	}

	sprintf( cmd, "%s raise %d", ST_C, M->line->id );

	Tcl_Eval( interp, cmd );
}


/* ARGSUSED */
int
comm_arrange_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	MSG M;

	M = MSG_LIST;

	while ( M != NULL )
	{
		place_message( M );

		M = M->next;
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
comm_query_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	MSG M;

	char value[1024];
	char cmd[1024];

	char *susec_pad;
	char *rusec_pad;

	int index;

	index = atoi( argv[1] );

	M = MSG_LIST;

	while ( M != NULL )
	{
		if ( M->line != NULL && M->line->id == index )
		{
			/* Highlight Comm Line */

			sprintf( cmd, "%s itemconfigure %d -arrow last -width 3",
				ST_C, index );

			Tcl_Eval( itp, cmd );

			SET_TCL_GLOBAL( itp, "st_highlight_type", "comm" );

			sprintf( value, "%d", index );

			SET_TCL_GLOBAL( itp, "st_highlight_id", value );

			/* Construct Info Message */

			susec_pad = trc_pad_num( M->sendtime.tv_usec, 6 );
			rusec_pad = trc_pad_num( M->recvtime.tv_usec, 6 );

			sprintf( value,
			"%s: from 0x%x to 0x%x, %d.%s to %d.%s, %s=%d %s=%d %s=%d",
				"Message",
				M->src_tid, M->dst_tid,
				M->sendtime.tv_sec, susec_pad,
				M->recvtime.tv_sec, rusec_pad,
				"code", M->msgtag, "ctxt", M->msgctx,
				"nbytes", M->nbytes );

			free( susec_pad );
			free( rusec_pad );

			sprintf( cmd,
			"%s.query_frame.query configure -text {View Info:   %s}",
				ST, value );

			Tcl_Eval( itp, cmd );

			SET_TCL_GLOBAL( itp, "st_query_text_scroll", "0" );

			SET_TCL_GLOBAL( itp, "st_query_text", value );

			Tcl_SetResult( itp, "1", TCL_STATIC );

			return( TCL_OK );
		}

		else
			M = M->next;
	}

	Tcl_SetResult( itp, "0", TCL_STATIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
comm_zoom_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	MSG M;

	char cmd[1024];

	int new_scale;

	new_scale = atoi( argv[1] );

	M = MSG_LIST;

	while ( M != NULL )
	{
		if ( M->line != NULL )
		{
			X1_COORD( M->line ) = X_OF_TIME( M->sendtime.tv_sec,
				M->sendtime.tv_usec, new_scale );

			CK_X_TIME( X1_COORD( M->line ),
				M->sendtime.tv_sec, M->sendtime.tv_usec );

			X2_COORD( M->line ) = X_OF_TIME( M->recvtime.tv_sec,
				M->recvtime.tv_usec, new_scale );

			CK_X_TIME( X2_COORD( M->line ),
				M->recvtime.tv_sec, M->recvtime.tv_usec );

			SET_COORDS( itp, ST_C, M->line );

			sprintf( cmd, "%s raise %d", ST_C, M->line->id );

			Tcl_Eval( itp, cmd );
		}

		M = M->next;
	}

	return( TCL_OK );
}


/* Message Queue Routines */

void
queue_message( M, arrange )
MSG M;
int arrange;
{
	char cmd[1024];

	REFRESH_GLOBAL( MQ_MSG_NOSIZE_COLOR );
	REFRESH_GLOBAL( MQ_MSG_SIZE_COLOR );
	REFRESH_GLOBAL( MQ_OUTLINE_COLOR );
	REFRESH_GLOBAL( MQ_ACTIVE );

	if ( strcmp( CHAR_GLOBVAL( MQ_ACTIVE ), "TRUE" )
		|| TRACE_STATUS == TRACE_FASTFWD
		|| CORRELATE_LOCK == LOCKED )
	{
		NEEDS_REDRAW_MQ = TRUE;

		return;
	}

	if ( M->DT == NULL )
		return;

	/* Create MSGQ */

	make_msgq( M );

	/* Arrange View */

	if ( arrange )
	{
		if ( M->DT->mqbytes > MQ_MAX_NBYTES )
		{
			MQ_MAX_NBYTES = M->DT->mqbytes;

			if ( MQ_MAX_NBYTES < NBYTES_1K )
			{
				sprintf( cmd, "%s.nbytes configure -text {%d}",
					MQ_F, MQ_MAX_NBYTES );
			}

			else if ( MQ_MAX_NBYTES < NBYTES_1M )
			{
				sprintf( cmd, "%s.nbytes configure -text {%dK}",
					MQ_F, MQ_MAX_NBYTES / NBYTES_1K );
			}

			else
			{
				sprintf( cmd, "%s.nbytes configure -text {%dM}",
					MQ_F, MQ_MAX_NBYTES / NBYTES_1M );
			}

			Tcl_Eval( interp, cmd );

			Tcl_Eval( interp, "mqArrange" );
		}

		else
			mqArrangeTask( M->DT );
	}
}


void
make_msgq( M )
MSG M;
{
	MSGQ Q;

	MSGQ tmp;

	char *color;

	tmp = create_msgq();

	tmp->msg = M;

	if ( M->nbytes != -1 )
		color = CHAR_GLOBVAL( MQ_MSG_SIZE_COLOR );

	else
		color = CHAR_GLOBVAL( MQ_MSG_NOSIZE_COLOR );

	CREATE_RECT( interp, MQ_C, tmp->rect, 0, 0, 0, 0,
		color, CHAR_GLOBVAL( MQ_OUTLINE_COLOR ) );

	/* Add to Task List */

	Q = M->DT->msgq;

	if ( Q != NULL )
	{
		while ( Q->next != NULL )
			Q = Q->next;

		Q->next = tmp;

		(M->DT->mqcnt)++;

		M->DT->mqbytes += M->assbytes;
	}

	else
	{
		M->DT->msgq = tmp;

		M->DT->mqcnt = 1;

		M->DT->mqbytes = M->assbytes;
	}
}


void
dequeue_message( M, arrange )
MSG M;
int arrange;
{
	MSGQ Q;
	MSGQ last;

	REFRESH_GLOBAL( MQ_ACTIVE );

	if ( strcmp( CHAR_GLOBVAL( MQ_ACTIVE ), "TRUE" )
		|| TRACE_STATUS == TRACE_FASTFWD
		|| CORRELATE_LOCK == LOCKED )
	{
		NEEDS_REDRAW_MQ = TRUE;

		return;
	}

	if ( M->DT == NULL )
		return;

	Q = M->DT->msgq;

	last = (MSGQ) NULL;

	while ( Q != NULL && Q->msg != M )
	{
		last = Q;

		Q = Q->next;
	}

	if ( Q != NULL )
	{
		if ( Q->rect != NULL )
		{
			DELETE_GOBJ( interp, MQ_C, Q->rect );
		}

		(M->DT->mqcnt)--;

		M->DT->mqbytes -= M->assbytes;

		if ( last != NULL )
			last->next = Q->next;

		else
			M->DT->msgq = Q->next;

		free_msgq( &Q );

		if ( arrange )
			mqArrangeTask( M->DT );
	}
}


/* ARGSUSED */
int
mq_arrange_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	char value[1024];
	char cmd[1024];

	REFRESH_GLOBAL( MQ_TASK_WT );
	REFRESH_GLOBAL( MQ_CWIDTH );
	REFRESH_GLOBAL( MQ_ACTIVE );
	REFRESH_GLOBAL( MQ_XVIEW );

	if ( strcmp( CHAR_GLOBVAL( MQ_ACTIVE ), "TRUE" )
		|| TRACE_STATUS == TRACE_FASTFWD
		|| CORRELATE_LOCK == LOCKED )
	{
		NEEDS_REDRAW_MQ = TRUE;

		return( TCL_OK );
	}

	T = TASK_LIST;

	while ( T != NULL )
	{
		mqArrangeTask( T );

		T = T->next;
	}

	/* Adjust Total Tasks Width */

	sprintf( value, "%d", NTASKS * 2 * INT_GLOBVAL( MQ_TASK_WT ) );

	SET_TCL_GLOBAL( interp, "mq_tasks_width", value );

	sprintf( cmd, "scrollSet %s %s %d %d %d",
		MQ_SBH, value, INT_GLOBVAL( MQ_CWIDTH ),
		INT_GLOBVAL( MQ_TASK_WT ), INT_GLOBVAL( MQ_XVIEW ) );

	Tcl_Eval( interp, cmd );

	return( TCL_OK );
}


void
mqArrangeTask( T )
TASK T;
{
	MSGQ Q;

	char cmd[1024];

	int x1, x2;
	int y1, y2;
	int bytes;
	int bot;
	int ht;

	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FRAME_OFFSET );
	REFRESH_GLOBAL( ROW_HEIGHT );
	REFRESH_GLOBAL( MQ_TASK_WT );
	REFRESH_GLOBAL( MQ_CHEIGHT );
	REFRESH_GLOBAL( MQ_CWIDTH );

	/* Calculate X Coords */

	x1 = ( T->index * 2 * INT_GLOBVAL( MQ_TASK_WT ) )
		+ ( INT_GLOBVAL( MQ_TASK_WT ) / 2 );

	x2 = x1 + INT_GLOBVAL( MQ_TASK_WT );

	ht = INT_GLOBVAL( MQ_CHEIGHT ) - INT_GLOBVAL( ROW_HEIGHT )
		- ( 2 * INT_GLOBVAL( FRAME_OFFSET )
			* INT_GLOBVAL( FRAME_BORDER ) );

	/* Layout Rectangles */

	Q = T->msgq;

	y2 = bot = INT_GLOBVAL( MQ_CHEIGHT ) - 1
		- ( INT_GLOBVAL( FRAME_BORDER )
			* ( 1 + INT_GLOBVAL( FRAME_OFFSET ) ) );

	bytes = 0;

	while ( Q != NULL )
	{
		y1 = y2;

		bytes += Q->msg->assbytes;

		y2 = bot - ( (bytes * ht) / MQ_MAX_NBYTES );

		X1_COORD( Q->rect ) = x1;
		Y1_COORD( Q->rect ) = y1;

		X2_COORD( Q->rect ) = x2;
		Y2_COORD( Q->rect ) = y2;

		SET_COORDS( interp, MQ_C, Q->rect );

		Q = Q->next;
	}

	/* Update Task Marker */

	sprintf( cmd, "%s itemconfigure %d -text {%d}",
		MQ_C, T->mqmark->id, T->mqcnt );

	Tcl_Eval( interp, cmd );

	sprintf( cmd, "%s coords %d %d %d",
		MQ_C, T->mqmark->id,
		x1 + ( (x2 - x1) / 2 ), y2 );

	Tcl_Eval( interp, cmd );
}


void
mqTaskAdd( T )
TASK T;
{
	MSG M;

	char cmd[1024];

	REFRESH_GLOBAL( MQ_ACTIVE );
	REFRESH_GLOBAL( MAIN_FONT );
	REFRESH_GLOBAL( FG_COLOR );

	/* Create Task Marker */

	T->mqmark = create_gobj();

	sprintf( cmd,
		"%s create text 0 -100 -text {0} %s -fill {%s} -font {%s} %s",
		MQ_C, "-anchor s", CHAR_GLOBVAL( FG_COLOR ),
		CHAR_GLOBVAL( MAIN_FONT ), "-justify center" );

	Tcl_Eval( interp, cmd );

	T->mqmark->id = atoi( interp->result );

	T->mqcnt = 0;

	T->mqbytes = 0;

	if ( !strcmp( CHAR_GLOBVAL( MQ_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD
		&& CORRELATE_LOCK != LOCKED )
	{
		/* Get Message Queue Info */

		M = PENDING_MSG_LIST;

		while ( M != NULL )
		{
			if ( M->dst_tid == T->tid )
				queue_message( M, FALSE );

			M = M->next;
		}

		/* Arrange the stuff... */

		Tcl_Eval( interp, "mqArrange" );
	}

	else
		NEEDS_REDRAW_MQ = TRUE;
}


/* ARGSUSED */
int
mq_update_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	MSGQ Q;
	MSGQ Qnext;

	MSG M;

	char cmd[1024];

	char *str;

	int mqbytes;
	int save;

	if ( NEEDS_REDRAW_MQ )
	{
		LOCK_INTERFACE( save );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Message Queue... }" );

		if ( CORRELATE_LOCK == LOCKED )
		{
			UNLOCK_INTERFACE( save );

			return( TCL_OK );
		}

		/* Clear Out Any Left-Over MSGQs */

		T = TASK_LIST;

		while ( T != NULL )
		{
			Q = T->msgq;

			T->msgq = (MSGQ) NULL;

			T->mqcnt = 0;

			T->mqbytes = 0;

			while ( Q != NULL )
			{
				if ( Q->rect != NULL )
				{
					DELETE_GOBJ( itp, MQ_C, Q->rect );
				}

				Qnext = Q->next;

				free_msgq( &Q );

				Q = Qnext;
			}

			T = T->next;
		}

		/* Add Current MSGQs */

		M = PENDING_MSG_LIST;

		mqbytes = 0;

		while ( M != NULL )
		{
			if ( M->DT != NULL
				&& M->recvtime.tv_sec == -1
				&& M->recvtime.tv_usec == -1 )
			{
				queue_message( M, FALSE );

				if ( M->DT->mqbytes > mqbytes )
					mqbytes = M->DT->mqbytes;
			}

			M = M->next;
		}

		/* Set MQ_MAX_NBYTES */

		MQ_MAX_NBYTES = mqbytes;

		MQ_MAX_NBYTES = MQ_MAX_NBYTES > 0 ? MQ_MAX_NBYTES : 1;

		if ( MQ_MAX_NBYTES < NBYTES_1K )
		{
			sprintf( cmd, "%s.nbytes configure -text {%d}",
				MQ_F, MQ_MAX_NBYTES );
		}

		else if ( MQ_MAX_NBYTES < NBYTES_1M )
		{
			sprintf( cmd, "%s.nbytes configure -text {%dK}",
				MQ_F, MQ_MAX_NBYTES / NBYTES_1K );
		}

		else
		{
			sprintf( cmd, "%s.nbytes configure -text {%dM}",
				MQ_F, MQ_MAX_NBYTES / NBYTES_1M );
		}

		Tcl_Eval( interp, cmd );

		/* Arrange the stuff... */

		Tcl_Eval( itp, "mqArrange" );

		/* Done Message */

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Message Queue... Done. }" );

		/* No Longer Needs Redraw */

		NEEDS_REDRAW_MQ = FALSE;

		UNLOCK_INTERFACE( save );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
mq_query_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	MSGQ Q;

	char value[1024];
	char cmd[1024];

	char *query_entry;
	char *usec_pad;
	char *label;

	int tindex;
	int x1, x2;
	int y1, y2;
	int bytes;
	int x, y;
	int bot;
	int ht;

	REFRESH_GLOBAL( MQ_TASK_WT );

	query_entry = trc_copy_str( argv[1] );

	x = atoi( argv[2] );
	y = atoi( argv[3] );

	/* Find Task */

	tindex = x / ( 2 * INT_GLOBVAL( MQ_TASK_WT ) );

	T = TASK_LIST;

	while ( T != NULL && T->index != tindex )
		T = T->next;

	if ( T == NULL )
	{
		Tcl_SetResult( itp, "0 none {}", TCL_STATIC );

		return( TCL_OK );
	}

	if ( !strcmp( query_entry, "query" ) )
	{
		x1 = ( tindex * 2 * INT_GLOBVAL( MQ_TASK_WT ) )
			+ ( INT_GLOBVAL( MQ_TASK_WT ) / 2 );

		x2 = x1 + INT_GLOBVAL( MQ_TASK_WT );

		/* Check for Message Rectangle */

		if ( x1 <= x && x <= x2 )
		{
			ht = INT_GLOBVAL( MQ_CHEIGHT ) - INT_GLOBVAL( ROW_HEIGHT )
				- ( 2 * INT_GLOBVAL( FRAME_OFFSET )
					* INT_GLOBVAL( FRAME_BORDER ) );

			y2 = bot = INT_GLOBVAL( MQ_CHEIGHT ) - 1
				- ( INT_GLOBVAL( FRAME_BORDER )
					* ( 1 + INT_GLOBVAL( FRAME_OFFSET ) ) );

			bytes = 0;

			Q = T->msgq;

			while ( Q != NULL )
			{
				y1 = y2;

				bytes += Q->msg->assbytes;

				y2 = bot - ( (bytes * ht) / MQ_MAX_NBYTES );

				if ( Q->rect != NULL && y2 <= y && y <= y1 )
				{
					/* Set Return String */

					usec_pad =
						trc_pad_num( Q->msg->sendtime.tv_usec, 6 );

					if ( Q->msg->nbytes != -1 )
					{
						sprintf( value,
					"%d %s \"From 0x%x %s=%d %s=%d %s=%d at %d.%s\"",
							Q->rect->id, "rect",
							Q->msg->src_tid,
							"code", Q->msg->msgtag,
							"ctxt", Q->msg->msgctx,
							"nbytes", Q->msg->nbytes,
							Q->msg->sendtime.tv_sec, usec_pad );
					}

					else
					{
						sprintf( value,
				"%d %s \"From 0x%x %s=%d %s=%d (%s ~= %d) at %d.%s\"",
							Q->rect->id, "rect",
							Q->msg->src_tid,
							"code", Q->msg->msgtag,
							"ctxt", Q->msg->msgctx,
							"nbytes", Q->msg->assbytes,
							Q->msg->sendtime.tv_sec, usec_pad );
					}

					free( usec_pad );

					Tcl_SetResult( itp, trc_copy_str( value ),
						TCL_DYNAMIC );

					return( TCL_OK );
				}

				Q = Q->next;
			}
		}
	}

	/* Just Entry or Not Inside Message Rectangle */
	/* Clear Item Index and Return Task Info */

	label = task_label_str( T );

	sprintf( value, "%d text {%s}", T->mqmark->id, label );

	free( label );

	Tcl_SetResult( itp, trc_copy_str( value ), TCL_DYNAMIC );

	free( query_entry );

	return( TCL_OK );
}


void
redraw_messagequeue()
{
	int save;

	REFRESH_GLOBAL( MQ_ACTIVE );

	save = TRACE_STATUS;

	TRACE_STATUS = TRACE_STOP;

	if ( !strcmp( CHAR_GLOBVAL( MQ_ACTIVE ), "TRUE" ) )
		Tcl_Eval( interp, "mqUpdate" );
	
	TRACE_STATUS = save;
}


/* ARGSUSED */
int
mq_reset_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	MSGQ Q;

	char cmd[1024];

	REFRESH_GLOBAL( MQ_TASK_WT );
	REFRESH_GLOBAL( MQ_CWIDTH );
	REFRESH_GLOBAL( MQ_XVIEW );

	T = TASK_LIST;

	while ( T != NULL )
	{
		/* Delete Marker */

		DELETE_GOBJ( itp, MQ_C, T->mqmark );

		/* Delete Message Rectangles */

		Q = T->msgq;

		while ( Q != NULL )
		{
			if ( Q->rect != NULL )
			{
				DELETE_GOBJ( itp, MQ_C, Q->rect );
			}

			Q = Q->next;
		}

		T->mqcnt = 0;

		T->mqbytes = 0;

		T = T->next;
	}

	/* Reset View Constants */

	MQ_MAX_NBYTES = 1;

	sprintf( cmd, "%s.nbytes configure -text {%d}",
		MQ_F, MQ_MAX_NBYTES );

	Tcl_Eval( interp, cmd );

	SET_TCL_GLOBAL( interp, "mq_tasks_width", "1" );

	sprintf( cmd, "scrollSet %s 1 %d %d %d",
		MQ_SBH, INT_GLOBVAL( MQ_CWIDTH ), INT_GLOBVAL( MQ_TASK_WT ),
		INT_GLOBVAL( MQ_XVIEW ) );

	Tcl_Eval( interp, cmd );

	return( TCL_OK );
}


void
msg_grid_add_task( T )
TASK T;
{
	MSG_GRID **old_grid;

	MSG_GRID G;

	MSG M;

	int old_size;
	int i, j;

	if ( NTASKS > MSG_GRID_SIZE )
	{
		old_grid = PENDING_MSG_GRID;

		old_size = MSG_GRID_SIZE;
		
		MSG_GRID_SIZE += 16;

		PENDING_MSG_GRID = (MSG_GRID **) malloc(
			(unsigned) MSG_GRID_SIZE * sizeof( MSG_GRID * ) );
		trc_memcheck( PENDING_MSG_GRID, "Pending Message Grid Ptrs" );

		for ( i=0 ; i < MSG_GRID_SIZE ; i++ )
		{
			PENDING_MSG_GRID[ i ] = (MSG_GRID *) malloc(
				(unsigned) MSG_GRID_SIZE * sizeof( MSG_GRID ) );
			trc_memcheck( PENDING_MSG_GRID,
				"Pending Message Grid Line" );
		}

		/* Copy Unchanged Grid Entries */

		for ( i=0 ; i < T->index ; i++ )
		{
			for ( j=0 ; j < T->index ; j++ )
			{
				PENDING_MSG_GRID[ i ][ j ] = old_grid[ i ][ j ];
			}
		}

		/* Copy Over Lateral Shifts */

		for ( i=(NTASKS - 1) ; i > T->index ; i-- )
		{
			for ( j=0 ; j < T->index ; j++ )
			{
				/* Down */

				PENDING_MSG_GRID[ i ][ j ] = old_grid[ i - 1 ][ j ];

				/* Across */

				PENDING_MSG_GRID[ j ][ i ] = old_grid[ j ][ i - 1 ];
			}
		}

		/* Copy Over Diagnonal Shifts */

		for ( i=(NTASKS - 1) ; i > T->index ; i-- )
		{
			for ( j=(NTASKS - 1) ; j > T->index ; j-- )
			{
				PENDING_MSG_GRID[ i ][ j ] = old_grid[ i - 1 ][ j - 1 ];
			}
		}

		/* Free Old Grid */

		if ( old_grid != NULL )
		{
			for ( i=0 ; i < old_size ; i++ )
				free( old_grid[ i ] );

			free( old_grid );
		}
	}

	else
	{
		/* Lateral Shifts */

		for ( i=(NTASKS - 1) ; i > T->index ; i-- )
		{
			for ( j=0 ; j < T->index ; j++ )
			{
				/* Down */

				PENDING_MSG_GRID[ i ][ j ] =
					PENDING_MSG_GRID[ i - 1 ][ j ];

				/* Across */

				PENDING_MSG_GRID[ j ][ i ] =
					PENDING_MSG_GRID[ j ][ i - 1 ];
			}
		}

		/* Diagnonal Shifts */

		for ( i=(NTASKS - 1) ; i > T->index ; i-- )
		{
			for ( j=(NTASKS - 1) ; j > T->index ; j-- )
			{
				PENDING_MSG_GRID[ i ][ j ] =
					PENDING_MSG_GRID[ i - 1 ][ j - 1 ];
			}
		}
	}

	/* Clear New Task Row & Column */

	for ( i=0 ; i < NTASKS ; i++ )
	{
		PENDING_MSG_GRID[ i ][ T->index ] = (MSG_GRID) NULL;

		PENDING_MSG_GRID[ T->index ][ i ] = (MSG_GRID) NULL;
	}

	/* Add In Any Pending Messages */

	M = PENDING_MSG_LIST;

	while ( M != NULL )
	{
		if ( M->src_tid == T->tid )
		{
			M->ST = T;

			msg_grid_add( M );
		}

		else if ( M->dst_tid == T->tid )
		{
			M->DT = T;

			msg_grid_add( M );
		}

		M = M->next;
	}
}


void
msg_grid_sort_tasks()
{
	MSG_GRID Gnext;
	MSG_GRID G;

	MSG M;

	int i, j;

	/* Free Old Grid Cells */

	for ( i=0 ; i < NTASKS ; i++ )
	{
		for ( j=0 ; j < NTASKS ; j++ )
		{
			if ( PENDING_MSG_GRID[ i ][ j ] != NULL )
			{
				G = PENDING_MSG_GRID[ i ][ j ];

				PENDING_MSG_GRID[ i ][ j ] = (MSG_GRID) NULL;

				while ( G != NULL )
				{
					Gnext = G->next;

					free_msg_grid( &G );

					G = Gnext;
				}
			}
		}
	}

	/* Add Back Pending Messages, Using New Indexing */

	M = PENDING_MSG_LIST;

	while ( M != NULL )
	{
		msg_grid_add( M );

		M = M->next;
	}
}


void
msg_grid_swap_task( T, oldindex )
TASK T;
int oldindex;
{
	MSG_GRID tmp;

	int i, j;
	int sgn;

	sgn = ( T->index > oldindex ) ? 1 : -1;

	/* Swap Horizontal */

	for ( i=0 ; i < NTASKS ; i++ )
	{
		tmp = PENDING_MSG_GRID[ i ][ oldindex ];

		for ( j=oldindex ; j != T->index ; j += sgn )
		{
			PENDING_MSG_GRID[ i ][ j ] =
				PENDING_MSG_GRID[ i ][ j + sgn ];
		}

		PENDING_MSG_GRID[ i ][ T->index ] = tmp;
	}

	/* Swap Vertical */

	for ( j=0 ; j < NTASKS ; j++ )
	{
		tmp = PENDING_MSG_GRID[ oldindex ][ j ];

		for ( i=oldindex ; i != T->index ; i += sgn )
		{
			PENDING_MSG_GRID[ i ][ j ] =
				PENDING_MSG_GRID[ i + sgn ][ j ];
		}

		PENDING_MSG_GRID[ T->index ][ j ] = tmp;
	}
}


void
msg_grid_add( M )
MSG M;
{
	MSG_GRID G;

	if ( M->ST == NULL || M->DT == NULL )
		return;

	G = PENDING_MSG_GRID[ M->ST->index ][ M->DT->index ];

	if ( G == NULL )
	{
		G = PENDING_MSG_GRID[ M->ST->index ][ M->DT->index ] =
			create_msg_grid();
	}

	else
	{
		while ( G->next != NULL )
			G = G->next;

		G->next = create_msg_grid();

		G = G->next;
	}

	G->msg = M;
}


void
msg_grid_del( M )
MSG M;
{
	MSG_GRID G;
	MSG_GRID last;

	if ( M->ST == NULL || M->DT == NULL )
		return;

	G = PENDING_MSG_GRID[ M->ST->index ][ M->DT->index ];

	last = (MSG_GRID) NULL;

	while ( G != NULL )
	{
		if ( G->msg == M )
		{
			if ( last != NULL )
				last->next = G->next;
			
			else
			{
				PENDING_MSG_GRID[ M->ST->index ][ M->DT->index ] =
					G->next;
			}

			free_msg_grid( &G );

			return;
		}

		last = G;

		G = G->next;
	}
}

