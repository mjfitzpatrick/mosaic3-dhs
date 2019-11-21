
static char rcsid[] = 
	"$Id: util.c,v 4.50 1998/04/09 21:12:06 kohl Exp $";

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


TCL_GLOBAL
create_tcl_global()
{
	TCL_GLOBAL tmp;

	tmp = (TCL_GLOBAL) malloc( sizeof( struct tcl_glob_struct ) );
	trc_memcheck( tmp, "TCL Global Structure" );

	tmp->name = (char *) NULL;

	tmp->type = -1;

	tmp->char_value = (char *) NULL;

	tmp->int_value = -1;

	return( tmp );
}


void
free_tcl_global( ptr )
TCL_GLOBAL *ptr;
{
	TCL_GLOBAL G;

	if ( ptr == NULL || *ptr == NULL )
		return;

	G = *ptr;

	G->name = (char *) NULL;

	G->type = -1;

	G->char_value = (char *) NULL;

	G->int_value = -1;

	free( G );

	*ptr = (TCL_GLOBAL) NULL;
}


TRACE_MASK_GROUP
create_trace_mask_group()
{
	TRACE_MASK_GROUP tmp;

	tmp = (TRACE_MASK_GROUP) malloc(
		sizeof( struct trace_mask_group_struct ) );
	trc_memcheck( tmp, "Trace Mask Group Structure" );

	tmp->name = (char *) NULL;

	tmp->tmask = (char *) NULL;

	tmp->next = (TRACE_MASK_GROUP) NULL;

	return( tmp );
}


TRACE_MASK_GROUP
lookup_trace_mask_group( TMG_LIST, name )
TRACE_MASK_GROUP TMG_LIST;
char *name;
{
	TRACE_MASK_GROUP TMG;

	TMG = TMG_LIST;

	while ( TMG != NULL && strcmp( name, TMG->name ) )
		TMG = TMG->next;

	return( TMG );
}


void
set_trace_format( fmt, interface )
int fmt;
int interface;
{
	char cmd[255];

	TRACE_FORMAT = fmt;

	if ( fmt == TEV_FMT_33 )
	{
		CURRENT_GROUP_LIST = &TM33_GROUP_LIST;

		CURRENT_INDEX_LIST = &TM33_INDEX_LIST;

		CURRENT_TRACE_MASK = TRACE33_MASK;

		if ( interface )
			strcpy( cmd, "set_trace_format 3.3" );
	}

	else
	{
		CURRENT_GROUP_LIST = &TM_GROUP_LIST;

		CURRENT_INDEX_LIST = &TM_INDEX_LIST;

		CURRENT_TRACE_MASK = TRACE_MASK;

		if ( interface )
			strcpy( cmd, "set_trace_format 3.4" );
	}

	if ( interface )
	{
		pvm_settmask( PvmTaskChild, CURRENT_TRACE_MASK );

		Tcl_Eval( interp, cmd );
	}
}


void
set_trace_mask_buttons( itp, TMASK, TMG_LIST, TMI_LIST, fmt )
Tcl_Interp *itp;
char *TMASK;
TRACE_MASK_GROUP TMG_LIST;
TRACE_MASK_INDEX TMI_LIST;
int fmt;
{
	TRACE_MASK_GROUP TMG;

	TRACE_MASK_INDEX TMI;

	char *TM;

	char result[2048];
	char tmp[4096];

	int nope;
	int j;

	strcpy( result, "" );

	/* Set Regular Event Buttons */

	TMI = TMI_LIST;

	while ( TMI != NULL )
	{
		if ( CHECK_TRACE_MASK( TMASK, fmt, TMI->index ) )
			sprintf( tmp, "{ %s ON } ", TMI->name );

		else
			sprintf( tmp, "{ %s OFF } ", TMI->name );

		append_cmd_str( result, tmp, 2048, "trace_mask_result",
			TRUE );

		TMI = TMI->next;
	}

	/* Check Group Event Buttons */

	TMG = TMG_LIST;

	while ( TMG != NULL )
	{
		TM = TMG->tmask;

		nope = FALSE;

		for ( j=FIRST_EVENT( fmt ) ; j <= LAST_EVENT( fmt ) && !nope ;
			j += EVENT_INCR( fmt ) )
		{
			if ( CHECK_TRACE_MASK( TM, fmt, j )
				&& !CHECK_TRACE_MASK( TMASK, fmt, j ) )
			{
				nope = TRUE;
			}
		}

		if ( nope )
			sprintf( tmp, "{ %s OFF } ", TMG->name );

		else
			sprintf( tmp, "{ %s ON } ", TMG->name );

		append_cmd_str( result, tmp, 2048, "trace_mask_result",
			TRUE );

		TMG = TMG->next;
	}

	sprintf( tmp, "trace_mask_result { %s }", result );

	Tcl_Eval( itp, tmp );
}


TRACE_MASK_INDEX
create_trace_mask_index()
{
	TRACE_MASK_INDEX tmp;

	tmp = (TRACE_MASK_INDEX) malloc(
		sizeof( struct trace_mask_index_struct ) );
	trc_memcheck( tmp, "Trace Mask Index Structure" );

	tmp->name = (char *) NULL;

	tmp->index = -1;

	tmp->next = (TRACE_MASK_INDEX) NULL;

	return( tmp );
}


void
set_trace_mask_index( TMI_LIST, name, index )
TRACE_MASK_INDEX *TMI_LIST;
char *name;
int index;
{
	TRACE_MASK_INDEX TMI;

	TMI = create_trace_mask_index();

	if ( *TMI_LIST == NULL )
		*TMI_LIST = TMI;

	else
	{
		TMI->next = *TMI_LIST;

		*TMI_LIST = TMI;
	}

	TMI->name = name;
	TMI->index = index;
}


TRACE_MASK_INDEX
lookup_trace_mask_name( TMI_LIST, name )
TRACE_MASK_INDEX TMI_LIST;
char *name;
{
	TRACE_MASK_INDEX TMI;

	TMI = TMI_LIST;

	while ( TMI != NULL && strcmp( name, TMI->name ) )
		TMI = TMI->next;

	return( TMI );
}


GOBJ
create_gobj()
{
	GOBJ tmp;

	int i;

	tmp = (GOBJ) malloc( sizeof( struct gobj_struct ) );
	trc_memcheck( tmp, "Graphical Object Structure" );

	tmp->id = -1;

	for ( i=0 ; i < MAX_GOBJ_COORDS ; i++ )
		tmp->coords[i] = -1;

	tmp->color = (char *) NULL;

	return( tmp );
}


void
free_gobj( ptr )
GOBJ *ptr;
{
	GOBJ G;

	int i;

	if ( ptr == NULL || *ptr == NULL )
		return;

	G = *ptr;

	G->id = -1;

	for ( i=0 ; i < MAX_GOBJ_COORDS ; i++ )
		G->coords[i] = -1;

	G->color = (char *) NULL;

	free( G );

	*ptr = (GOBJ) NULL;
}


TOBJ
create_tobj()
{
	TOBJ tmp;

	tmp = (TOBJ) malloc( sizeof( struct tobj_struct ) );
	trc_memcheck( tmp, "Textual Object Structure" );

	tmp->id = -1;

	tmp->x = tmp->y = -1;

	tmp->text = (char *) NULL;

	tmp->selected = -1;

	tmp->prev = (TOBJ) NULL;
	tmp->next = (TOBJ) NULL;

	return( tmp );
}


void
free_tobj( ptr )
TOBJ *ptr;
{
	TOBJ TT;

	if ( ptr == NULL || *ptr == NULL )
		return;

	TT = *ptr;

	TT->id = -1;

	TT->x = TT->y = -1;

	if ( TT->text != NULL )
	{
		free( TT->text );

		TT->text = (char *) NULL;
	}

	TT->selected = -1;

	TT->prev = (TOBJ) NULL;
	TT->next = (TOBJ) NULL;

	free( TT );

	*ptr = (TOBJ) NULL;
}


SEARCH
create_search()
{
	SEARCH tmp;

	tmp = (SEARCH) malloc( sizeof( struct search_struct ) );
	trc_memcheck( tmp, "Textual Search Structure" );

	tmp->direction = -1;

	tmp->changed = -1;

	tmp->search = (char *) NULL;

	tmp->regexp = (Tcl_RegExp) NULL;

	tmp->tobj = (TOBJ) NULL;

	tmp->marker = (char *) NULL;

	tmp->match_id = -1;

	tmp->x1 = tmp->x2 = -1;

	tmp->ycoord = -1;

	tmp->yview = -1;

	return( tmp );
}


void
free_search( ptr )
SEARCH *ptr;
{
	SEARCH S;

	if ( ptr == NULL || *ptr == NULL )
		return;

	S = *ptr;

	S->direction = -1;

	S->changed = -1;

	if ( S->search != NULL )
	{
		free( S->search );

		S->search = (char *) NULL;
	}

	S->regexp = (Tcl_RegExp) NULL;

	S->tobj = (TOBJ) NULL;

	S->marker = (char *) NULL;

	S->match_id = -1;

	S->x1 = S->x2 = -1;

	S->ycoord = -1;

	S->yview = -1;

	free( S );

	*ptr = (SEARCH) NULL;
}


void
reset_search( TW )
TWIN TW;
{
	SEARCH S;

	if ( TW == NULL || TW->search == NULL )
		return;

	S = TW->search;

	S->changed = -1;

	if ( S->search != NULL )
	{
		free( S->search );

		S->search = (char *) NULL;
	}

	S->regexp = (Tcl_RegExp) NULL;

	S->tobj = (TOBJ) NULL;

	S->marker = (char *) NULL;

	if ( S->match_id != -1 )
	{
		UNDRAW_TEXT( interp, TW->canvas, S->match_id );

		S->match_id = -1;
	}

	S->x1 = S->x2 = -1;

	S->ycoord = -1;

	S->yview = -1;
}


TWIN
create_twin()
{
	TWIN tmp;

	tmp = (TWIN) malloc( sizeof( struct twin_struct ) );
	trc_memcheck( tmp, "Textual Window Structure" );

	tmp->text = (TOBJ) NULL;

	tmp->last_text = (TOBJ) NULL;

	tmp->filter = (char *) NULL;

	tmp->regexp = (Tcl_RegExp) NULL;

	tmp->search = (SEARCH) NULL;

	tmp->canvas = (char *) NULL;
	tmp->sbv = (char *) NULL;

	tmp->file = (char *) NULL;
	tmp->fp = (FILE *) NULL;

	tmp->active = (TCL_GLOBAL) NULL;
	tmp->cheight = (TCL_GLOBAL) NULL;
	tmp->lasty = (TCL_GLOBAL) NULL;
	tmp->yview = (TCL_GLOBAL) NULL;

	tmp->needs_redraw = -1;

	return( tmp );
}


void
free_twin( ptr )
TWIN *ptr;
{
	TWIN TW;

	if ( ptr == NULL || *ptr == NULL )
		return;

	TW = *ptr;

	TW->text = (TOBJ) NULL;

	TW->last_text = (TOBJ) NULL;

	if ( TW->filter != NULL )
	{
		free( TW->filter );

		TW->filter = (char *) NULL;
	}

	TW->regexp = (Tcl_RegExp) NULL;

	if ( TW->search != NULL )
		free_search( &(TW->search) );

	if ( TW->canvas != NULL )
	{
		free( TW->canvas );

		TW->canvas = (char *) NULL;
	}

	if ( TW->sbv != NULL )
	{
		free( TW->sbv );

		TW->sbv = (char *) NULL;
	}

	if ( TW->file != NULL )
	{
		free( TW->file );
		
		TW->file = (char *) NULL;
	}

	if ( TW->fp != NULL )
	{
		fclose( TW->fp );

		TW->fp = (FILE *) NULL;
	}

	TW->active = (TCL_GLOBAL) NULL;
	TW->cheight = (TCL_GLOBAL) NULL;
	TW->lasty = (TCL_GLOBAL) NULL;
	TW->yview = (TCL_GLOBAL) NULL;

	TW->needs_redraw = -1;

	free( TW );

	*ptr = (TWIN) NULL;
}


MSG
create_msg()
{
	MSG tmp;

	tmp = (MSG) malloc( sizeof( struct msg_struct ) );
	trc_memcheck( tmp, "Message Structure" );

	tmp->ST = (TASK) NULL;
	tmp->DT = (TASK) NULL;

	tmp->src_tid = -1;
	tmp->dst_tid = -1;
	tmp->msgtag = -1;
	tmp->msgctx = -1;
	tmp->nbytes = -1;
	tmp->assbytes = -1;

	tmp->sendtime.tv_sec = -1;
	tmp->sendtime.tv_usec = -1;

	tmp->recvtime.tv_sec = -1;
	tmp->recvtime.tv_usec = -1;

	tmp->line = (GOBJ) NULL;

	tmp->prev = (MSG) NULL;
	tmp->next = (MSG) NULL;

	return( tmp );
}


void
free_msg( ptr )
MSG *ptr;
{
	MSG M;

	if ( ptr == NULL || *ptr == NULL )
		return;

	M = *ptr;

	M->ST = (TASK) NULL;
	M->DT = (TASK) NULL;

	M->src_tid = -1;
	M->dst_tid = -1;
	M->msgtag = -1;
	M->msgctx = -1;
	M->nbytes = -1;
	M->assbytes = -1;

	M->sendtime.tv_sec = -1;
	M->sendtime.tv_usec = -1;

	M->recvtime.tv_sec = -1;
	M->recvtime.tv_usec = -1;

	M->line = (GOBJ) NULL;

	M->prev = (MSG) NULL;
	M->next = (MSG) NULL;

	free( M );

	*ptr = (MSG) NULL;
}


MSGQ
create_msgq()
{
	MSGQ tmp;

	tmp = (MSGQ) malloc( sizeof( struct msgq_struct ) );
	trc_memcheck( tmp, "Message Queue Structure" );

	tmp->msg = (MSG) NULL;

	tmp->rect = (GOBJ) NULL;

	tmp->next = (MSGQ) NULL;

	return( tmp );
}


void
free_msgq( ptr )
MSGQ *ptr;
{
	MSGQ Q;

	if ( ptr == NULL || *ptr == NULL )
		return;

	Q = *ptr;

	Q->msg = (MSG) NULL;

	Q->rect = (GOBJ) NULL;

	Q->next = (MSGQ) NULL;

	free( Q );

	*ptr = (MSGQ) NULL;
}


MSG_GRID
create_msg_grid()
{
	MSG_GRID tmp;

	tmp = (MSG_GRID) malloc( sizeof( struct msg_grid_struct ) );
	trc_memcheck( tmp, "Message Grid Structure" );

	tmp->msg = (MSG) NULL;

	tmp->next = (MSG_GRID) NULL;

	return( tmp );
}


void
free_msg_grid( ptr )
MSG_GRID *ptr;
{
	MSG_GRID G;

	if ( ptr == NULL || *ptr == NULL )
		return;

	G = *ptr;

	G->msg = (MSG) NULL;

	G->next = (MSG_GRID) NULL;

	free( G );

	*ptr = (MSG_GRID) NULL;
}


STATE
create_state()
{
	STATE tmp;

	tmp = (STATE) malloc( sizeof( struct state_struct ) );
	trc_memcheck( tmp, "Task State Structure" );

	tmp->status = -1;

	tmp->callstr = (char *) NULL;

	tmp->rect = (GOBJ) NULL;

	tmp->user_defined = -1;

	tmp->starttime.tv_sec = -1;
	tmp->starttime.tv_usec = -1;

	tmp->endtime.tv_sec = -1;
	tmp->endtime.tv_usec = -1;

	tmp->next = (STATE) NULL;

	return( tmp );
}


void
free_state( ptr )
STATE *ptr;
{
	STATE S;

	if ( ptr == NULL || *ptr == NULL )
		return;

	S = *ptr;

	S->status = -1;

	S->callstr = (char *) NULL;

	S->rect = (GOBJ) NULL;

	S->user_defined = -1;

	S->starttime.tv_sec = -1;
	S->starttime.tv_usec = -1;

	S->endtime.tv_sec = -1;
	S->endtime.tv_usec = -1;

	S->next = (STATE) NULL;

	free( S );

	*ptr = (STATE) NULL;
}


TASK
create_task()
{
	TASK tmp;

	tmp = (TASK) malloc( sizeof( struct task_struct ) );
	trc_memcheck( tmp, "Task Structure" );

	tmp->name = (char *) NULL;

	tmp->tid = -1;
	tmp->ptid = -1;
	tmp->pvmd_tid = -1;

	tmp->host = (TRC_HOST) NULL;

	tmp->flags = -1;
	tmp->status = -1;

	tmp->deadtime.tv_sec = -1;
	tmp->deadtime.tv_usec = -1;

	tmp->index = -1;

	tmp->y1 = tmp->y2 = -1;

	tmp->st_label = (GOBJ) NULL;

	tmp->ct_label = (GOBJ) NULL;
	tmp->ct_callstr = (GOBJ) NULL;

	tmp->states = (STATE) NULL;

	tmp->msgq = (MSGQ) NULL;

	tmp->mqmark = (GOBJ) NULL;

	tmp->mqcnt = -1;
	tmp->mqbytes = -1;

	tmp->next = (TASK) NULL;

	return( tmp );
}


void
free_task( ptr )
TASK *ptr;
{
	TASK T;

	STATE S;
	STATE stmp;

	if ( ptr == NULL || *ptr == NULL )
		return;

	T = *ptr;

	if ( T->name != NULL )
		free( T->name );
	
	T->tid = -1;
	T->ptid = -1;
	T->pvmd_tid = -1;

	T->host = (TRC_HOST) NULL;

	T->flags = -1;
	T->status = -1;

	T->deadtime.tv_sec = -1;
	T->deadtime.tv_usec = -1;

	T->index = -1;

	T->y1 = T->y2 = -1;

	T->st_label = (GOBJ) NULL;

	T->ct_label = (GOBJ) NULL;

	T->ct_callstr = (GOBJ) NULL;

	if ( T->states != NULL )
	{
		S = T->states;

		while ( S != NULL )
		{
			stmp = S->next;

			free_state( &S );

			S = stmp;
		}
	}

	T->states = (STATE) NULL;

	T->msgq = (MSGQ) NULL;

	T->mqmark = (GOBJ) NULL;

	T->mqcnt = -1;
	T->mqbytes = -1;

	T->next = (TASK) NULL;

	free( T );

	*ptr = (TASK) NULL;
}


TASK
get_task_tid( tid )
int tid;
{
	TASK T;

	T = TASK_LIST;

	while ( T != NULL )
	{
		/* printf( "T" ); fflush( stdout ); */

		if ( T->tid == tid )
			return( T );

		T = T->next;
	}

	return( (TASK) NULL );
}


int
tasks_alive()
{
	TASK T;

	int undead;
	int alive;

	T = TASK_LIST;

	undead = 0;

	alive = 0;

	while ( T != NULL )
	{
		if ( T->status == TASK_ALIVE )
			alive++;

		if ( T->deadtime.tv_sec != -1 )
			undead++;

		T = T->next;
	}

	/* Game Over, Kill 'Em */

	if ( alive && undead >= alive )
	{
		T = TASK_LIST;

		while ( T != NULL )
		{
			if ( T->deadtime.tv_sec != -1 )
			{
				taskState( T, T->deadtime.tv_sec, T->deadtime.tv_usec,
					STATE_DEAD, "Dead Host" );

				taskExit( T, T->deadtime.tv_sec, T->deadtime.tv_usec,
					"Dead Host" );
			}

			T = T->next;
		}

		return( FALSE );
	}

	if ( alive )
		return( TRUE );
	
	else
		return( FALSE );
}


UT_STATE
create_ut_state()
{
	UT_STATE tmp;

	int i;

	tmp = (UT_STATE) malloc( sizeof( struct ut_state_struct ) );
	trc_memcheck( tmp, "Utilization State Structure" );

	tmp->running = (GOBJ) NULL;
	tmp->system = (GOBJ) NULL;
	tmp->idle = (GOBJ) NULL;

	for ( i=0 ; i < MAX_STATE ; i++ )
		tmp->counts[i] = -1;

	tmp->starttime.tv_sec = -1;
	tmp->starttime.tv_usec = -1;

	tmp->endtime.tv_sec = -1;
	tmp->endtime.tv_usec = -1;

	tmp->next = (UT_STATE) NULL;

	return( tmp );
}


void
free_ut_state( ptr )
UT_STATE *ptr;
{
	UT_STATE U;

	int i;

	if ( ptr == NULL || *ptr == NULL )
		return;

	U = *ptr;

	U->running = (GOBJ) NULL;
	U->system = (GOBJ) NULL;
	U->idle = (GOBJ) NULL;

	for ( i=0 ; i < MAX_STATE ; i++ )
		U->counts[i] = -1;

	U->starttime.tv_sec = -1;
	U->starttime.tv_usec = -1;

	U->endtime.tv_sec = -1;
	U->endtime.tv_usec = -1;

	U->next = (UT_STATE) NULL;

	free( U );

	*ptr = (UT_STATE) NULL;
}


ARCH
create_arch()
{
	ARCH tmp;

	tmp = (ARCH) malloc( sizeof( struct arch_struct ) );
	trc_memcheck( tmp, "Architecture Structure" );

	tmp->code = (char *) NULL;

	tmp->name = (char *) NULL;

	tmp->height = -1;
	tmp->width = -1;

	tmp->next = (ARCH) NULL;

	return( tmp );
}


ARCH
get_arch_code( name )
char *name;
{
	ARCH A;
	ARCH last;

	ARCH dummy;
	ARCH tmp;

	if ( name == NULL )
		return( (ARCH) NULL );

	tmp = (ARCH) NULL;

	A = ARCH_LIST;

	last = (ARCH) NULL;

	while ( A != NULL && tmp == NULL )
	{
		if ( !strcmp( name, A->code ) )
			tmp = A;

		else
		{
			last = A;

			A = A->next;
		}
	}

	if ( tmp == NULL )
	{
		printf( "\nWarning: Missing Architecture Icon for %s\n\n",
			name );

		dummy = get_arch_code( "DUMMY" );

		if ( last == NULL )
			tmp = ARCH_LIST = create_arch();
		
		else
			tmp = last->next = create_arch();

		tmp->code = trc_copy_str( name );

		tmp->name = trc_copy_str( dummy->name );

		tmp->height = dummy->height;
		tmp->width = dummy->width;
	}

	return( tmp );
}


NETLINK
create_netlink()
{
	NETLINK tmp;

	tmp = (NETLINK) malloc( sizeof( struct netlink_struct ) );
	trc_memcheck( tmp, "Network Link Structure" );

	tmp->link = (GOBJ) NULL;

	tmp->msg_bytes = 0;
	tmp->correlate_bytes = -1;

	tmp->msg_bw = 0;
	tmp->correlate_bw = -1;

	tmp->bw_start.tv_usec = 0;
	tmp->bw_start.tv_sec = 0;

	tmp->bw_end.tv_usec = 0;
	tmp->bw_end.tv_sec = 0;

	tmp->correlate_bw_start.tv_usec = 0;
	tmp->correlate_bw_start.tv_sec = 0;

	tmp->correlate_bw_end.tv_usec = 0;
	tmp->correlate_bw_end.tv_sec = 0;

	return( tmp );
}


void
free_netlink( ptr )
NETLINK *ptr;
{
	NETLINK L;

	if ( ptr == NULL || *ptr == NULL )
		return;

	L = *ptr;

	L->link = (GOBJ) NULL;

	L->msg_bytes = -1;
	L->correlate_bytes = -1;

	L->msg_bw = -1;
	L->correlate_bw = -1;

	L->bw_start.tv_usec = -1;
	L->bw_start.tv_sec = -1;

	L->bw_end.tv_usec = -1;
	L->bw_end.tv_sec = -1;

	L->correlate_bw_start.tv_usec = -1;
	L->correlate_bw_start.tv_sec = -1;

	L->correlate_bw_end.tv_usec = -1;
	L->correlate_bw_end.tv_sec = -1;

	free( L );

	*ptr = (NETLINK) NULL;
}


HOST_EXT
create_host_ext()
{
	HOST_EXT tmp;

	int i;

	tmp = (HOST_EXT) malloc( sizeof( struct host_ext_struct ) );
	trc_memcheck( tmp, "Host Extension Structure" );

	tmp->index = -1;

	tmp->status = -1;

	tmp->menuvar = (char *) NULL;

	tmp->nethost = (TRC_HOST) NULL;

	return( tmp );
}


void
free_host_ext( ptr )
HOST_EXT *ptr;
{
	HOST_EXT HE;

	if ( ptr == NULL || *ptr == NULL )
		return;

	HE = *ptr;

	HE->index = -1;

	HE->status = -1;

	HE->menuvar = (char *) NULL;

	HE->nethost = (TRC_HOST) NULL;

	free( HE );

	*ptr = (HOST_EXT) NULL;
}


HOST_EXT_DRAW
create_host_ext_draw()
{
	HOST_EXT_DRAW tmp;

	int i;

	tmp = (HOST_EXT_DRAW) malloc(
		sizeof( struct host_ext_draw_struct ) );
	trc_memcheck( tmp, "Host Extension Drawing Structure" );

	/* Arch Field */

	tmp->arch = (ARCH) NULL;

	/* Network Fields */

	tmp->status = -1;
	tmp->correlate_status = -1;

	for ( i=0 ; i < MAX_STATE ; i++ )
	{
		tmp->counts[i] = -1;
		tmp->correlate_counts[i] = -1;
	}

	tmp->color = (char *) NULL;

	tmp->icon = (GOBJ) NULL;
	tmp->nme = (GOBJ) NULL;
	tmp->box = (GOBJ) NULL;

	for ( i=0 ; i < MAX_LINK ; i++ )
		tmp->links[i] = (NETLINK) NULL;

	tmp->subnet = (NETWORK) NULL;

	return( tmp );
}


void
free_host_ext_draw( ptr )
HOST_EXT_DRAW *ptr;
{
	HOST_EXT_DRAW HED;

	int i;

	if ( ptr == NULL || *ptr == NULL )
		return;

	HED = *ptr;

	/* Arch Field */

	HED->arch = (ARCH) NULL;

	/* Network Fields */

	HED->status = -1;
	HED->correlate_status = -1;

	for ( i=0 ; i < MAX_STATE ; i++ )
	{
		HED->counts[i] = -1;
		HED->correlate_counts[i] = -1;
	}

	HED->color = (char *) NULL;

	HED->icon = (GOBJ) NULL;
	HED->nme = (GOBJ) NULL;
	HED->box = (GOBJ) NULL;

	for ( i=0 ; i < MAX_LINK ; i++ )
		HED->links[i] = (NETLINK) NULL;

	HED->subnet = (NETWORK) NULL;

	free( HED );

	*ptr = (HOST_EXT_DRAW) NULL;
}


TRC_HOST
get_host_index( index )
int index;
{
	TRC_HOST H;

	H = HOST_LIST;

	while ( H != NULL )
	{
		if ( ((HOST_EXT) H->ext)->index == index )
			return( H );

		H = H->next;
	}

	return( (TRC_HOST) NULL );
}


TRC_HOST
get_host_tid( host_list, tid )
TRC_HOST host_list;
int tid;
{
	TRC_HOST H;

	H = host_list;

	while ( H != NULL )
	{
		if ( H->pvmd_tid == tid )
			return( H );

		H = H->next;
	}

	return( (TRC_HOST) NULL );
}


NETWORK
create_network()
{
	NETWORK tmp;

	tmp = (NETWORK) malloc( sizeof( struct network_struct ) );
	trc_memcheck( tmp, "Network Structure" );

	tmp->type = -1;

	tmp->NET_C = (char *) NULL;

	tmp->host_list = (TRC_HOST) NULL;

	tmp->last_host = (TRC_HOST) NULL;

	tmp->links = (NETLINK *) NULL;

	tmp->nlinks = -1;

	return( tmp );
}


void
free_network( ptr )
NETWORK *ptr;
{
	NETWORK N;

	int i;

	if ( ptr == NULL || *ptr == NULL )
		return;

	N = *ptr;

	N->type = -1;

	N->NET_C = (char *) NULL;

	free_host_list( &(N->host_list) );

	N->last_host = (TRC_HOST) NULL;

	for ( i=0 ; i < MAX_LINK ; i++ )
	{
		if ( N->links[i] != NULL )
			free_netlink( &(N->links[i]) );
	}

	N->nlinks = -1;

	free( N );

	*ptr = (NETWORK) NULL;
}


void
free_host_list( host_list )
TRC_HOST *host_list;
{
	/* XXX */

	printf( "AAAAAIIIIEEEEE\n" );
}


void
read_hostfile()
{
	HOST_EXT HE;

	TRC_HOST H;

	FILE *fp;

	char globline[1024];
	char hostline[1024];
	char reftmp[1024];
	char msg[1024];
	char tmp[1024];

	int in_pvm;
	int done;
	int i, j;

	/* Add local host to list */

	if ( NEW_PVM && TRC_HOST_NAME != NULL )
	{
		H = HOST_LIST = trc_create_host();

		H->name = trc_copy_str( TRC_HOST_NAME );

		H->alias = trc_copy_str( TRC_HOST_ALIAS );

		H->refname = trc_copy_str( H->name );

		H->in_pvm = TO_BE_ADDED;

		H->ext = (void *) (HE = create_host_ext());

		HE->status = HOST_OFF;

		HE->index = NHOSTS++;
	}

	/* Initialize Global Options Line */

	strcpy( globline, "" );

	/* Get list of hosts */

	fp = fopen( HOSTFILE, "r" );

	if ( fp != NULL )
	{
		done = 0;

		do
		{
			i = 0;

			while ( (hostline[i] = getc( fp )) != (char) EOF
				&& hostline[i] != '\n' )
			{
				i++;
			}

			if ( hostline[i] != '\n' )
				done++;

			else
			{
				hostline[i] = '\0';

				if ( hostline[0] != '#' && strcmp( hostline, "" ) )
				{
					if ( hostline[0] == '*' )
						sprintf( globline, " %s", hostline + 1 );

					else
					{
						if ( hostline[0] == '&' )
						{
							sprintf( reftmp, "%s%s",
								hostline + 1, globline );
		
							in_pvm = TRC_NOT_IN_PVM;
						}
		
						else
						{
							sprintf( reftmp, "%s%s",
								hostline, globline );
		
							in_pvm = TO_BE_ADDED;
						}
		
						sscanf( reftmp, "%s", tmp );
	
						if ( !NEW_PVM || !TRC_LOCALHOST( tmp ) )
						{
							if ( HOST_LIST == NULL )
								H = HOST_LIST = trc_create_host();
		
							else
								H = H->next = trc_create_host();

							H->name = trc_copy_str( tmp );
		
							H->alias = trc_host_alias_str( H->name );
		
							H->refname = trc_copy_str( reftmp );
		
							H->in_pvm = in_pvm;

							H->ext = (void *) (HE = create_host_ext());

							HE->status = HOST_OFF;

							HE->index = NHOSTS++;
						}
					}
				}
			}
		}
		while ( !done );

		fclose( fp );
	}

	else
	{
		fprintf( stderr, "No Default Hostfile \"%s\" Found.\n",
			HOSTFILE );
	}
}


void
handle_hostfile_hosts()
{
	HOST_EXT HE;

	TRC_HOST H;

	H = HOST_LIST;

	while ( H != NULL )
	{
		/* Add in Any Missed Hosts from Host File */

		if ( H->in_pvm == TO_BE_ADDED )
		{
			if ( !TRC_LOCALHOST( H->name ) )
			{
				H->in_pvm = TRC_NOT_IN_PVM;

				do_add_host( H );
			}

			else
				H->in_pvm = TRC_IN_PVM;
		}

		/* Skip Flash On Initial Host Configuration */

		else if ( H->in_pvm == TRC_IN_PVM )
		{
			HE = (HOST_EXT) H->ext;

			if ( HE->status == HOST_NOTIFY_ADD )
			{
				HE->status = HOST_ON;

				if ( HE->nethost != NULL )
				{
					((HOST_EXT_DRAW) (HE->nethost->ext))->status
						= HOST_ON;
					
					REFRESH_GLOBAL( NET_FG_COLOR );

					SET_HOST_BOX( interp, MAIN_NET->NET_C, HE->nethost,
						CHAR_GLOBVAL( NET_FG_COLOR ), 1 );
				}
			}
		}

		H = H->next;
	}
}


void
set_trace_controls()
{
	char *str;

	int save;

	switch ( TRACE_STATUS )
	{
		case TRACE_REWIND:	str = "trace_controls rewind";	break;

		case TRACE_STOP:	str = "trace_controls stop";	break;

		case TRACE_FASTFWD:	str = "trace_controls fastfwd";		break;

		case TRACE_FWD:		str = "trace_controls fwd";		break;

		case TRACE_FWDSTEP:	str = "trace_controls fwdstep";	break;
	}

	save = TRACE_STATUS;

	TRACE_STATUS = TRACE_STOP;

	Tcl_Eval( interp, str );

	TRACE_STATUS = save;
}


int
check_mode_pending()
{
	if ( TRACE_MODE_PENDING == TRACE_MODE_NONE )
		return( FALSE );

	if ( TRACE_MODE_PENDING & TRACE_MODE_RESET )
	{
		do_reset_views( interp );

		TRACE_MODE_PENDING &= ~(TRACE_MODE_RESET);
	}

	if ( TRACE_MODE_PENDING & TRACE_MODE_CLEAR_HOSTS )
	{
		clear_network_hosts();

		TRACE_MODE_PENDING &= ~(TRACE_MODE_CLEAR_HOSTS);
	}

	if ( TRACE_MODE_PENDING )
	{
		printf( "\nError: Unknown Trace Mode Pending - 0x%x\n\n",
			TRACE_MODE_PENDING );
		
		TRACE_MODE_PENDING = TRACE_MODE_NONE;

		return( FALSE );
	}

	return( TRUE );
}


int
signal_by_name( name )
char *name;
{
	int i;

	for ( i=0 ; i < NUM_SIGNALS ; i++ )
	{
		if ( trc_compare( SIGNAL_STRS[i], name ) )
			return( i + 1 );
	}

	return( -1 );
}


int
ipwr( x, y )
int x;
int y;
{
	int z;
	int i;

	z = x;

	for ( i=1 ; i < y ; i++ )
		z *= x;
	
	return( z );
}


void
status_msg( ID, msg )
TRC_ID ID;
char *msg;
{
	char tmp[1024];

	sprintf( tmp, "setMsg { %s }", msg );

	Tcl_Eval( interp, tmp );
}


void
status_msg_safe( ID, msg )
TRC_ID ID;
char *msg;
{
	char tmp[1024];

	sprintf( tmp, "setMsgSafe { %s }", msg );

	Tcl_Eval( interp, tmp );
}


char *
task_label_str( T )
TASK T;
{
	TRC_HOST H;

	char tmp[1024];

	if ( T == NULL )
		strcpy( tmp, "" );

	else if ( T->host != NULL )
	{
		sprintf( tmp, "%s:%s TID=0x%x",
			host_name( T->host, FALSE ), T->name, T->tid );
	}

	else
	{
		H = get_host_tid( HOST_LIST, T->pvmd_tid );

		if ( H != NULL )
		{
			sprintf( tmp, "%s:%s TID=0x%x",
				host_name( H, FALSE ), T->name, T->tid );
		}

		else
		{
			sprintf( tmp, "(pvmd_tid=0x%x):%s TID=0x%x",
				T->pvmd_tid, T->name, T->tid );
		}
	}

	return( trc_copy_str( tmp ) );
}


char *
trunc_str( str, len )
char *str;
int len;
{
	char tmp[1024];

	int slen;

	slen = strlen( str );

	sprintf( tmp, "%s", str );

	if ( slen > len )
		sprintf( tmp + len - 1, "*" );

	return( trc_copy_str( tmp ) );
}


char *
upper_str( str )
char *str;
{
	char *ptr;
	char *tmp;

	tmp = trc_copy_str( str );

	ptr = tmp;

	while ( *ptr != '\0' )
	{
		if ( *ptr >= 'a' && *ptr <= 'z' )
			*ptr += 'A' - 'a';

		ptr++;
	}

	return( tmp );
}


int
append_cmd_str( result, str, maxlen, err, cmd_flag )
char *result;
char *str;
int maxlen;
char *err;
int cmd_flag;
{
	char *strcat();

	char *errstr;

	int elen;
	int rlen;
	int len;
	int rc;
	int i;

	rlen = strlen( result );

	len = rlen + strlen( str ) + 1;

	if ( len <= maxlen )
		strcat( result, str );

	else
	{
		if ( cmd_flag == TRUE )
		{
			if ( err == NULL )
			{
				printf( "\nError in Append Cmd String: Null Cmd\n\n" );

				return( TCL_ERROR );
			}

			elen = len + strlen( err ) + 5;

			errstr = (char *) malloc( (unsigned) elen * sizeof(char) );
			trc_memcheck( errstr, "Append Cmd String Err String" );

			sprintf( errstr, "%s { %s }", err, result );

			rc = Tcl_Eval( interp, errstr );

			if ( rc == TCL_ERROR )
				printf( "\nError Executing \"%s\"\n\n", errstr );

			free( errstr );

			if ( strlen( str ) <= maxlen )
				strcpy( result, str );

			else
			{
				printf( "\nError Append Cmd String \"%s\" Too Long\n\n",
					str );

				strcpy( result, "" );
			}

			return( rc );
		}

		else
		{
			if ( err != NULL )
				printf( "\nError in Append Cmd String: %s\n\n", err );

			i = rlen;

			while ( i < maxlen - 4 )
			{
				result[i] = str[ i - rlen ];

				i++;
			}

			sprintf( (char *) (result + (maxlen - 4)), "..." );
		}
	}

	return( TCL_OK );
}


void
error_exit()
{
	int cc;

#ifdef USE_PVM_33

	if ( MYTID > 0 && GROUPS_ALIVE )
	{
		cc = pvm_getinst( "xpvm", MYTID );

		if ( cc >= 0 )
		{
			cc = pvm_lvgroup( "xpvm" );

			if ( cc < 0 )
			{
				printf( "\nError Leaving XPVM Group, cc=%d\n\n",
					cc );
			}
		}
	}

#endif

	exit( -1 );
}

