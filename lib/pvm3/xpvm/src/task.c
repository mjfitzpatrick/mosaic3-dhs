
static char rcsid[] = 
	"$Id: task.c,v 4.50 1998/04/09 21:11:53 kohl Exp $";

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


int
taskAdd( T, sec, usec, str )
TASK T;
int sec;
int usec;
char *str;
{
	STATE S;

	char cmd[1024];

	int draw;
	int x;

	REFRESH_GLOBAL( ST_OUTLINE_COLOR );
	REFRESH_GLOBAL( ST_RUNNING_COLOR );
	REFRESH_GLOBAL( ST_ACTIVE );
	REFRESH_GLOBAL( MAIN_FONT );
	REFRESH_GLOBAL( FG_COLOR );
	REFRESH_GLOBAL( SCALE );

	if ( T->host == NULL )
	{
		printf( "\nError: Missing Host for pvmd_tid=0x%x\n\n",
			T->pvmd_tid );

		free_task( &T );

		return( TCL_OK );
	}

	/* Update Host Icon State */

	netHandle( T->host, STATE_NONE, STATE_RUNNING );

	/* No Utilization View Update Here */
	/* (only call at end of states...) */

	/* Create Space-Time Task Label */

	sprintf( cmd,
		"%s create text 0 -100 -text {%s:%s} -fill {%s} -font {%s} %s",
		ST_L, host_name( T->host, FALSE ), T->name,
		CHAR_GLOBVAL( FG_COLOR ), CHAR_GLOBVAL( MAIN_FONT ),
		"-anchor w" );

	Tcl_Eval( interp, cmd );

	T->st_label = create_gobj();

	T->st_label->id = atoi( interp->result );

	/* Create First Space-Time Task State */

	draw = ( !strcmp( CHAR_GLOBVAL( ST_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD );

	if ( !draw )
		NEEDS_REDRAW_ST = TRUE;

	S = create_state();

	S->status = STATE_RUNNING;

	S->user_defined = 0;

	S->callstr = trc_copy_str( str );

	if ( draw )
	{
		x = X_OF_TIME( sec, usec, INT_GLOBVAL( SCALE ) );

		CK_X_TIME( x, sec, usec );

		CREATE_RECT( interp, ST_C, S->rect, x, 0, x, 0,
			CHAR_GLOBVAL( ST_RUNNING_COLOR ),
			CHAR_GLOBVAL( ST_OUTLINE_COLOR ) );
	}

	S->starttime.tv_sec = sec;
	S->starttime.tv_usec = usec;

	S->endtime.tv_sec = sec;
	S->endtime.tv_usec = usec;

	T->states = S;

	/* Create Call Trace Task List Elements */

	sprintf( cmd,
		"%s create text 0 -100 -text {%s:%s} -fill {%s} -font {%s} %s",
		CT_L, host_name( T->host, FALSE ), T->name,
		CHAR_GLOBVAL( FG_COLOR ), CHAR_GLOBVAL( MAIN_FONT ),
		"-anchor w" );

	Tcl_Eval( interp, cmd );

	T->ct_label = create_gobj();

	T->ct_label->id = atoi( interp->result );

	sprintf( cmd,
		"%s create text 0 -100 -text {%s} -fill {%s} -font {%s} %s",
		CT_C, str, CHAR_GLOBVAL( FG_COLOR ), CHAR_GLOBVAL( MAIN_FONT ),
		"-anchor w" );

	Tcl_Eval( interp, cmd );

	T->ct_callstr = create_gobj();

	T->ct_callstr->id = atoi( interp->result );

	/* Add Event String to Trace History */

	task_tevhist( T->tid, sec, usec, str );

	/* Add Task to List */

	insert_task( T );

	if ( draw )
		Tcl_Eval( interp, "taskArrange" );

	/* Re-Allocate Pending Message Grid */

	msg_grid_add_task( T );

	/* Re-Arrange Message Queue View */

	mqTaskAdd( T );

	return( TCL_OK );
}


void
insert_task( T )
TASK T;
{
	TASK tptr;
	TASK last;

	char Tmp[1024];
	char tmp[1024];

	int first;
	int index;
	int cmp;

	/* Generate Base Alpha String */

	if ( TASK_SORT == TASK_SORT_ALPHA )
		sprintf( Tmp, "%s:%s", T->host->name, T->name );

	/* Order New Task in List */

	tptr = TASK_LIST;

	last = (TASK) NULL;

	first = TRUE;

	index = 0;

	do
	{
		if ( first )
			first = FALSE;
		
		else
		{
			index++;

			last = tptr;

			tptr = tptr->next;
		}

		if ( tptr != NULL )
		{
			switch ( TASK_SORT )
			{
				case TASK_SORT_ALPHA:
				{
					sprintf( tmp, "%s:%s",
						tptr->host->name, tptr->name );

					cmp = strcmp( tmp, Tmp );

					cmp = cmp ? cmp : tptr->tid - T->tid;

					break;
				}

				case TASK_SORT_TID:
				{
					cmp = tptr->tid - T->tid;

					break;
				}

				case TASK_SORT_CUSTOM:
				{
					cmp = -1;

					break;
				}

				default:
					cmp = -1;
			}
		}
	}
	while ( tptr != NULL && cmp < 0 );

	if ( tptr != NULL )
	{
		T->next = tptr;

		if ( last != NULL )
			last->next = T;
		
		else
			TASK_LIST = T;

		T->index = index;

		/* Adjust remaining Task indices */

		tptr = T->next;

		while ( tptr != NULL )
		{
			(tptr->index)++;

			tptr = tptr->next;
		}
	}

	else
	{
		if ( last != NULL )
			last->next = T;

		else
			TASK_LIST = T;

		T->index = index;
	}

	NTASKS++;
}


void
sort_tasks()
{
	TASK tptr, tnext;
	TASK last;

	char tmp[1024], tmp2[1024];

	int changed;
	int index;
	int cmp;

	/* Check for Custom Sort */

	if ( TASK_SORT == TASK_SORT_CUSTOM )
		return;

	/* Process List Until Sorted */

	do
	{
		/* Reset Changed Flag */

		changed = 0;

		/* Order Each Task in List */

		tptr = TASK_LIST;

		if ( tptr != NULL )
			tnext = tptr->next;

		last = (TASK) NULL;

		while ( tptr != NULL )
		{
			if ( tnext != NULL )
			{
				switch ( TASK_SORT )
				{
					case TASK_SORT_ALPHA:
					{
						sprintf( tmp, "%s:%s",
							tptr->host->name, tptr->name );

						sprintf( tmp2, "%s:%s",
							tnext->host->name, tnext->name );

						cmp = strcmp( tmp, tmp2 );

						cmp = cmp ? cmp : tptr->tid - tnext->tid;

						break;
					}

					case TASK_SORT_TID:
					{
						cmp = tptr->tid - tnext->tid;

						break;
					}

					default:
						cmp = -1;
				}

				if ( cmp > 0 )
				{
					/* Swap Tasks */

					tptr->next = tnext->next;

					tnext->next = tptr;

					if ( last != NULL )
						last->next = tnext;
					
					else
						TASK_LIST = tnext;

					/* Swap Task Indices */

					index = tnext->index;
					tnext->index = tptr->index;
					tptr->index = index;

					changed++;
				}
			}

			last = tptr;

			tptr = tnext;

			if ( tptr != NULL )
				tnext = tptr->next;
		}
	}
	while ( changed );

	/* Fix Pending Message Grid */

	msg_grid_sort_tasks();

	/* Update Message Queue View */

	Tcl_Eval( interp, "mqArrange" );
}


void
swap_tasks( src, dst )
int src;
int dst;
{
	TASK tsrc, srclast;
	TASK tdst, dstlast;
	TASK tptr;

	int index;
	int save;

	/* Base Sanity Check */

	if ( src < 0 || dst < 0 )
		return;

	/* Protect Our Butts */

	LOCK_INTERFACE( save );

	Tcl_Eval( interp, "setMsg { Swapping Task Order... }" );

	/* Find Source Task - Remove From List */

	tsrc = TASK_LIST;

	srclast = (TASK) NULL;

	while ( tsrc != NULL && tsrc->index != src )
	{
		srclast = tsrc;

		tsrc = tsrc->next;
	}

	if ( tsrc == NULL || ( dst > tsrc->index && tsrc->next == NULL ) )
	{
		Tcl_Eval( interp, "setMsg { No Task Order Swap Necessary. }" );

		UNLOCK_INTERFACE( save );

		return;
	}

	if ( srclast != NULL )
		srclast->next = tsrc->next;

	else
		TASK_LIST = tsrc->next;

	/* Find Destination Task */

	tdst = TASK_LIST;

	dstlast = (TASK) NULL;

	while ( tdst != NULL && tdst->index != dst )
	{
		dstlast = tdst;

		tdst = tdst->next;
	}

	if ( src > dst )
	{
		if ( dstlast != NULL )
			dstlast->next = tsrc;

		else
			TASK_LIST = tsrc;
	
		tsrc->next = tdst;
	}

	else
	{
		if ( tdst != NULL )
		{
			tsrc->next = tdst->next;
		
			tdst->next = tsrc;
		}

		else
		{
			if ( dstlast != NULL )
				dstlast->next = tsrc;
			
			else
				TASK_LIST = tsrc;

			tsrc->next = (TASK) NULL;
		}
	}

	/* Re-Index Tasks */

	tptr = TASK_LIST;

	index = 0;

	while ( tptr != NULL )
	{
		tptr->index = index++;

		tptr = tptr->next;
	}

	/* Re-Arrange Tasks */

	Tcl_Eval( interp, "taskArrange" );

	/* Fix Pending Message Grid */

	msg_grid_swap_task( tsrc, src );

	/* Update Message Queue View */

	Tcl_Eval( interp, "mqArrange" );

	/* Restore Lock */

	Tcl_Eval( interp, "setMsg { Task Order Swapped. }" );

	UNLOCK_INTERFACE( save );
}


void
taskState( T, sec, usec, status, call_str )
TASK T;
int sec;
int usec;
int status;
char *call_str;
{
	STATE S;
	STATE stmp;
	STATE slast;

	char value[1024];
	char cmd[1024];

	char *usec_pad;

	char *color;

	int user_defined = 0;
	int xrange;
	int x2;

	REFRESH_GLOBAL( ST_USER_DEFINED_COLOR );
	REFRESH_GLOBAL( ST_OUTLINE_COLOR );
	REFRESH_GLOBAL( ST_RUNNING_COLOR );
	REFRESH_GLOBAL( ST_SYSTEM_COLOR );
	REFRESH_GLOBAL( CT_QUERY_INDEX );
	REFRESH_GLOBAL( ST_IDLE_COLOR );
	REFRESH_GLOBAL( CT_ACTIVE );
	REFRESH_GLOBAL( ST_ACTIVE );
	REFRESH_GLOBAL( SCALE );
	REFRESH_GLOBAL( TIMEX );

	/* Update Call String for Task */

	if ( !strcmp( CHAR_GLOBVAL( CT_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD
		&& CORRELATE_LOCK != LOCKED )
	{
		sprintf( cmd, "%s itemconfigure %d -text {%s}",
			CT_C, T->ct_callstr->id, call_str );

		Tcl_Eval( interp, cmd );

		if ( INT_GLOBVAL( CT_QUERY_INDEX ) == T->index )
		{
			usec_pad = trc_pad_num( usec, 6 );

			sprintf( value, "Time Stamp: %d.%s", sec, usec_pad );

			free( usec_pad );

			sprintf( cmd, "%s.query configure -text {%s}",
				CT_F, value );

			Tcl_Eval( interp, cmd );
		}
	}

	else
		NEEDS_REDRAW_CT = TRUE;

	/* Add Event String to Trace History */

	task_tevhist( T->tid, sec, usec, call_str );

	/* Check for User Defined Event */

	if ( status == STATE_USER_DEFINED )
		user_defined++;

	/* Check for Task State Change */

	S = T->states;

	if ( S != NULL )
	{
		if ( S->status != status && !user_defined )
		{
			/* Update Host Icon State */

			netHandle( T->host, S->status, status );

			/* Update Utilization View Counts */

			utHandle( S->status,
				S->starttime.tv_sec, S->starttime.tv_usec, sec, usec );
		}

		if ( user_defined )
		{
			slast = (STATE) NULL;

			while ( S != NULL &&
				( TIME_COMPARE( sec, usec,
					S->starttime.tv_sec, S->starttime.tv_usec ) < 0 ) )
			{
				slast = S;

				S = S->next;
			}
		}

		/* Set End of Last State */

		S->endtime.tv_sec = sec;
		S->endtime.tv_usec = usec;

		/* Create New State */

		stmp = create_state();

		stmp->status = ( user_defined ) ? S->status : status;

		stmp->user_defined = user_defined;

		stmp->callstr = trc_copy_str( call_str );

		stmp->starttime.tv_sec = sec;
		stmp->starttime.tv_usec = usec;

		if ( user_defined && slast != NULL )
		{
			stmp->endtime.tv_sec = slast->starttime.tv_sec;
			stmp->endtime.tv_usec = slast->starttime.tv_usec;

			slast->next = stmp;
		}

		else
		{
			stmp->endtime.tv_sec = CURRENT_TIME.tv_sec;
			stmp->endtime.tv_usec = CURRENT_TIME.tv_usec;

			T->states = stmp;
		}

		stmp->next = S;

		if ( !strcmp( CHAR_GLOBVAL( ST_ACTIVE ), "TRUE" )
			&& TRACE_STATUS != TRACE_FASTFWD )
		{
			if ( S->rect != NULL )
			{
				/* Determine Color for New State */

				if ( user_defined )
					color = CHAR_GLOBVAL( ST_USER_DEFINED_COLOR );

				else if ( status == STATE_RUNNING )
					color = CHAR_GLOBVAL( ST_RUNNING_COLOR );

				else if ( status == STATE_SYSTEM )
					color = CHAR_GLOBVAL( ST_SYSTEM_COLOR );

				else if ( status == STATE_IDLE )
					color = CHAR_GLOBVAL( ST_IDLE_COLOR );

				else
					color = CHAR_GLOBVAL( ST_OUTLINE_COLOR );

				/* Create / Steal New State Rectangle */

				x2 = X_OF_TIME( sec, usec, INT_GLOBVAL( SCALE ) );

				CK_X_TIME( x2, sec, usec );

				xrange = x2 - X1_COORD( S->rect );

				/* Steal It, Color It */

				if ( !xrange
					|| ( xrange == 1
						&& S->next != NULL && S->next->rect != NULL ) )
				{
					stmp->rect = S->rect;

					S->rect = (GOBJ) NULL;
			
					sprintf( cmd, "%s itemconfigure %d -fill %s",
						ST_C, stmp->rect->id, color );

					Tcl_Eval( interp, cmd );
				}

				/* Make a New One (Finish Off Old One) */

				else
				{
					X2_COORD( S->rect ) = x2;

					SET_COORDS( interp, ST_C, S->rect );

					CREATE_RECT( interp, ST_C, stmp->rect,
						X2_COORD( S->rect ), Y1_COORD( S->rect ),
						INT_GLOBVAL( TIMEX ), Y2_COORD( S->rect ),
						color, CHAR_GLOBVAL( ST_OUTLINE_COLOR ) );

					/* Move new state rectangle below comm lines */

					sprintf( cmd, "%s lower %d", ST_C, stmp->rect->id );

					Tcl_Eval( interp, cmd );
				}
			}
		}

		else
			NEEDS_REDRAW_ST = TRUE;
	}
}


void
taskExit( T, sec, usec, str )
TASK T;
int sec;
int usec;
char *str;
{
	STATE S;

	char value[1024];
	char cmd[1024];

	char *usec_pad;

	int queued;

	if ( tflag )
		return;

	REFRESH_GLOBAL( CT_QUERY_INDEX );
	REFRESH_GLOBAL( CT_ACTIVE );
	REFRESH_GLOBAL( ST_ACTIVE );
	REFRESH_GLOBAL( SCALE );

	/* Update Call String for Task */

	if ( !strcmp( CHAR_GLOBVAL( CT_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD
		&& CORRELATE_LOCK != LOCKED )
	{
		sprintf( cmd, "%s itemconfigure %d -text {%s}",
			CT_C, T->ct_callstr->id, str );

		Tcl_Eval( interp, cmd );

		if ( INT_GLOBVAL( CT_QUERY_INDEX ) == T->index )
		{
			usec_pad = trc_pad_num( usec, 6 );

			sprintf( value, "Time Stamp: %d.%s", sec, usec_pad );

			free( usec_pad );

			sprintf( cmd, "%s.query configure -text {%s}",
				CT_F, value );

			Tcl_Eval( interp, cmd );
		}
	}

	else
		NEEDS_REDRAW_CT = TRUE;

	/* Update Task Space-Time State */

	S = T->states;

	if ( S != NULL )
	{
		/* Update Host Icon State */

		netHandle( T->host, S->status, STATE_DEAD );

		/* Update Utilization View Counts */

		utHandle( S->status,
			S->starttime.tv_sec, S->starttime.tv_usec, sec, usec );

		/* Finish Off Space-Time State Rectangle */

		if ( !strcmp( CHAR_GLOBVAL( ST_ACTIVE ), "TRUE" )
			&& TRACE_STATUS != TRACE_FASTFWD )
		{
			X2_COORD( S->rect ) =
				X_OF_TIME( sec, usec, INT_GLOBVAL( SCALE ) );

			CK_X_TIME( X2_COORD( S->rect ), sec, usec );

			SET_COORDS( interp, ST_C, S->rect );
		}

		else
			NEEDS_REDRAW_ST = TRUE;

		S->status = STATE_DEAD;

		S->endtime.tv_sec = sec;
		S->endtime.tv_usec = usec;
	}

	/* Set Task State */

	T->status = TASK_DEAD;

	/* Check for Dead Hosts to Set Free */

	check_for_host_dead( T );
}


/* ARGSUSED */
int
set_task_sort_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char cmd[255];

	char *sort;
	char *str;

	int save;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: set_task_sort [ alpha | tid | custom ]",
			TCL_STATIC );
		
		return( TCL_ERROR );
	}

	LOCK_INTERFACE( save );

	sort = argv[1];

	/* Set Task Sorting Selection */

	if ( !strcmp( sort, "alpha" ) )
	{
		TASK_SORT = TASK_SORT_ALPHA;

		str = "Alpha";
	}
	
	else if ( !strcmp( sort, "tid" ) )
	{
		TASK_SORT = TASK_SORT_TID;

		str = "TID";
	}

	else if ( !strcmp( sort, "custom" ) )
	{
		TASK_SORT = TASK_SORT_CUSTOM;

		str = "Custom";
	}

	else
	{
		printf( "\nWarning:  Unknown Task Sort Type %s\n\n", sort );

		TASK_SORT = TASK_SORT_ALPHA;

		str = "Alpha";
	}

	/* If New Sort Needs Re-Arrangement, Do It */

	if ( TASK_SORT != TASK_SORT_CUSTOM )
	{
		sprintf( cmd, "setMsg { Re-Arranging Tasks for %s Sort... }",
			str );

		Tcl_Eval( itp, cmd );

		/* Fix Task List Order */

		sort_tasks();

		/* Re-Arrange Tasks */

		Tcl_Eval( interp, "taskArrange" );
	}

	sprintf( cmd, "setMsg { Task Sorting Set to %s }", str );

	Tcl_Eval( itp, cmd );

	UNLOCK_INTERFACE( save );
	
	return( TCL_OK );
}


/* ARGSUSED */
int
task_arrange_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	STATE S;

	char value[1024];
	char cmd[1024];

	int delta_y;
	int tx, ty;
	int ly;
	int i;

	REFRESH_GLOBAL( BORDER_SPACE );
	REFRESH_GLOBAL( ST_LABEL_HT );
	REFRESH_GLOBAL( ST_CHEIGHT );
	REFRESH_GLOBAL( CT_CHEIGHT );
	REFRESH_GLOBAL( ST_RECT_HT );
	REFRESH_GLOBAL( STL_YVIEW );
	REFRESH_GLOBAL( CT_YVIEW );

	delta_y = ( INT_GLOBVAL( ST_LABEL_HT )
		- INT_GLOBVAL( ST_RECT_HT ) ) / 2;

	tx = ty = INT_GLOBVAL( BORDER_SPACE );

	T = TASK_LIST;

	while ( T != NULL )
	{
		ly = ty + ( INT_GLOBVAL( ST_LABEL_HT ) / 2 );

		/* Space-Time Label */

		sprintf( cmd, "%s coords %d %d %d",
			ST_L, T->st_label->id, tx, ly );

		Tcl_Eval( itp, cmd );

		/* Call Trace Label / Call String */

		sprintf( cmd, "%s coords %d %d %d",
			CT_L, T->ct_label->id, tx, ly );

		Tcl_Eval( itp, cmd );

		sprintf( cmd, "%s coords %d %d %d",
			CT_C, T->ct_callstr->id, tx, ly );

		Tcl_Eval( itp, cmd );

		/* Space-Time State Rectangles */

		T->y1 = ty + delta_y;
		T->y2 = T->y1 + INT_GLOBVAL( ST_RECT_HT );

		S = T->states;

		while ( S != NULL )
		{
			if ( S->rect != NULL )
			{
				Y1_COORD( S->rect ) = T->y1;
				Y2_COORD( S->rect ) = T->y2;

				SET_COORDS( itp, ST_C, S->rect );
			}

			S = S->next;
		}

		ty += INT_GLOBVAL( ST_LABEL_HT );

		T = T->next;
	}

	sprintf( value, "%d", ty + INT_GLOBVAL( BORDER_SPACE ) );

	SET_TCL_GLOBAL( itp, "tasks_height", value );

	Tcl_Eval( itp, "commArrange" );

	/* Set Scrollbars for Space-Time and Call Trace */

	sprintf( cmd, "scrollSet %s %s %d %d %d",
		ST_SBV, value, INT_GLOBVAL( ST_CHEIGHT ),
		INT_GLOBVAL( ST_LABEL_HT ), INT_GLOBVAL( STL_YVIEW ) );
	
	Tcl_Eval( itp, cmd );

	sprintf( cmd, "scrollSet %s %s %d %d %d",
		CT_SBV, value, INT_GLOBVAL( CT_CHEIGHT ),
		INT_GLOBVAL( ST_LABEL_HT ), INT_GLOBVAL( CT_YVIEW ) );
	
	Tcl_Eval( itp, cmd );

	return( TCL_OK );
}


void
taskSTIncr( sec, usec )
int sec;
int usec;
{
	TASK T;

	STATE S;

	char cmd[1024];

	REFRESH_GLOBAL( ST_CANVAS_HEIGHT );
	REFRESH_GLOBAL( ST_TIMELINE );
	REFRESH_GLOBAL( ST_ACTIVE );
	REFRESH_GLOBAL( TIMEX );

	/* Check Space-Time View Active */

	if ( strcmp( CHAR_GLOBVAL( ST_ACTIVE ), "TRUE" ) )
		return;

	/* Adjust Time Line */

	sprintf( cmd, "%s coords %d %d %d %d %d",
		ST_C, INT_GLOBVAL( ST_TIMELINE ),
		INT_GLOBVAL( TIMEX ), 0,
		INT_GLOBVAL( TIMEX ), INT_GLOBVAL( ST_CANVAS_HEIGHT ) );

	Tcl_Eval( interp, cmd );

	/* Adjust Task State Rectangles */

	T = TASK_LIST;

	while ( T != NULL )
	{
		if ( T->status == TASK_ALIVE )
		{
			if ( T->deadtime.tv_sec == -1 )
			{
				S = T->states;

				if ( S != NULL && S->status != STATE_DEAD )
				{
					X2_COORD( S->rect ) = INT_GLOBVAL( TIMEX );

					SET_COORDS( interp, ST_C, S->rect );

					S->endtime.tv_sec = sec;
					S->endtime.tv_usec = usec;
				}
			}
		}

		T = T->next;
	}
}


void
do_time_correlate( sec, usec )
int sec;
int usec;
{
	HOST_EXT_DRAW HED;

	TRC_HOST H;

	NETLINK L;

	STATE S;

	TASK T;

	MSGQ Qnext;
	MSGQ Q;

	MSG M;

	char cmd[1024];

	char *color;

	int reset_me;

	int do_net_arr;
	int do_mq_arr;
	int do_host;

	int cmps, cmpe;
	int flag;
	int i;

	reset_me = ( sec < 0 || usec < 0 );

	do_net_arr = FALSE;
	do_mq_arr = FALSE;

	/* Prep Network View */

	REFRESH_GLOBAL( NET_RUNNING_COLOR );

	if ( reset_me )
		{ REFRESH_NET_VOLUME_COLORS; }

	NEEDS_REDRAW_NT = TRUE;

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		if ( HED->status == HOST_CORRELATE )
		{
			if ( HED->correlate_status == HOST_OFF )
			{
				undraw_network_host( H, FALSE );

				HED->color = (char *) NULL;

				do_net_arr = TRUE;
			}

			else
			{
				HED->status = HED->correlate_status;

				set_host_box_status( H, HED->status );
			}
		}

		else if ( HED->status == HOST_OFF
			&& HED->correlate_status != -1 )
		{
			HED->status = HED->correlate_status;

			draw_network_host( H );

			set_host_box_status( H, HED->status );

			do_net_arr = TRUE;
		}

		HED->correlate_status = -1;

		for ( i=0 ; i < MAX_STATE ; i++ )
			HED->correlate_counts[i] = 0;

		for ( i=0 ; i < MAX_LINK ; i++ )
		{
			if ( (L = HED->links[i]) != NULL )
			{
				if ( reset_me )
				{
					if ( L->correlate_bytes != -1 )
						L->msg_bytes = L->correlate_bytes;

					L->correlate_bytes = -1;

					if ( L->correlate_bw != -1 )
					{
						L->msg_bw = L->correlate_bw;

						L->bw_start.tv_usec =
							L->correlate_bw_start.tv_usec;
						L->bw_start.tv_sec =
							L->correlate_bw_start.tv_sec;

						L->bw_end.tv_usec =
							L->correlate_bw_end.tv_usec;
						L->bw_end.tv_sec =
							L->correlate_bw_end.tv_sec;
					}

					L->correlate_bw = -1;

					L->correlate_bw_start.tv_usec = 0;
					L->correlate_bw_start.tv_sec = 0;

					L->correlate_bw_end.tv_usec = 0;
					L->correlate_bw_end.tv_sec = 0;

					NET_COLOR_LINK_VOLUME( interp,
						MAIN_NET->NET_C, L );

					DRAW_NET_BANDWIDTH( interp, MAIN_NET->NET_C, L );
				}

				else
				{
					if ( L->link != NULL )
					{
						sprintf( cmd,
							"%s itemconfigure %d -fill %s",
							MAIN_NET->NET_C, L->link->id,
							CHAR_GLOBVAL( NET_FG_COLOR ) );
					
						Tcl_Eval( interp, cmd );
					}

					if ( L->correlate_bytes == -1 )
						L->correlate_bytes = L->msg_bytes;

					L->msg_bytes = 0;

					if ( L->correlate_bw == -1 )
					{
						L->correlate_bw = L->msg_bw;

						L->correlate_bw_start.tv_usec =
							L->bw_start.tv_usec;
						L->correlate_bw_start.tv_sec =
							L->bw_start.tv_sec;

						L->correlate_bw_end.tv_usec =
							L->bw_end.tv_usec;
						L->correlate_bw_end.tv_sec =
							L->bw_end.tv_sec;
					}

					L->msg_bw = 0;

					L->bw_start.tv_usec = 0;
					L->bw_start.tv_sec = 0;

					L->bw_end.tv_usec = 0;
					L->bw_end.tv_sec = 0;
				}
			}
		}

		H = H->next;
	}

	for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
	{
		if ( (L = MAIN_NET->links[i]) != NULL )
		{
			if ( reset_me )
			{
				if ( L->correlate_bytes != -1 )
					L->msg_bytes = L->correlate_bytes;

				L->correlate_bytes = -1;

				if ( L->correlate_bw != -1 )
				{
					L->msg_bw = L->correlate_bw;

					L->bw_start.tv_usec =
						L->correlate_bw_start.tv_usec;
					L->bw_start.tv_sec =
						L->correlate_bw_start.tv_sec;

					L->bw_end.tv_usec =
						L->correlate_bw_end.tv_usec;
					L->bw_end.tv_sec =
						L->correlate_bw_end.tv_sec;
				}

				L->correlate_bw = -1;

				L->correlate_bw_start.tv_usec = 0;
				L->correlate_bw_start.tv_sec = 0;

				L->correlate_bw_end.tv_usec = 0;
				L->correlate_bw_end.tv_sec = 0;

				NET_COLOR_LINK_VOLUME( interp,
					MAIN_NET->NET_C, L );

				DRAW_NET_BANDWIDTH( interp, MAIN_NET->NET_C, L );
			}

			else
			{
				if ( L->link != NULL )
				{
					sprintf( cmd,
						"%s itemconfigure %d -fill %s",
						MAIN_NET->NET_C, L->link->id,
						CHAR_GLOBVAL( NET_FG_COLOR ) );
					
					Tcl_Eval( interp, cmd );
				}

				if ( L->correlate_bytes == -1 )
					L->correlate_bytes = L->msg_bytes;

				L->msg_bytes = 0;

				if ( L->correlate_bw == -1 )
				{
					L->correlate_bw = L->msg_bw;

					L->correlate_bw_start.tv_usec =
						L->bw_start.tv_usec;
					L->correlate_bw_start.tv_sec =
						L->bw_start.tv_sec;

					L->correlate_bw_end.tv_usec =
						L->bw_end.tv_usec;
					L->correlate_bw_end.tv_sec =
						L->bw_end.tv_sec;
				}

				L->msg_bw = 0;

				L->bw_start.tv_usec = 0;
				L->bw_start.tv_sec = 0;

				L->bw_end.tv_usec = 0;
				L->bw_end.tv_sec = 0;
			}
		}
	}

	/* Prep Msgq View */

	NEEDS_REDRAW_MQ = TRUE;

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
				DELETE_GOBJ( interp, MQ_C, Q->rect );

				do_mq_arr = TRUE;
			}

			Qnext = Q->next;

			free_msgq( &Q );

			Q = Qnext;
		}

		T = T->next;
	}

	/* Prep Call Trace View */

	NEEDS_REDRAW_CT = TRUE;

	/* Verify Valid Correlate Time */

	if ( reset_me )
	{
		if ( do_net_arr )
			arrange_network_hosts( MAIN_NET );

		return;
	}

	/* Find Task States */

	T = TASK_LIST;

	while ( T != NULL )
	{
		S = T->states;

		flag = 0;

		while ( S != NULL && !flag )
		{
			cmps = TIME_COMPARE(
				S->starttime.tv_sec, S->starttime.tv_usec,
				sec, usec );

			if ( cmps <= 0 )
			{
				/* Update Host Count */

				HED = (HOST_EXT_DRAW) T->host->ext;

				(HED->correlate_counts[ S->status ])++;

				/* Update Call Trace Text */

				sprintf( cmd, "%s itemconfigure %d -text {%s}",
					CT_C, T->ct_callstr->id, S->callstr );

				Tcl_Eval( interp, cmd );

				/* Next Task */

				flag++;
			}

			S = S->next;
		}

		if ( !flag )
		{
			sprintf( cmd, "%s itemconfigure %d -text {%s}",
				CT_C, T->ct_callstr->id, "(Unborn Task)" );

			Tcl_Eval( interp, cmd );
		}

		T = T->next;
	}

	/* Do Network View Correlate */

	REFRESH_GLOBAL( NET_RUNNING_COLOR );
	REFRESH_GLOBAL( NET_SYSTEM_COLOR );
	REFRESH_GLOBAL( NET_IDLE_COLOR );
	REFRESH_GLOBAL( NET_EMPTY_COLOR );

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		do_host = FALSE;

		if ( HED->status == HOST_OFF )
		{
			for ( i=0 ; i < MAX_STATE ; i++ )
			{
				if ( i != STATE_DEAD && HED->correlate_counts[i] )
					do_host = TRUE;
			}

			if ( do_host )
			{
				HED->correlate_status = HOST_OFF;

				HED->status = HOST_CORRELATE;

				draw_network_host( H );

				do_net_arr = TRUE;
			}
		}
		
		else
		{
			HED->correlate_status = HED->status;

			for ( i=0 ; i < MAX_STATE ; i++ )
			{
				if ( i != STATE_DEAD && HED->correlate_counts[i] )
					do_host = TRUE;
			}

			if ( do_host )
			{
				HED->status = HOST_CORRELATE;

				set_host_box_status( H, HED->status );
			}

			else
			{
				undraw_network_host( H, FALSE );

				HED->color = (char *) NULL;

				do_net_arr = TRUE;
			}
		}

		if ( do_host )
		{
			/* Set Host Color */

			if ( HED->correlate_counts[STATE_RUNNING] > 0 )
				color = CHAR_GLOBVAL( NET_RUNNING_COLOR );

			else if ( HED->correlate_counts[STATE_SYSTEM] > 0 )
				color = CHAR_GLOBVAL( NET_SYSTEM_COLOR );

			else if ( HED->correlate_counts[STATE_IDLE] > 0 )
				color = CHAR_GLOBVAL( NET_IDLE_COLOR );

			else
				color = CHAR_GLOBVAL( NET_EMPTY_COLOR );

			if ( HED->color == NULL || strcmp( HED->color, color ) )
				color_network_host( H, color );

			HED->color = color;
		}

		H = H->next;
	}

	if ( do_net_arr )
		arrange_network_hosts( MAIN_NET );

	/* Do Msgq View Correlate */

	REFRESH_GLOBAL( MQ_MSG_NOSIZE_COLOR );
	REFRESH_GLOBAL( MQ_MSG_SIZE_COLOR );
	REFRESH_GLOBAL( MQ_OUTLINE_COLOR );

	/* Find Full Messages */

	M = MSG_LIST;

	while ( M != NULL )
	{
		cmps = TIME_COMPARE(
			M->sendtime.tv_sec, M->sendtime.tv_usec,
			sec, usec );

		if ( cmps <= 0 )
		{
			cmpe = TIME_COMPARE(
				M->recvtime.tv_sec, M->recvtime.tv_usec,
				sec, usec );

			if ( cmpe >= 0 )
			{
				/* Add Message to Msgq View */

				make_msgq( M );

				if ( M->DT->mqbytes > MQ_MAX_NBYTES )
					MQ_MAX_NBYTES = M->DT->mqbytes;

				do_mq_arr = TRUE;

				/* Add Message to Network View */

				net_add_volume( M );

				net_add_bw( M );
			}
		}

		M = M->next;
	}

	/* Find Partial Messages */

	M = PENDING_MSG_LIST;

	while ( M != NULL )
	{
		if ( M->DT != NULL
			&& M->recvtime.tv_sec == -1
			&& M->recvtime.tv_usec == -1 )
		{
			cmps = TIME_COMPARE(
				M->sendtime.tv_sec, M->sendtime.tv_usec,
				sec, usec );

			if ( cmps <= 0 )
			{
				/* Add Message to Msgq View */

				make_msgq( M );

				if ( M->DT->mqbytes > MQ_MAX_NBYTES )
					MQ_MAX_NBYTES = M->DT->mqbytes;

				do_mq_arr = TRUE;

				/* Add Message to Network View */

				net_add_volume( M );
			}
		}

		M = M->next;
	}

	/* Re-Arrange Msgq View */

	if ( do_mq_arr )
	{
		/* Set Max Nbytes Scale */

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

		/* Arrange Task Stacks */

		T = TASK_LIST;

		while ( T != NULL )
		{
			mqArrangeTask( T );

			T = T->next;
		}
	}

	/* Draw Network Volume */

	REFRESH_NET_VOLUME_COLORS;

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		for ( i=0 ; i < MAX_LINK ; i++ )
		{
			if ( (L = HED->links[i]) != NULL )
			{
				if ( L->link != NULL )
				{
					NET_COLOR_LINK_VOLUME( interp,
						MAIN_NET->NET_C, L );

					DRAW_NET_BANDWIDTH( interp, MAIN_NET->NET_C, L );
				}
			}
		}
		
		H = H->next;
	}

	for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
	{
		if ( (L = MAIN_NET->links[i]) != NULL )
		{
			if ( L->link != NULL )
			{
				NET_COLOR_LINK_VOLUME( interp,
					MAIN_NET->NET_C, L );

				DRAW_NET_BANDWIDTH( interp, MAIN_NET->NET_C, L );
			}
		}
	}
}


void
set_host_box_status( H, status )
TRC_HOST H;
int status;
{
	switch ( status )
	{
		case HOST_ON:
		case HOST_CORRELATE:
		{
			REFRESH_GLOBAL( NET_FG_COLOR );

			SET_HOST_BOX( interp, MAIN_NET->NET_C, H,
				CHAR_GLOBVAL( NET_FG_COLOR ), 1 );

			break;
		}

		case HOST_NOTIFY_ADD:
		case HOST_ADDED:
		{
			REFRESH_GLOBAL( NET_ALIVE_COLOR );

			SET_HOST_BOX( interp, MAIN_NET->NET_C, H,
				CHAR_GLOBVAL( NET_ALIVE_COLOR ), 2 );

			break;
		}

		case HOST_NOTIFY_DEL:
		case HOST_DELETED:
		{
			REFRESH_GLOBAL( NET_DEAD_COLOR );

			SET_HOST_BOX( interp, MAIN_NET->NET_C, H,
				CHAR_GLOBVAL( NET_DEAD_COLOR ), 2 );

			break;
		}
	}
}


/* ARGSUSED */
int
task_label_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	static int save_index = -1;

	TASK T;

	char tmp[1024];

	char *label;
	char *op;

	int sindex, dindex;
	int custom;
	int index;
	int dest;
	int orig;
	int diff;
	int adj;

	op = argv[1];

	index = atoi( argv[2] );

	custom = 0;

	if ( TASK_SORT == TASK_SORT_CUSTOM )
	{
		if ( !strcmp( op, "press" ) )
			save_index = index;

		else if ( !strcmp( op, "move" ) )
		{
			if ( save_index != -1 )
				index = save_index;
		}

		else if ( !strcmp( op, "release" ) )
		{
			if ( save_index != -1 )
			{
				adj = save_index % 10; 

				sindex = save_index - adj;
				dindex = index - adj;

				orig = ( sindex - ( sindex % 10 ) ) / 10;
				dest = ( dindex - ( dindex % 10 ) ) / 10;

				diff = ( sindex > dindex ) ?
					( sindex - dindex ) : ( dindex - sindex );
				
				diff %= 10;

				if ( ( diff > 4 && dindex > sindex )
						|| ( diff < 4 && sindex > dindex ) )
					dest++;

				dest = ( dest < 0 ) ? 0 : dest;

				if ( orig != dest )
				{
					swap_tasks( orig, dest );

					sprintf( tmp, "TRUE {Moved Task %d to %d}",
						orig, dest );

					Tcl_SetResult( itp,
						trc_copy_str( tmp ), TCL_DYNAMIC );

					return( TCL_OK );
				}

				index = save_index;
			}
		}

		custom++;
	}

	index = ( index - ( index % 10 ) ) / 10;

	T = TASK_LIST;

	while ( T != NULL && T->index != index )
		T = T->next;

	label = task_label_str( T );

	sprintf( tmp, "%s {%s}",
		custom ? "TRUE" : "FALSE",
		label );

	Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

	free( label );

	return( TCL_OK );
}


/* ARGSUSED */
int
task_query_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	STATE S;

	char value[1024];
	char cmd[1024];

	char *susec_pad;
	char *eusec_pad;

	int tindex1, tindex2;
	int id;

	id = atoi( argv[1] );

	tindex1 = atoi( argv[2] );
	tindex2 = atoi( argv[3] );

	T = TASK_LIST;

	while ( T != NULL && T->index < tindex1 )
		T = T->next;

	while ( T != NULL && T->index <= tindex2 )
	{
		S = T->states;

		while ( S != NULL )
		{
			if ( S->rect != NULL && S->rect->id == id )
			{
				/* Highlight State Rectangle */

				sprintf( cmd, "%s itemconfigure %d -width 3",
					ST_C, id );
				
				Tcl_Eval( itp, cmd );

				SET_TCL_GLOBAL( itp, "st_highlight_type", "state" );

				sprintf( value, "%d", id );

				SET_TCL_GLOBAL( itp, "st_highlight_id", value );

				/* Construct Info Message */

				susec_pad = trc_pad_num( S->starttime.tv_usec, 6 );
				eusec_pad = trc_pad_num( S->endtime.tv_usec, 6 );

				sprintf( value, "TID=0x%x: %s <%d.%s - %d.%s>",
					T->tid, S->callstr,
					S->starttime.tv_sec, susec_pad,
					S->endtime.tv_sec, eusec_pad );

				free( susec_pad );
				free( eusec_pad );

				sprintf( cmd,
			"%s.query_frame.query configure -text {View Info:   %s}",
					ST, value );

				Tcl_Eval( itp, cmd );

				SET_TCL_GLOBAL( itp, "st_query_text_scroll", "0" );

				SET_TCL_GLOBAL( itp, "st_query_text", value );

				Tcl_SetResult( itp, "1", TCL_STATIC );

				return( TCL_OK );
			}

			S = S->next;
		}

		T = T->next;
	}

	Tcl_SetResult( itp, "0", TCL_STATIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
task_zoom_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	STATE S;
	STATE last;

	char cmd[1024];

	char *color;

	int new_scale;
	int xrange;
	int x1, x2;

	REFRESH_GLOBAL( ST_USER_DEFINED_COLOR );
	REFRESH_GLOBAL( ST_OUTLINE_COLOR );
	REFRESH_GLOBAL( ST_RUNNING_COLOR );
	REFRESH_GLOBAL( ST_SYSTEM_COLOR );
	REFRESH_GLOBAL( ST_IDLE_COLOR );

	new_scale = atoi( argv[1] );

	T = TASK_LIST;

	while ( T != NULL )
	{
		S = T->states;

		last = (STATE) NULL;

		while ( S != NULL )
		{
			x1 = X_OF_TIME( S->starttime.tv_sec, S->starttime.tv_usec,
				new_scale );

			CK_X_TIME( x1, S->starttime.tv_sec, S->starttime.tv_usec );

			x2 = X_OF_TIME( S->endtime.tv_sec, S->endtime.tv_usec,
				new_scale );
	
			CK_X_TIME( x2, S->endtime.tv_sec, S->endtime.tv_usec );

			/* Create or Modify Rectangle */

			xrange = x2 - x1;

			if ( xrange
				&& ( xrange != 1
					|| last == NULL || last->rect == NULL ) )
			{
				if ( S->rect != NULL )
				{
					X1_COORD( S->rect ) = x1;
					X2_COORD( S->rect ) = x2;
		
					SET_COORDS( itp, ST_C, S->rect );
				}

				else
				{
					if ( S->user_defined )
						color = CHAR_GLOBVAL( ST_USER_DEFINED_COLOR );

					else if ( S->status == STATE_RUNNING )
						color = CHAR_GLOBVAL( ST_RUNNING_COLOR );

					else if ( S->status == STATE_SYSTEM )
						color = CHAR_GLOBVAL( ST_SYSTEM_COLOR );

					else if ( S->status == STATE_IDLE )
						color = CHAR_GLOBVAL( ST_IDLE_COLOR );

					else
						color = CHAR_GLOBVAL( ST_OUTLINE_COLOR );

					CREATE_RECT( interp, ST_C, S->rect,
						x1, T->y1, x2, T->y2,
						color, CHAR_GLOBVAL( ST_OUTLINE_COLOR ) );
				}
			}

			/* No Rectangle, Destroy if Present */

			else if ( S->rect != NULL )
			{
				if ( last != NULL )
				{
					DELETE_GOBJ( itp, ST_C, S->rect );
				}

				else
				{
					X1_COORD( S->rect ) = x1;
					X2_COORD( S->rect ) = x2;
		
					SET_COORDS( itp, ST_C, S->rect );
				}
			}

			else if ( last == NULL )
			{
				if ( S->user_defined )
					color = CHAR_GLOBVAL( ST_USER_DEFINED_COLOR );

				else if ( S->status == STATE_RUNNING )
					color = CHAR_GLOBVAL( ST_RUNNING_COLOR );

				else if ( S->status == STATE_SYSTEM )
					color = CHAR_GLOBVAL( ST_SYSTEM_COLOR );

				else if ( S->status == STATE_IDLE )
					color = CHAR_GLOBVAL( ST_IDLE_COLOR );

				else
					color = CHAR_GLOBVAL( ST_OUTLINE_COLOR );

				CREATE_RECT( interp, ST_C, S->rect,
					x1, T->y1, x2, T->y2,
					color, CHAR_GLOBVAL( ST_OUTLINE_COLOR ) );
			}

			last = S;

			S = S->next;
		}

		T = T->next;
	}

	return( TCL_OK );
}


void
redraw_spacetime()
{
	REFRESH_GLOBAL( ST_ACTIVE );

	if ( !strcmp( CHAR_GLOBVAL( ST_ACTIVE ), "TRUE" ) )
		Tcl_Eval( interp, "taskSTUpdate" );
}


/* ARGSUSED */
int
task_st_update_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[1024];

	int save;

	if ( NEEDS_REDRAW_ST )
	{
		LOCK_INTERFACE( save );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Space-Time... }" );
		
		REFRESH_GLOBAL( SCALE );

		Tcl_Eval( interp, "taskArrange" );

		sprintf( tmp, "taskZoom %d", INT_GLOBVAL( SCALE ) );
		Tcl_Eval( interp, tmp );

		Tcl_Eval( interp, "commArrange" );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Space-Time... Done. }" );
		
		NEEDS_REDRAW_ST = FALSE;

		UNLOCK_INTERFACE( save );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
task_reset_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	STATE S;

	char cmd[1024];

	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FRAME_OFFSET );

	T = TASK_LIST;

	while ( T != NULL )
	{
		/* Delete Space-Time Label */

		if ( T->st_label != NULL )
			DELETE_GOBJ( itp, ST_L, T->st_label );

		/* Delete Call Trace Label / Call String */

		if ( T->ct_label != NULL )
			DELETE_GOBJ( itp, CT_L, T->ct_label );

		if ( T->ct_callstr != NULL )
			DELETE_GOBJ( itp, CT_C, T->ct_callstr );

		/* Delete Space-Time State Rectangles */

		S = T->states;

		while ( S != NULL )
		{
			if ( S->rect != NULL )
			{
				DELETE_GOBJ( itp, ST_C, S->rect );
			}

			S = S->next;
		}

		T = T->next;
	}

	ST_SCROLL_MARK = 0;

	return( TCL_OK );
}


/* ARGSUSED */
int
task_ct_update_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	char cmd[1024];

	char *str;

	int save;

	if ( NEEDS_REDRAW_CT )
	{
		LOCK_INTERFACE( save );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Call Trace... }" );

		if ( CORRELATE_LOCK == LOCKED )
		{
			UNLOCK_INTERFACE( save );

			return( TCL_OK );
		}

		T = TASK_LIST;

		while ( T != NULL )
		{
			if ( T->states != NULL )
				str = T->states->callstr;

			else
				str = "(Task Exited)";

			sprintf( cmd, "%s itemconfigure %d -text {%s}",
				CT_C, T->ct_callstr->id, str );
		
			Tcl_Eval( itp, cmd );

			T = T->next;
		}

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Call Trace... Done. }" );

		NEEDS_REDRAW_CT = FALSE;

		UNLOCK_INTERFACE( save );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
task_ct_query_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TASK T;

	STATE S;

	char value[1024];

	char *usec_pad;

	int index;

	index = atoi( argv[1] );

	T = TASK_LIST;

	while ( T != NULL && T->index != index )
		T = T->next;

	if ( T != NULL && T->states != NULL )
	{
		S = T->states;

		usec_pad = trc_pad_num( S->starttime.tv_usec, 6 );

		sprintf( value, "Time Stamp: %d.%s",
			S->starttime.tv_sec, usec_pad );

		free( usec_pad );
	}

	else
		strcpy( value, "" );

	Tcl_SetResult( itp, trc_copy_str( value ), TCL_DYNAMIC );

	return( TCL_OK );
}


void
redraw_calltrace()
{
	REFRESH_GLOBAL( CT_ACTIVE );

	if ( !strcmp( CHAR_GLOBVAL( CT_ACTIVE ), "TRUE" ) )
		Tcl_Eval( interp, "taskCTUpdate" );
}

