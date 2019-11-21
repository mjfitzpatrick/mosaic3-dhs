
static char rcsid[] = 
	"$Id: commands.c,v 4.50 1998/04/09 21:11:34 kohl Exp $";

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


/* PVM Group Hack */

extern int gstid;


/* ARGSUSED */
int
hosts_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	HOST_EXT HE;

	TRC_HOST H;

	TRC_HOST last;

	char msg[1024];
	char tmp[1024];

	char *refname;

	int hindex;
	int found;
	int hid;
	int i;

	if ( argc < 2 )
	{
		Tcl_SetResult( itp,
			"usage: pvm_hosts [ <button_index> | 1 <other_host> ]",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	REFRESH_GLOBAL( NET_ALIVE_COLOR );

	hid = atoi( argv[1] );

	switch ( hid )
	{
		/* Add All Hosts */

		case ADD_ALL_HID:
		{
			add_all_hosts();

			break;
		}

		/* Add Other Host */

		case OTHER_HOST_HID:
		{
			/* Extract Regular Host Name */

			refname = trc_copy_str( argv[2] );

			sscanf( refname, "%s", tmp );

			/* Lookup Host */

			H = HOST_LIST;

			last = (TRC_HOST) NULL;

			found = 0;

			while ( H != NULL && !found )
			{
				if ( TRC_HOST_COMPARE( H, tmp ) )
					found++;

				else
				{
					last = H;

					H = H->next;
				}
			}

			/* Host already in list, add normally... */

			if ( found )
			{
				if ( H->in_pvm == TRC_NOT_IN_PVM )
				{
					/* Update Host Name and Reference Name */

					if ( strlen( tmp ) > strlen( H->name ) )
					{
						free( H->name );

						H->name = trc_copy_str( tmp );
					}

					free( H->refname );

					H->refname = refname;

					do_add_host( H );
				}

				else
				{
					sprintf( msg,
					   "setMsg { Error: Host %s Already Present! }",
					   tmp );

					Tcl_Eval( itp, msg );

					free( refname );
				}
			}

			/* Add Host to List */

			else
			{
				H = last->next = trc_create_host();

				H->refname = refname;

				H->name = trc_copy_str( tmp );

				H->alias = trc_host_alias_str( H->name );

				H->in_pvm = TRC_NOT_IN_PVM;

				H->ext = (void *) (HE = create_host_ext());

				HE->status = HOST_OFF;

				HE->index = NHOSTS++;

				Tcl_Eval( interp, "refreshHostsMenu" );

				do_add_host( H );
			}
		}

		/* Add Specific Host */

		default:
		{
			hindex = hid - MAX_HID;

			H = get_host_index( hindex );

			if ( H != NULL )
			{
				if ( H->in_pvm == TRC_IN_PVM )
					do_delete_host( H );

				else if ( H->in_pvm == TRC_NOT_IN_PVM )
					do_add_host( H );
			}
		}
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
tasks_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	struct pvmtaskinfo *T;
	struct pvmtaskinfo *tip;

	TRC_HOST H;

	char result[4096];
	char msg[4096];
	char tmp[1024];

	char *strcat();

	char **av;
	int ac;

	char *where;
	char *tstat;
	char *args;
	char *name;
	char *host;
	char *init;

	int *tids;

	int ntasks;
	int tmptid;
	int signal;
	int ntask;
	int stval;
	int flag;
	int save;
	int buf;
	int err;
	int tid;
	int cc;
	int n;
	int i;

	if ( !strcmp( argv[1], "spawn" ) )
	{
		LOCK_INTERFACE( save );

		init = trc_copy_str( argv[2] );

		name = trc_copy_str( argv[3] );

		args = trc_copy_str( argv[4] );

		buf = atoi( argv[5] );

		ntasks = atoi( argv[6] );

		/* Extract Args */

		Tcl_SplitList( interp, args, &ac, &av );

		/* Check Spawn Command Line Args */

		if ( buf != TRACE_BUF )
		{
			TRACE_BUF = buf;

			set_trace_buffer( TRACE_BUF, TRUE );
		}

		strcpy( tmp, "" );

		where = (char *) NULL;

#ifndef USE_PVM_33
		flag = PvmNoSpawnParent;
#else
		flag = 0;
#endif

		for ( i=7 ; i < argc ; i++ )
		{
			if ( !strcmp( argv[i], "debug" ) )
				flag |= PvmTaskDebug;

			else if ( !strcmp( argv[i], "trace" ) )
				flag |= PvmTaskTrace;

			else if ( !strcmp( argv[i], "front" ) )
				flag |= PvmMppFront;

			else if ( !strcmp( argv[i], "compl" ) )
				flag |= PvmHostCompl;

			else if ( !strcmp( argv[i], "host" ) )
			{
				flag |= PvmTaskHost;

				host = trc_copy_str( argv[i+1] );

				H = trc_get_host_name( host );

				if ( H != NULL )
				{
					where = trc_copy_str( H->refname );

					free( host );
				}

				else
					where = host;

				sprintf( tmp, " on %s", where );

				i++;
			}

			else if ( !strcmp( argv[i], "arch" ) )
			{
				flag |= PvmTaskArch;

				where = trc_copy_str( argv[i+1] );

				sprintf( tmp, " on %s", where );

				i++;
			}
		}

		sprintf( msg, "setMsg { Spawning %d cop%s of %s%s... }",
			ntasks, ( ntasks > 1 ? "ies" : "y" ), name, tmp );

		Tcl_Eval( interp, msg );

		/* Set Up Trace Stuff for Auto Play */

		if ( !TRACE_DO_SPAWN
			&& ( !strcmp( init, "fresh" )
				|| ( !strcmp( init, "append" )
					&& ( TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK
						|| ID->trace_out == NULL ) ) ) )
		{
			if ( do_reset_views( itp ) == TCL_ERROR )
			{
				UNLOCK_INTERFACE( save );

				return( TCL_ERROR );
			}

			TRACE_FILE_STATUS = TRACE_FILE_OVERWRITE;

			PROCESS_COUNTER_INTERVAL = PROCESS_COUNTER_OVERWRITE;
			RECV_COUNTER_INTERVAL = RECV_COUNTER_OVERWRITE;

			RECV_TIMER_INTERVAL = RECV_TIMER_OVERWRITE;

			SET_TCL_GLOBAL( itp, "trace_file_status", "OverWrite" );

			TRACE_DO_SPAWN = TRUE;

			if ( !setup_trace( itp ) )
			{
				UNLOCK_INTERFACE( save );

				return( TCL_OK );
			}
		}

		TRACE_DO_SPAWN = FALSE;

		/* Spawn Task(s) */

		tids = (int *) malloc( (unsigned) ntasks * sizeof(int) );
		trc_memcheck( tids, "Spawn Return TIDs Array" );

		if ( !( flag & PvmTaskTrace ) )
			pvm_setopt( PvmTraceTid, 0 );

		cc = pvm_spawn( name, av, flag, where, ntasks, tids );

		if ( !( flag & PvmTaskTrace ) )
			pvm_setopt( PvmTraceTid, MYTID );

		if ( cc <= 0 )
		{
			if ( cc < 0 )
				pvm_perror( "Error Spawning" );

			sprintf( msg, "setMsg { Error Spawning %s }", name );
		}

		else
		{
			sprintf( result, "%d Task%s of %s Spawned%s%s",
				cc, ( cc != 1 ? "s" : "" ), name, tmp,
				( cc > 0 ? ":" : "" ) );

			for ( i=0 ; i < cc ; i++ )
			{
				sprintf( tmp, " 0x%x", tids[i] );

				trc_append_str( result, tmp, 4096 );
			}

			sprintf( msg, "setMsg { %s }", result );
		}

		free( tids );

		free( init );

		free( name );

		Tcl_Eval( interp, msg );

		UNLOCK_INTERFACE( save );
	}

	else if ( !strcmp( argv[1], "otf" ) )
	{
		sscanf( argv[2], "%x", &tid );

		tstat = trc_copy_str( argv[3] );

		if ( !tid )
		{
			if ( !pvm_tasks( 0, &ntask, &tip ) && ntask > 0 )
			{
				tids = (int *) malloc( (unsigned) ntask * sizeof(int) );
				trc_memcheck( tids, "pvm_tasks() TIDs Array" );

				n = 0;

				for ( i=0 ; i < ntask ; i++ )
				{
					if ( (tmptid = tip[i].ti_tid) != MYTID )
						tids[n++] = tmptid;
				}

				set_task_tracing( tids, n, tstat );

				sprintf( tmp,
					"setMsg { All Tasks Tracing Adjusted. }" );

				free( tids );
			}
		}

		else
		{
			set_task_tracing( &tid, 1, tstat );

			sprintf( tmp,
				"setMsg { Task 0x%x Tracing Adjusted. }", tid );
		}

		free( tstat );

		Tcl_Eval( itp, tmp );
	}

	else if ( !strcmp( argv[1], "kill" ) )
	{
		sscanf( argv[2], "%x", &tid );

		name = trc_copy_str( argv[3] );

		if ( !tid )
		{
			if ( !pvm_tasks( 0, &ntask, &tip ) && ntask > 0 )
			{
				tids = (int *) malloc( (unsigned) ntask * sizeof(int) );
				trc_memcheck( tids, "pvm_tasks() TIDs Array" );

				for ( i=0 ; i < ntask ; i++ )
				{
					tids[i] = tip[i].ti_tid;

					if ( tids[i] != MYTID )
					{
						pvm_kill( tids[i] );

						if ( TRC_GROUPTASK( tip[i].ti_a_out ) )
							reset_groups();
					}
				}

				sprintf( tmp, "setMsg { All Tasks Killed. }" );

				for ( i=0 ; i < ntask ; i++ )
				{
					if ( tids[i] != MYTID )
						wait_kill( tids[i] );
				}

				free( tids );
			}
		}

		else
		{
			cc = pvm_kill( tid );

			if ( !cc )
			{
				sprintf( tmp, "setMsg { Task 0x%x Killed. }", tid );

				if ( TRC_GROUPTASK( name ) )
					reset_groups();

				wait_kill( tid );
			}

			else
			{
				pvm_perror( "Error Killing Task" );

				sprintf( tmp,
					"setMsg { Error Killing Task 0x%x }", tid );
			}
		}

		free( name );

		Tcl_Eval( itp, tmp );
	}

	else if ( !strcmp( argv[1], "signal" ) )
	{
		sscanf( argv[2], "%x", &tid );

		signal = signal_by_name( argv[3] );

		if ( !tid )
		{
			if ( !pvm_tasks( 0, &ntask, &tip ) )
			{
				err = 0;

				for ( i=0 ; i < ntask ; i++ )
				{
					T = &(tip[i]);

					if ( T->ti_tid != MYTID )
					{
						PVMCKERR( pvm_sendsig( T->ti_tid, signal ),
							"Error Signaling Task", err++ );
					}
				}

				if ( err )
					sprintf( tmp, "setMsg { Error Signaling Tasks. }" );

				else
					sprintf( tmp, "setMsg { All Tasks Signaled. }" );
			}
		}

		else
		{
			cc = pvm_sendsig( tid, signal );

			if ( cc < 0 )
			{
				pvm_perror( "Error Signaling Task" );

				sprintf( tmp,
					"setMsg { Error Signaling Task 0x%x %s (%d) }",
					tid, argv[3], signal );
			}

			else
			{
				sprintf( tmp, "setMsg { Task 0x%x Signaled %s (%d) }",
					tid, argv[3], signal );
			}
		}

		Tcl_Eval( itp, tmp );
	}

	else if ( !strcmp( argv[1], "systasks" ) )
	{
		if ( !strcmp( argv[3], "ON" ) )
			stval = TRUE;

		else
			stval = FALSE;

		if ( !strcmp( argv[2], "group" ) )
			ID->group_tasks = stval;
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
get_tasks_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TRC_HOST H;

	struct pvmtaskinfo *T;
	struct pvmtaskinfo *tip;

	char tmp[1024];

	char *a_out;
	char *type;

	int ntask;
	int i;

	if ( argc < 2 )
	{
		Tcl_SetResult( itp,
			"usage: get_tasks_proc <type>", TCL_STATIC );

		return( TCL_ERROR );
	}

	type = trc_copy_str( argv[1] );

	PVMCKERR( pvm_tasks( 0, &ntask, &tip ),
		"Error Obtaining Task Information", exit( -1 ) );

	for ( i=0 ; i < ntask ; i++ )
	{
		T = &(tip[i]);

		if ( T->ti_tid != MYTID )
		{
			if ( strcmp( T->ti_a_out, "" ) )
				a_out = T->ti_a_out;

			else
				a_out = "-";

			H = trc_get_host_tid( T->ti_host );

			if ( H != NULL )
			{
				sprintf( tmp, "add_list_task %s %s %s 0x%x",
					type, H->alias, a_out, T->ti_tid );
			}

			else
			{
				sprintf( tmp, "add_list_task %s ??? %s 0x%x",
					type, a_out, T->ti_tid );
			}

			Tcl_Eval( itp, tmp );
		}
	}

	return( TCL_OK );
}


void
wait_kill( tid )
int tid;
{
	struct pvmtaskinfo *T;
	struct pvmtaskinfo *tip;

	int safety;
	int found;
	int ntask;
	int cnt;
	int i;

	safety = 0;

	cnt = 0;

	do
	{
		found = 0;

		PVMCKERR( pvm_tasks( 0, &ntask, &tip ),
			"Error Waiting for Tasks to Die", exit( -1 ) );

		for ( i=0 ; i < ntask && !found ; i++ )
		{
			T = &(tip[i]);

			if ( T->ti_tid == tid )
			{
				found++;

				if ( !strcmp( T->ti_a_out, "" ) )
					cnt++;
			}
		}

		safety++;
	}
	while ( found && cnt < 10 && safety < 100 );
}


/* ARGSUSED */
int
reset_pvm_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	struct pvmtaskinfo *tip;

	int *noresets = (int *) NULL;
	int nnr = 0;

	int found;
	int ntask;
	int i, j;
	int inum;
	int tid;

	Tcl_Eval( itp, "setMsg { Resetting PVM... }" );

	/* Copied from console code - cons.c */

	PVMCKERR( pvm_tasks( 0, &ntask, &tip ),
		"Error Obtaining Task Information", exit( -1 ) );

#ifndef USE_PVM_33
	pvm_getnoresets( &noresets, &nnr );
#endif

	for ( i=0 ; i < ntask ; i++ )
	{
		tid = tip[i].ti_tid;

		for ( j=0, found=0 ; j < nnr && !found ; j++ )
			if ( noresets[j] == tid )
				found++;

		if ( !found && tid != 0 && tid != MYTID )
			pvm_kill( tid );
	}

	reset_groups();

#ifdef USE_PVM_33

	/* Re-add XPVM to "xpvm" group */

	inum = pvm_joingroup( "xpvm" );

	if ( inum < 0 )
		pvm_perror( "Error Joining XPVM Group" );

	else if ( inum != 0 )
	{
		printf( "\nWarning: XPVM Group Already Present, inum=%d\n\n",
			inum );
	}

	GROUPS_ALIVE = TRUE;

#endif

	/* Done */

	Tcl_Eval( itp, "setMsg { PVM Reset Done. }" );

	return( TCL_OK );
}


void
reset_groups()
{
#ifdef USE_PVM_33

	int i, j;

	while ( (i = pvm_lookup( "pvmgs", -1, &j )) >= 0 )
		pvm_delete( "pvmgs", i );

	gstid = -1;    /* Hack to force re-start of group servers... */

#endif

	GROUPS_ALIVE = FALSE;
}


/* ARGSUSED */
int
reset_views_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	int ret;

	ret = do_reset_views( itp );

	return( ret );
}


int
do_reset_views( itp )
Tcl_Interp *itp;
{
	UT_STATE Unext;
	UT_STATE U;

	MSG_GRID Gnext;
	MSG_GRID G;

	MSG Mnext;
	MSG M;

	TOBJ TTnext;
	TOBJ TT;

	TASK Tnext;
	TASK T;

	char cmd[1024];

	int i, j;

	if ( TRACE_PROCESS_LOCK == LOCKED )
	{
		TRACE_MODE_PENDING |= TRACE_MODE_RESET;

		return( TCL_OK );
	}

	Tcl_Eval( itp, "setMsg { Resetting XPVM Views... }" );

	/* Reset Interface */

	Tcl_Eval( itp, "reset_interface" );

	/* Reset Time Vars */

	CURRENT_TIME.tv_sec = -1;
	CURRENT_TIME.tv_usec = -1;

	BASE_TIME.tv_sec = -1;
	BASE_TIME.tv_usec = -1;

	/* Reset Pending Message List */

	M = PENDING_MSG_LIST;

	while ( M != NULL )
	{
		Mnext = M->next;

		free_msg( &M );

		M = Mnext;
	}

	PENDING_MSG_LIST = (MSG) NULL;

	/* Reset Pending Message Grid */

	for ( i=0 ; i < NTASKS ; i++ )
	{
		for ( j=0 ; j < NTASKS ; j++ )
		{
			G = PENDING_MSG_GRID[i][j];

			while ( G != NULL )
			{
				Gnext = G->next;

				free_msg_grid( &G );

				G = Gnext;
			}

			PENDING_MSG_GRID[i][j] = (MSG_GRID) NULL;
		}
	}

	/* Clear Space-Time Message Lines */

	M = MSG_LIST;

	while ( M != NULL )
	{
		if ( M->line != NULL )
		{
			DELETE_GOBJ( itp, ST_C, M->line );
		}

		Mnext = M->next;

		free_msg( &M );

		M = Mnext;
	}

	MSG_LIST = (MSG) NULL;

	/* Reset Task List */

	T = TASK_LIST;

	while ( T != NULL )
	{
		Tnext = T->next;

		free_task( &T );

		T = Tnext;
	}

	TASK_LIST = (TASK) NULL;

	NTASKS = 0;

	/* Reset Utilization State List */

	U = UT_LIST;

	while ( U != NULL )
	{
		Unext = U->next;

		free_ut_state( &U );

		U = Unext;
	}

	UT_LIST = (UT_STATE) NULL;

	/* Reset Task Output Text */

	TT = TASK_OUTPUT->text;

	while ( TT != NULL )
	{
		TTnext = TT->next;

		if ( TT->id != -1 )
		{
			DELETE_TOBJ( itp, TASK_OUTPUT->canvas, TT );
		}
		
		else
			free_tobj( &TT );

		TT = TTnext;
	}

	TASK_OUTPUT->last_text = (TOBJ) NULL;
	TASK_OUTPUT->text = (TOBJ) NULL;

	reset_search( TASK_OUTPUT );

	sprintf( cmd, "%s.filter_entry delete 0 end", TO_F );
	Tcl_Eval( itp, cmd );

	Tcl_Eval( itp, "set_output_filter" );

	/* Reset Trace Event History Text */

	TT = TASK_TEVHIST->text;

	while ( TT != NULL )
	{
		TTnext = TT->next;

		if ( TT->id != -1 )
		{
			DELETE_TOBJ( itp, TASK_TEVHIST->canvas, TT );
		}
		
		else
			free_tobj( &TT );

		TT = TTnext;
	}

	TASK_TEVHIST->last_text = (TOBJ) NULL;
	TASK_TEVHIST->text = (TOBJ) NULL;

	reset_search( TASK_TEVHIST );

	sprintf( cmd, "%s.filter_entry delete 0 end", TV_F );
	Tcl_Eval( itp, cmd );

	Tcl_Eval( itp, "set_tevhist_filter" );

	/* Set Result Message */

	Tcl_Eval( itp, "setMsg { XPVM Views Reset Done. }" );

	return( TCL_OK );
}


/* ARGSUSED */
int
reset_trace_file_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	setup_trace( itp );

	return( TCL_OK );
}


/* ARGSUSED */
void
quit_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	struct pvmtaskinfo *taskp;

	int ntasks;
	int found;
	int cc;
	int i;

	if ( MYTID > 0 )
	{

#ifdef USE_PVM_33

		if ( GROUPS_ALIVE )
		{
			PVMCKERR( pvm_tasks( 0, &ntasks, &taskp ),
				"Error Cleaning up XPVM Group", exit( -1 ) );

			found = 0;

			for ( i=0 ; i < ntasks && !found ; i++ )
			{
				if ( TRC_GROUPTASK( taskp[i].ti_a_out ) )
					found++;
			}

			if ( found )
			{
				cc = pvm_getinst( "xpvm", MYTID );

				if ( cc >= 0 )
				{
					cc = pvm_lvgroup( "xpvm" );

					if ( cc < 0 )
					{
						printf(
							"\nError Leaving XPVM Group, cc=%d\n\n",
							cc );
					}
				}
			}

			else
				printf( "Warning: Group Server Not Found...\n" );
		}

#endif

		printf( "Quitting xpvm - pvmd still running.\n" );

		pvm_exit();
	}

	exit( 0 );
}


/* ARGSUSED */
void
halt_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	pvm_halt();

	exit( 0 );
}


/* ARGSUSED */
int
playback_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char *strcat();

	char tmp[255];

	long ltmp;

	int index;
	int save;

	/* Protect Our Butts */

	LOCK_INTERFACE( save );

	/* Get New Trace Status */

	ltmp = (long) clientData;
	index = (int) ltmp;

	/* Apply Status Change */

	switch ( index )
	{
		case TRACE_REWIND:
		{
			if ( TRACE_FILE_STATUS == TRACE_FILE_OVERWRITE )
			{
				TRACE_FILE_STATUS = TRACE_FILE_PLAYBACK;

				PROCESS_COUNTER_INTERVAL = PROCESS_COUNTER_PLAYBACK;
				RECV_COUNTER_INTERVAL = RECV_COUNTER_PLAYBACK;

				RECV_TIMER_INTERVAL = RECV_TIMER_PLAYBACK;

				SET_TCL_GLOBAL( itp, "trace_file_status", "PlayBack" );
			}

			if ( ID->trace_in != NULL )
				fclose( ID->trace_in );

			do_trace_playback();

			if ( do_reset_views( itp ) == TCL_ERROR )
				return( TCL_ERROR );

			if ( TRACE_STATUS == TRACE_FASTFWD )
				redraw_views();

			break;
		}

		case TRACE_STOP:
		{
			if ( TRACE_STATUS == TRACE_FASTFWD )
				redraw_views();

			break;
		}

		case TRACE_FWD:
		case TRACE_FWDSTEP:
		{
			TRACE_ACTIVE = TRUE;

			if ( TRACE_STATUS == TRACE_FASTFWD )
				redraw_views();

			break;
		}

		case TRACE_FASTFWD:
		{
			TRACE_ACTIVE = TRUE;

			Tcl_Eval( itp, "raiseFastForwardFrames" );

			Tcl_Eval( itp, "setMsg { Fast-Forwarding Trace... }" );

			break;
		}
	}

	TRACE_STATUS = index;

	set_trace_controls();

	/* Restart Trace */

	UNLOCK_INTERFACE( save );

	return( TCL_OK );
}


void
redraw_views()
{
	int save;

	/* Protect Our Butts */

	LOCK_INTERFACE( save );

	/* Set Info Message */

	Tcl_Eval( interp, "setMsg { Drawing Views... }" );

	/* Redraw Views */

	redraw_network();

	redraw_spacetime();

	redraw_utilization();

	redraw_timelines();

	redraw_messagequeue();

	redraw_calltrace();

	redraw_taskoutput();

	redraw_tevhist();

	/* Set Info Message */

	Tcl_Eval( interp, "setMsg { Drawing Views... Done. }" );

	/* Hide Any Fast Forward Frames */

	Tcl_Eval( interp, "lowerFastForwardFrames" );

	/* Restart Trace */

	UNLOCK_INTERFACE( save );
}


/* ARGSUSED */
int
trace_overwrite_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: trace_overwrite_result (TRUE/FALSE)",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	if ( ID->trace_out != NULL )
		fclose( ID->trace_out );

	if ( ID->trace_in != NULL )
		fclose( ID->trace_in );

	if ( !strcmp( argv[1], "TRUE" ) )
	{
		do_trace_overwrite();

		if ( TRACE_DO_SPAWN )
			Tcl_Eval( itp, "do_spawn again" );
	}

	else
	{
		TRACE_FILE_STATUS = TRACE_FILE_PLAYBACK;

		PROCESS_COUNTER_INTERVAL = PROCESS_COUNTER_PLAYBACK;
		RECV_COUNTER_INTERVAL = RECV_COUNTER_PLAYBACK;

		RECV_TIMER_INTERVAL = RECV_TIMER_PLAYBACK;

		SET_TCL_GLOBAL( itp, "trace_file_status", "PlayBack" );

		do_trace_playback();

		TRACE_DO_SPAWN = FALSE;

		if ( TRACE_STATUS == TRACE_FASTFWD )
			redraw_views();

		TRACE_STATUS = TRACE_STOP;

		set_trace_controls();
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
get_initial_trace_file_status_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char *tfs;

	if ( TRACE_FILE_STATUS == TRACE_FILE_OVERWRITE )
		tfs = "OverWrite";
	
	else if ( TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
		tfs = "PlayBack";

	else
	{
		printf( "\nWarning in %s:  TRACE_FILE_STATUS not set...\n\n",
			"get_initial_trace_file_status_proc()" );

		tfs = "OverWrite";
	}

	Tcl_SetResult( itp, tfs, TCL_STATIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
trace_file_status_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	int newstatus;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: trace_file_status_handle <status>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	newstatus = -1;

	if ( !strcmp( argv[1], "OverWrite" ) )
	{
		TRACE_FILE_STATUS = TRACE_FILE_OVERWRITE;

		PROCESS_COUNTER_INTERVAL = PROCESS_COUNTER_OVERWRITE;
		RECV_COUNTER_INTERVAL = RECV_COUNTER_OVERWRITE;

		RECV_TIMER_INTERVAL = RECV_TIMER_OVERWRITE;

		newstatus = TRACE_FWD;
	}

	else if ( !strcmp( argv[1], "PlayBack" ) )
	{
		TRACE_FILE_STATUS = TRACE_FILE_PLAYBACK;

		PROCESS_COUNTER_INTERVAL = PROCESS_COUNTER_PLAYBACK;
		RECV_COUNTER_INTERVAL = RECV_COUNTER_PLAYBACK;

		RECV_TIMER_INTERVAL = RECV_TIMER_PLAYBACK;

		newstatus = TRACE_STOP;
	}

	if ( do_reset_views( itp ) == TCL_ERROR )
		return( TCL_ERROR );

	setup_trace( itp );

	if ( newstatus != -1 )
	{
		if ( TRACE_STATUS == TRACE_FASTFWD )
			redraw_views();

		TRACE_STATUS = newstatus;

		set_trace_controls();
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
trace_file_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: trace_file_handle <file_name>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	if ( ID->trace_file == NULL || strcmp( argv[1], ID->trace_file ) )
	{
		trc_set_trace_file( ID, argv[1] );

		TRACE_OVERWRITE_FLAG = FALSE;

		if ( do_reset_views( itp ) == TCL_ERROR )
			return( TCL_ERROR );

		setup_trace( itp );
	}

	return( TCL_OK );
}


int
setup_trace( itp )
Tcl_Interp *itp;
{
	int ret;

	ret = TRUE;

	if ( ID->trace_out != NULL )
	{
		fclose( ID->trace_out );

		ID->trace_out = (FILE *) NULL;
	}

	if ( ID->trace_in != NULL )
	{
		fclose( ID->trace_in );

		ID->trace_in = (FILE *) NULL;
	}

	TRACE_PENDING = 0;

	if ( ID->trace_file != NULL )
	{
		switch ( TRACE_FILE_STATUS )
		{
			case TRACE_FILE_OVERWRITE:
			{
				if ( check_trace_overwrite_exists() )
					do_trace_overwrite();

				else
					ret = FALSE;

				break;
			}

			case TRACE_FILE_PLAYBACK:
			{
				do_trace_playback();

				break;
			}
		}
	}

	return( ret );
}


void
do_trace_overwrite()
{
	int ecnt;
	int save;
	int fck;
	int i;

	/* Protect our butts */

	save = TRACE_STATUS;

	TRACE_STATUS = TRACE_STOP;

	/* Reset Trace File */

	fck = trc_reset_trace_file( ID );

	if ( fck )
	{
		ID->trace_in = fopen( ID->trace_file, "r" );
		trc_filecheck( ID->trace_in, ID->trace_file );
	}

	else
		ID->trace_in = (FILE *) NULL;

	/* Fix Hosts */

	restore_hosts_from_playback();

	ecnt = trc_save_host_status_events( ID );

	TRACE_OVERWRITE_FLAG = TRUE;

	TRACE_PENDING = ecnt;

	/* Restore Status */

	TRACE_STATUS = save;

	/* Check for Rewind - If Straight Back From Playback, Use FWD */

	if ( TRACE_STATUS == TRACE_REWIND )
	{
		TRACE_STATUS = TRACE_FWD;

		set_trace_controls();
	}
}


void
do_trace_playback()
{
	int save;

	/* Protect our butts */

	save = TRACE_STATUS;

	TRACE_STATUS = TRACE_STOP;

	/* Reset Trace File */

	ID->trace_out = (FILE *) NULL;

	ID->trace_in = fopen( ID->trace_file, "r" );
	trc_filecheck( ID->trace_in, ID->trace_file );

	/* Fix Hosts */

	clear_network_hosts();

	/* Reset Pending Event Counter */

	TRACE_PENDING = 0;

	/* Restore Status */

	TRACE_STATUS = save;
}


/* ARGSUSED */
int
get_trace_format_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char *result;

	if ( TRACE_FORMAT == TEV_FMT_33 )
		result = trc_copy_str( "3.3" );

	else if ( TRACE_FORMAT == TEV_FMT_34 )
		result = trc_copy_str( "3.4" );

	else
		result = trc_copy_str( "Unknown" );

	Tcl_SetResult( itp, result, TCL_DYNAMIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
define_trace_mask_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TRACE_MASK_GROUP *TMG_LIST;
	TRACE_MASK_GROUP TMG;

	TRACE_MASK_INDEX *TMI_LIST;
	TRACE_MASK_INDEX TMI;

	char *TM;

	char *formatstr;
	char *maskstr;
	char *liststr;

	char **av;
	int ac;

	int fmt;
	int set;
	int i;

	if ( argc < 3 )
	{
		Tcl_SetResult( itp,
		"usage: define_trace_mask [ 3.3 | 3.4 ] <name> <event_list>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	formatstr = argv[1];

	maskstr = argv[2];

	liststr = argv[3];

	if ( !strcmp( formatstr, "3.3" ) )
	{
		fmt = TEV_FMT_33;

		TMG_LIST = &TM33_GROUP_LIST;

		TMI_LIST = &TM33_INDEX_LIST;
	}

	else if ( !strcmp( formatstr, "3.4" ) )
	{
		fmt = TEV_FMT_34;

		TMG_LIST = &TM_GROUP_LIST;

		TMI_LIST = &TM_INDEX_LIST;
	}

	else
	{
		printf( "\nError: Unknown PVM format \"%s\"...\n\n",
			formatstr );

		return( TCL_OK );
	}

	set = FALSE;

	if ( !strcmp( maskstr, "DEFAULT" ) )
	{
		if ( fmt == TEV_FMT_33 )
			TM = TRACE33_MASK;

		else if ( fmt == TEV_FMT_34 )
			TM = TRACE_MASK;

		if ( TRACE_FORMAT == fmt )
			set = TRUE;
	}

	else
	{
		TMG = lookup_trace_mask_group( *TMG_LIST, maskstr );

		if ( TMG == NULL )
		{
			/* GROUP_LIST always starts non-null - w/ALL_EVENTS */

			TMG = create_trace_mask_group();

			TMG->next = *TMG_LIST;

			*TMG_LIST = TMG;

			TMG->name = trc_copy_str( maskstr );

			TMG->tmask = (char *) malloc( (unsigned)
				TRACE_MASK_LENGTH( fmt ) * sizeof(char) );
			trc_memcheck( TMG->tmask, "Group Trace Mask" );
		}

		else
		{
			printf( "\nError: Trace Mask Group %s Already Exists.\n\n",
				maskstr );
		}

		TM = TMG->tmask;
	}

	INIT_TRACE_MASK( TM, fmt );

	Tcl_SplitList( itp, liststr, &ac, &av );

	for ( i=0 ; i < ac ; i++ )
	{
		TMI = lookup_trace_mask_name( *TMI_LIST, av[i] );

		if ( TMI != NULL )
		{
			SET_TRACE_MASK( TM, fmt, TMI->index );
		}

#ifndef USE_PVM_33
		else
			printf( "Error: Trace Mask For \"%s\" Not Found\n", av[i] );
#endif
	}

	if ( set )
	{
		pvm_settmask( PvmTaskChild, TM );

#ifndef USE_PVM_33
		re_register_tracer( TM );
#endif

		set_trace_mask_buttons( itp, TM, *TMG_LIST, *TMI_LIST, fmt );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
trace_mask_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	TRACE_MASK_GROUP TMG;

	TRACE_MASK_INDEX TMI;

	char result[2048];
	char tmp[4096];

	char *onoffstr;
	char *evstr;

	int index;
	int onoff;
	int set;
	int i;

	if ( argc < 3 )
	{
		Tcl_SetResult( itp,
			"usage: pvm_trace_mask <event> [ ON | OFF ]",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	evstr = argv[1];

	onoffstr = argv[2];

	if ( !strcmp( onoffstr, "ON" ) )
		onoff = TRUE;

	else
		onoff = FALSE;

	set = FALSE;

	TMI = lookup_trace_mask_name( *CURRENT_INDEX_LIST, evstr );

	if ( TMI != NULL )
	{
		index = TMI->index;

		if ( onoff )
		{
			SET_TRACE_MASK( CURRENT_TRACE_MASK, TRACE_FORMAT, index );
		}

		else
		{
			UNSET_TRACE_MASK( CURRENT_TRACE_MASK, TRACE_FORMAT, index );
		}

		pvm_settmask( PvmTaskChild, CURRENT_TRACE_MASK );

		set = TRUE;
	}

	else
	{
		TMG = lookup_trace_mask_group( *CURRENT_GROUP_LIST, evstr );

		if ( TMG != NULL )
		{
			for ( i=FIRST_EVENT( TRACE_FORMAT ) ;
				i <= LAST_EVENT( TRACE_FORMAT ) ;
				i += EVENT_INCR( TRACE_FORMAT ) )
			{
				if ( CHECK_TRACE_MASK( TMG->tmask, TRACE_FORMAT, i ) )
				{
					if ( onoff )
					{
						SET_TRACE_MASK( CURRENT_TRACE_MASK,
							TRACE_FORMAT, i );
					}

					else
					{
						UNSET_TRACE_MASK( CURRENT_TRACE_MASK,
							TRACE_FORMAT, i );
					}
				}
			}

			pvm_settmask( PvmTaskChild, CURRENT_TRACE_MASK );

			set = TRUE;
		}

		else
			printf( "Trace Event %s Not Found In Mask\n", evstr );
	}

	if ( set )
	{
		set_trace_mask_buttons( itp, CURRENT_TRACE_MASK,
			*CURRENT_GROUP_LIST, *CURRENT_INDEX_LIST, TRACE_FORMAT );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
trace_buffer_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	int tbuf;

	if ( argc < 2 )
	{
		Tcl_SetResult( itp, "usage: set_trace_buffer <tbuf>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	tbuf = atoi( argv[1] );

	if ( tbuf != TRACE_BUF )
	{
		TRACE_BUF = tbuf;

		set_trace_buffer( TRACE_BUF, TRUE );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
task_out_update_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	int save;

	if ( TASK_OUTPUT->needs_redraw )
	{
		LOCK_INTERFACE( save );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Task Output... }" );

		update_twin_text( TASK_OUTPUT, FALSE );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Task Output... Done. }" );

		TASK_OUTPUT->needs_redraw = FALSE;

		UNLOCK_INTERFACE( save );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
output_file_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char *newchecked;
	char *file;

	char *type;

	char tmp[1024];

	int checked;
	int append;
	int nope;
	int new;

	int openit;

	/* Note - don't user tracer lib output file, do it custom */
	/*     to allow filtering, etc.                           */

	if ( argc != 3 )
	{
		sprintf( tmp, "arg count - usage: %s %s %s",
			"output_file_handle", "<file_name>",
			"[new|checked|append|nope]" );

		Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

		return( TCL_ERROR );
	}

	file = argv[1];
	newchecked = argv[2];

	checked = !strcmp( newchecked, "checked" );
	append = !strcmp( newchecked, "append" );
	nope = !strcmp( newchecked, "nope" );
	new = !strcmp( newchecked, "new" );

	openit = FALSE;

	if ( new )
	{
		if ( TASK_OUT_LOCK == LOCKED )
		{
			sprintf( tmp, "%s.file_entry delete 0 end", TO_F );
			Tcl_Eval( itp, tmp );

			sprintf( tmp, "%s.file_entry insert 0 %s",
				TO_F, TASK_OUTPUT->file );
			Tcl_Eval( itp, tmp );

			SET_TCL_GLOBAL( itp, "output_file", TASK_OUTPUT->file );

			return( TCL_OK );
		}

		if ( TASK_OUTPUT->file == NULL
			|| strcmp( file, TASK_OUTPUT->file ) )
		{
			if ( TASK_OUTPUT->file != NULL )
				free( TASK_OUTPUT->file );
		
			TASK_OUTPUT->file = trc_copy_str( file );

			if ( TASK_OUTPUT->fp != NULL )
				fclose( TASK_OUTPUT->fp );

			if ( strcmp( TASK_OUTPUT->file, "" ) )
			{
				sprintf( tmp, "output_file_handle %s",
					TASK_OUTPUT->file );

				if ( check_file_overwrite_exists( TASK_OUTPUT->file,
					tmp ) )
				{
					type = "w";

					openit = TRUE;
				}
			}

			else
			{
				free( TASK_OUTPUT->file );

				TASK_OUTPUT->file = (char *) NULL;
			}
		}
	}

	else
	{
		TASK_OUT_LOCK = UNLOCKED;

		if ( checked )
		{
			type = "w";

			openit = TRUE;
		}

		else if ( append )
		{
			type = "a";

			openit = TRUE;
		}

		else if ( nope )
			Tcl_Eval( itp, "output_file_handle {} new" );
	}

	if ( openit )
	{
		if ( TASK_OUTPUT->fp != NULL )
			fclose( TASK_OUTPUT->fp );

		TASK_OUTPUT->fp = fopen( TASK_OUTPUT->file, type );

		if ( !trc_filecheck( TASK_OUTPUT->fp, TASK_OUTPUT->file ) )
		{
			sprintf( tmp, "Error Opening Output File \"%s\"",
				TASK_OUTPUT->file );

			Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

			return( TCL_ERROR );
		}

		update_twin_text( TASK_OUTPUT, TRUE );
	}

	return( TCL_OK );
}


int
check_file_overwrite_exists( fname, cmd )
char *fname;
char *cmd;
{
	FILE *fp;

	char tmp[1024];

	fp = fopen( fname, "r" );

	if ( fp != NULL )
	{
		fclose( fp );

		TASK_OUT_LOCK = LOCKED;

		sprintf( tmp, "prompt_file_overwrite %s {%s}", fname, cmd );

		Tcl_Eval( interp, tmp );

		return( FALSE );
	}

	else
		return( TRUE );
}


/* ARGSUSED */
int
file_overwrite_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[1024];

	char *result;
	char *cmd;

	if ( argc != 3 )
	{
		Tcl_SetResult( itp,
	"arg count - usage: file_overwrite_result (TRUE/FALSE/APPEND) cmd",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	result = argv[1];
	cmd = argv[2];

	if ( !strcmp( result, "TRUE" ) )
	{
		sprintf( tmp, "%s.file_entry delete 0 end", TO_F );
		Tcl_Eval( itp, tmp );

		sprintf( tmp, "%s.file_entry insert 0 %s",
			TO_F, TASK_OUTPUT->file );
		Tcl_Eval( itp, tmp );

		SET_TCL_GLOBAL( itp, "output_file", TASK_OUTPUT->file );

		sprintf( tmp, "%s checked", cmd );
		Tcl_Eval( itp, tmp );
	}

	else if ( !strcmp( result, "APPEND" ) )
	{
		sprintf( tmp, "%s.file_entry delete 0 end", TO_F );
		Tcl_Eval( itp, tmp );

		sprintf( tmp, "%s.file_entry insert 0 %s",
			TO_F, TASK_OUTPUT->file );
		Tcl_Eval( itp, tmp );

		SET_TCL_GLOBAL( itp, "output_file", TASK_OUTPUT->file );

		sprintf( tmp, "%s append", cmd );
		Tcl_Eval( itp, tmp );
	}

	else if ( !strcmp( result, "FALSE" ) )
	{
		sprintf( tmp, "%s.file_entry delete 0 end", TO_F );
		Tcl_Eval( itp, tmp );

		SET_TCL_GLOBAL( itp, "output_file", "" );

		sprintf( tmp, "%s nope", cmd );
		Tcl_Eval( itp, tmp );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
output_filter_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[1024];

	char *newchecked;
	char *filter;

	char *type;

	int checked;
	int append;
	int nope;
	int new;

	int openit;

	if ( argc != 2 && argc != 3 )
	{
		sprintf( tmp, "arg count - usage: %s %s %s",
			"output_filter_handle", "<filter>",
			"[new|checked|append|nope]" );

		Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

		return( TCL_ERROR );
	}

	if ( argc == 2 )
	{
		filter = "";
		newchecked = argv[1];
	}

	else
	{
		filter = argv[1];
		newchecked = argv[2];
	}

	checked = !strcmp( newchecked, "checked" );
	append = !strcmp( newchecked, "append" );
	nope = !strcmp( newchecked, "nope" );
	new = !strcmp( newchecked, "new" );

	openit = FALSE;

	if ( new )
	{
		if ( TASK_OUT_LOCK == LOCKED )
		{
			sprintf( tmp, "%s.filter_entry delete 0 end", TO_F );
			Tcl_Eval( itp, tmp );

			sprintf( tmp, "%s.filter_entry insert 0 %s",
				TO_F, TASK_OUTPUT->filter );
			Tcl_Eval( itp, tmp );

			SET_TCL_GLOBAL( itp, "output_filter", TASK_OUTPUT->filter );

			return( TCL_OK );
		}

		if ( TASK_OUTPUT->filter == NULL
			|| strcmp( filter, TASK_OUTPUT->filter ) )
		{
			if ( TASK_OUTPUT->filter != NULL )
				free( TASK_OUTPUT->filter );

			TASK_OUTPUT->filter = trc_copy_str( filter );

#if ( TCL_MAJOR_VERSION > 7 ) || ( TCL_MINOR_VERSION >= 4 )

			TASK_OUTPUT->regexp = Tcl_RegExpCompile( itp,
				TASK_OUTPUT->filter );

#endif

			twin_clear( TASK_OUTPUT );

			sprintf( tmp, "output_filter_handle %s",
				TASK_OUTPUT->filter );

			if ( TASK_OUTPUT->file != NULL
				&& check_file_overwrite_exists( TASK_OUTPUT->file,
					tmp ) )
			{
				type = "w";

				openit = TRUE;
			}
		}
	}

	else
	{
		TASK_OUT_LOCK = UNLOCKED;

		if ( checked )
		{
			type = "w";

			openit = TRUE;
		}

		else if ( append )
		{
			type = "a";

			openit = TRUE;
		}

		else if ( nope )
			Tcl_Eval( itp, "output_file_handle {} new" );
	}

	if ( openit )
	{
		if ( TASK_OUTPUT->fp != NULL )
			fclose( TASK_OUTPUT->fp );

		TASK_OUTPUT->fp = fopen( TASK_OUTPUT->file, type );

		if ( !trc_filecheck( TASK_OUTPUT->fp, TASK_OUTPUT->file ) )
		{
			sprintf( tmp, "Error Opening Output File \"%s\"",
				TASK_OUTPUT->file );

			Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

			return( TCL_ERROR );
		}

		update_twin_text( TASK_OUTPUT, TRUE );
	}

	else
		update_twin_text( TASK_OUTPUT, FALSE );

	return( TCL_OK );
}


/* ARGSUSED */
int
output_search_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	SEARCH S;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: output_search_handle <target>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	S = TASK_OUTPUT->search;

	if ( S->search == NULL || strcmp( argv[1], S->search ) )
	{
		if ( S->search != NULL )
			free( S->search );

		S->search = trc_copy_str( argv[1] );

#if ( TCL_MAJOR_VERSION > 7 ) || ( TCL_MINOR_VERSION >= 4 )

		S->regexp = Tcl_RegExpCompile( itp, S->search );

#endif

		S->changed = TRUE;
	}

	else
		S->changed = FALSE;

	search_twin( TASK_OUTPUT );

	return( TCL_OK );
}


/* ARGSUSED */
int
task_tevhist_update_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	int save;

	if ( TASK_TEVHIST->needs_redraw )
	{
		LOCK_INTERFACE( save );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Event History... }" );

		update_twin_text( TASK_TEVHIST, FALSE );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Event History... Done. }" );

		TASK_TEVHIST->needs_redraw = FALSE;

		UNLOCK_INTERFACE( save );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
tevhist_filter_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: tevhist_filter_handle <filter>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	if ( TASK_TEVHIST->filter == NULL
		|| strcmp( argv[1], TASK_TEVHIST->filter ) )
	{
		if ( TASK_TEVHIST->filter != NULL )
			free( TASK_TEVHIST->filter );

		TASK_TEVHIST->filter = trc_copy_str( argv[1] );

#if ( TCL_MAJOR_VERSION > 7 ) || ( TCL_MINOR_VERSION >= 4 )

		TASK_TEVHIST->regexp = Tcl_RegExpCompile( itp,
			TASK_TEVHIST->filter );

#endif

		twin_clear( TASK_TEVHIST );

		update_twin_text( TASK_TEVHIST, FALSE );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
tevhist_search_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	SEARCH S;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: tevhist_search_handle <target>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	S = TASK_TEVHIST->search;

	if ( S->search == NULL || strcmp( argv[1], S->search ) )
	{
		if ( S->search != NULL )
			free( S->search );

		S->search = trc_copy_str( argv[1] );

#if ( TCL_MAJOR_VERSION > 7 ) || ( TCL_MINOR_VERSION >= 4 )

		S->regexp = Tcl_RegExpCompile( itp, S->search );

#endif

		S->changed = TRUE;
	}

	else
		S->changed = FALSE;

	search_twin( TASK_TEVHIST );

	return( TCL_OK );
}


/* ARGSUSED */
int
toggle_search_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	SEARCH S;

	TWIN TW;

	char tmp[2048];

	char *layout;
	char *label;
	char *twin;
	char *str;

	if ( argc != 4 )
	{
		Tcl_SetResult( itp,
			"arg count - usage: toggle_search <twin> <label> <layout>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	twin = argv[1];
	label = argv[2];
	layout = argv[3];

	/* Get Text Window Struct */

	if ( !strcmp( twin, "output" ) )
		TW = TASK_OUTPUT;
	
	else if ( !strcmp( twin, "tevhist" ) )
		TW = TASK_TEVHIST;
	
	else
	{
		printf( "Warning: Unknown Text Window \"%s\"\n", twin );

		return( TCL_OK );
	}

	/* Toggle Search Flag */

	S = TW->search;

	if ( S->direction == SEARCH_FORWARD )
	{
		S->direction = SEARCH_BACKWARD;

		str = "(Backward)";
	}
	
	else
	{
		S->direction = SEARCH_FORWARD;

		str = "(Forward)";
	}

	/* Update Interface */

	sprintf( tmp, "%s configure -text %s", label, str );

	Tcl_Eval( itp, tmp );

	Tcl_Eval( itp, layout );

	return( TCL_OK );
}


/* ARGSUSED */
int
get_hosts_menu_list( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	HOST_EXT HE;

	TRC_HOST H;

	char *cmd_argv[MAX_HID];
	char *tmparg[4];
	char *merge[2];

	char **list_argv;

	char *label;
	char *state;
	char *help;

	char *result;
	char *hosts;
	char *cmds;

	char tmp[1024];

	int i;

	/* Host Commands */

	for ( i=0 ; i < MAX_HID ; i++ )
	{
		tmparg[0] = HOST_CMD_STRS[i];

		sprintf( tmp, "%d", i );
		tmparg[1] = trc_copy_str( tmp );

		tmparg[2] = HOST_CMD_HELP[i];

		cmd_argv[i] = Tcl_Merge( 3, tmparg );

		free( tmparg[1] );
	}

	cmds = Tcl_Merge( MAX_HID, cmd_argv );

	/* Hosts */

	if ( NHOSTS > 0 )
	{
		list_argv = (char **) malloc( (unsigned) NHOSTS
			* sizeof(char *) );
		trc_memcheck( list_argv, "Menu List Argv" );

		H = HOST_LIST;

		i = 0;

		while ( H != NULL )
		{
			if ( !trc_compare( "0x", H->name ) )
			{
				label = host_name( H, FALSE );

				if ( H->in_pvm == TRC_IN_PVM )
					state = trc_copy_str( "ON" );

				else
					state = trc_copy_str( "OFF" );

				help = host_name( H, TRUE );

				HE = (HOST_EXT) H->ext;

				tmparg[0] = label;

				sprintf( tmp, "%d", MAX_HID + HE->index );
				tmparg[1] = trc_copy_str( tmp );

				tmparg[2] = state;

				sprintf( tmp, "%s %s", HOST_MENU_HELP_STR, help );
				tmparg[3] = trc_copy_str( tmp );

				list_argv[i] = Tcl_Merge( 4, tmparg );

				free( tmparg[1] );
				free( tmparg[3] );

				free( label );
				free( state );
				free( help );

				i++;
			}

			H = H->next;
		}

		hosts = Tcl_Merge( i, list_argv );
	}

	else
		hosts = trc_copy_str( "" );

	/* Return Combined List as Result */

	merge[0] = cmds;
	merge[1] = hosts;

	result = Tcl_Merge( 2, merge );

	free( cmds );
	free( hosts );

	Tcl_SetResult( itp, result, TCL_DYNAMIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
load_bitmap_file( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	Pixmap bitmap;

	char tmp[1024];

	if ( argc != 2 )
	{
		Tcl_SetResult( itp, "usage: load_bitmap_file <file>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	sprintf( tmp, "@%s", argv[1] );

	bitmap = Tk_GetBitmap( itp, Top, Tk_GetUid( tmp ) );

	if ( bitmap <= 0 )
	{
		sprintf( tmp, "Error Opening Bitmap %s", argv[1] );

		Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

		return( TCL_ERROR );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
title_info_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[255];

	sprintf( tmp, "%s  (PVM %s)  [TID=0x%x]",
		XPVM_VERSION, pvm_version(), MYTID );

	Tcl_SetResult( itp, tmp, TCL_STATIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
define_arch( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	ARCH A;

	Pixmap bitmap;

	char tmp[1024];

	int height;
	int width;

	if ( argc != 3 )
	{
		Tcl_SetResult( itp, "usage: define_arch <code> <file>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	if ( ARCH_LIST == NULL )
		A = ARCH_LIST = create_arch();

	else
	{
		A = ARCH_LIST;

		while ( A->next != NULL )
			A = A->next;

		A->next = create_arch();

		A = A->next;
	}

	A->code = trc_copy_str( argv[1] );

	sprintf( tmp, "@%s", argv[2] );

	A->name = trc_copy_str( tmp );

	bitmap = Tk_GetBitmap( itp, Top, Tk_GetUid( tmp ) );

	if ( bitmap <= 0 )
	{
		sprintf( tmp, "Error Opening Bitmap %s", argv[2] );

		Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

		return( TCL_ERROR );
	}

#if ( TCL_MAJOR_VERSION > 7 ) || ( TCL_MINOR_VERSION >= 4 )

	Tk_SizeOfBitmap( Disp, bitmap,
		(int *) &width, (int *) &height );

#else

	Tk_SizeOfBitmap( Disp, bitmap,
		(unsigned *) &width, (unsigned *) &height );

#endif

	A->height = height;
	A->width = width;

	return( TCL_OK );
}


/* ARGSUSED */
int
pad_cmd( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char *result;
	char *ptr;
	char *str;

	int len;
	int nd;
	int nz;
	int i;

	if ( argc < 3 )
	{
		Tcl_SetResult( itp,
			"usage: pad <number_str> <desired_length>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	str = argv[1];

	len = atoi( argv[2] );

	nd = strlen( str );

	nz = len - nd;

	result = (char *) malloc( (unsigned) (len + 1) * sizeof(char) );
	trc_memcheck( result, "Numerical Pad String" );

	ptr = result;

	for ( i=0 ; i < nz ; i++ )
		*ptr++ = '0';

	sprintf( (char *) (result + nz), "%s", str );

	Tcl_SetResult( itp, result, TCL_DYNAMIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
strip_label_cmd( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[1024];

	char *result;
	char *ptr;
	char *str;

	int len;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp, "usage: strip_label <str>", TCL_STATIC );

		return( TCL_ERROR );
	}

	str = argv[1];

	ptr = tmp;

	while ( *str != '\0' )
	{
		if ( *str == '-' || *str == ' ' || *str == '.' )
			*ptr++ = '_';

		else
			*ptr++ = *str;

		str++;
	}

	*ptr = '\0';

	result = trc_copy_str( tmp );

	Tcl_SetResult( itp, result, TCL_DYNAMIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
fix_help_line_cmd( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[1024];

	char *result;
	char *ptr;
	char *str;

	int len;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp, "usage: fix_help_line <line>", TCL_STATIC );

		return( TCL_ERROR );
	}

	str = argv[1];

	ptr = tmp;

	while ( *str != '\0' )
	{
		if ( *str == '' )
			*ptr++ = '{';

		else if ( *str == '' )
			*ptr++ = '}';

		else
			*ptr++ = *str;

		str++;
	}

	*ptr = '\0';

	result = trc_copy_str( tmp );

	Tcl_SetResult( itp, result, TCL_DYNAMIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
set_query_time_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char cmd[1024];
	char tmp[255];

	char *usec_pad;

	double qt;

	int qt_usec;
	int qt_sec;
	int x;

	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FRAME_OFFSET );
	REFRESH_GLOBAL( SCALE );

	x = atoi( argv[1] );

	TIMEIDX = ( x >= 0 ) ? x : -1;

	if ( x >= 0 )
	{
		TIMEIDX += INT_GLOBVAL( FRAME_BORDER )
			* ( 1 + INT_GLOBVAL( FRAME_OFFSET ) );

		qt = ( (double) x ) * ( (double) INT_GLOBVAL( SCALE ) );

		qt_sec = (int) ( qt / (double) 1000000.0 );

		qt_usec = (int) ( qt
			- ( ( (double) qt_sec ) * ( (double) 1000000.0 ) ) );

		usec_pad = trc_pad_num( qt_usec, 6 );

		sprintf( tmp, "Query Time: %d.%s", qt_sec, usec_pad );

		free( usec_pad );
	}

	else
		tmp[0] = '\0';

	sprintf( cmd,
		"%s.query_frame.query configure -text {View Info:   %s}",
		ST, tmp );

	Tcl_Eval( itp, cmd );

	SET_TCL_GLOBAL( itp, "st_query_text_scroll", "0" );

	SET_TCL_GLOBAL( itp, "st_query_text", tmp );

	return( TCL_OK );
}


/* ARGSUSED */
int
correlate_views_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	static int was_reset = 1;

	char cmd[1024];
	char tmp[255];

	char *usec_pad;
	char *status;

	int ct_usec;
	int ct_sec;
	int ct;
	int x;

	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FRAME_OFFSET );
	REFRESH_GLOBAL( SCALE );

	x = atoi( argv[1] );

	TIMEIDX = ( x >= 0 ) ? x : -1;

	if ( x >= 0 )
	{
		TIMEIDX += INT_GLOBVAL( FRAME_BORDER )
			* ( 1 + INT_GLOBVAL( FRAME_OFFSET ) );

		CORRELATE_LOCK = LOCKED;

		ct = ( (double) x ) * ( (double) INT_GLOBVAL( SCALE ) );

		ct_sec = (int) ( ct / (double) 1000000.0 );

		ct_usec = (int) ( ct
			- ( ( (double) ct_sec ) * ( (double) 1000000.0 ) ) );

		usec_pad = trc_pad_num( ct_usec, 6 );

		sprintf( tmp, "Correlate Time: %d.%s", ct_sec, usec_pad );

		free( usec_pad );

		do_time_correlate( ct_sec, ct_usec );

		status = "setMsg { Correlate Mode Activated... }";

		was_reset = 0;
	}

	else
	{
		if ( was_reset++ )
			return( TCL_OK );

		do_time_correlate( -1, -1 );

		CORRELATE_LOCK = UNLOCKED;

		redraw_network();

		if ( CORRELATE_LOCK == LOCKED )
			return( TCL_OK );

		redraw_calltrace();

		if ( CORRELATE_LOCK == LOCKED )
			return( TCL_OK );

		redraw_messagequeue();

		if ( CORRELATE_LOCK == LOCKED )
			return( TCL_OK );

		tmp[0] = '\0';

		status = "setMsg { Views Restored After Correlate }";
	}

	sprintf( cmd,
		"%s.query_frame.query configure -text {View Info:   %s}",
		ST, tmp );

	Tcl_Eval( itp, cmd );

	SET_TCL_GLOBAL( itp, "st_query_text_scroll", "0" );

	SET_TCL_GLOBAL( itp, "st_query_text", tmp );

	Tcl_Eval( itp, status );

	return( TCL_OK );
}


/* ARGSUSED */
int
time_zoom_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	int new_scale;
	int timex;

	new_scale = atoi( argv[1] );

	timex = X_OF_TIME( CURRENT_TIME.tv_sec, CURRENT_TIME.tv_usec,
		new_scale );

	CK_X_TIME( timex, CURRENT_TIME.tv_sec, CURRENT_TIME.tv_usec );

	set_view_time( timex, TRUE );

	return( TCL_OK );
}


/* ARGSUSED */
int
check_max_scale_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	int new_scale;
	int max_int;

	new_scale = atoi( argv[1] );

	/* Generate Maximum Positive Integer */

	max_int = ipwr( 2, ( ((int) sizeof( int )) * 8 ) - 1 ) - 1;

	/* Assume maximum canvas width of 1280 */

	if ( new_scale >= max_int / 1280 )
		Tcl_SetResult( itp, "0", TCL_STATIC );

	else
		Tcl_SetResult( itp, "1", TCL_STATIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
double_click_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	static struct timeval last = { -1, -1 };

	struct timeval tmp;

	int diff;

	gettimeofday( &tmp, (struct timezone *) NULL );

	diff = TIME_ELAPSED( last, tmp );

	if ( diff > 0 && diff < 500000 )
		Tcl_SetResult( itp, "1", TCL_STATIC );

	else
		Tcl_SetResult( itp, "0", TCL_STATIC );

	last.tv_sec = tmp.tv_sec;
	last.tv_usec = tmp.tv_usec;

	return( TCL_OK );
}


/* ARGSUSED */
int
interface_lock_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	static int save = -1;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp,
			"usage: interface_lock_proc < lock | unlock >",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	if ( !strcmp( argv[1], "lock" ) )
	{
		if ( save != -1 )
		{
			Tcl_SetResult( itp, "Lock Already Set",
				TCL_STATIC );

			return( TCL_ERROR );
		}

		LOCK_INTERFACE( save );

		Tcl_SetResult( itp,
			( INTERFACE_LOCK == LOCKED ) ? "locked" : "unlocked",
			TCL_STATIC );
	}

	else if ( !strcmp( argv[1], "unlock" ) )
	{
		if ( save == -1 )
		{
			Tcl_SetResult( itp, "Lock Not Set",
				TCL_STATIC );

			return( TCL_ERROR );
		}

		UNLOCK_INTERFACE( save );

		save = -1;

		Tcl_SetResult( itp,
			( INTERFACE_LOCK == LOCKED ) ? "locked" : "unlocked",
			TCL_STATIC );
	}

	else if ( !strcmp( argv[1], "query" ) )
	{
		Tcl_SetResult( itp,
			( INTERFACE_LOCK == LOCKED ) ? "locked" : "unlocked",
			TCL_STATIC );
	}

	else
	{
		Tcl_SetResult( itp,
			"usage: interface_lock_proc < lock | unlock >",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
set_net_volume_levels_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[1024];

	int i;

	NET_VOLUME_COLOR_SIZE = MAKE_TCL_GLOBAL( "net_volume_color_size",
		TCL_GLOBAL_INT );

	REFRESH_GLOBAL( NET_VOLUME_COLOR_SIZE );

	NET_VOLUME_COLOR = (TCL_GLOBAL *) malloc( (unsigned)
		INT_GLOBVAL( NET_VOLUME_COLOR_SIZE ) * sizeof( TCL_GLOBAL ) );
	trc_memcheck( NET_VOLUME_COLOR, "Network Volume Colors" );

	for ( i=0 ; i < INT_GLOBVAL( NET_VOLUME_COLOR_SIZE ) ; i++ )
	{
		sprintf( tmp, "net_volume_color(%d)", i );

		NET_VOLUME_COLOR[i] = MAKE_TCL_GLOBAL( trc_copy_str( tmp ),
			TCL_GLOBAL_CHAR );
	}

	NET_VOLUME_LEVEL = (int *) malloc( (unsigned)
		INT_GLOBVAL( NET_VOLUME_COLOR_SIZE ) * sizeof( int ) );
	trc_memcheck( NET_VOLUME_LEVEL, "Network Volume Levels" );

	for ( i=0 ; i < INT_GLOBVAL( NET_VOLUME_COLOR_SIZE ) ; i++ )
	{
		switch ( i )
		{
			case 0:	NET_VOLUME_LEVEL[i] = 1;				break;
			case 1:	NET_VOLUME_LEVEL[i] = NBYTES_1K / 16;	break;
			case 2:	NET_VOLUME_LEVEL[i] = NBYTES_1K / 8;	break;
			case 3:	NET_VOLUME_LEVEL[i] = NBYTES_1K / 4;	break;
			case 4:	NET_VOLUME_LEVEL[i] = NBYTES_1K;		break;
			case 5:	NET_VOLUME_LEVEL[i] = NBYTES_1K * 10;	break;
			case 6:	NET_VOLUME_LEVEL[i] = NBYTES_1K * 100;	break;
			case 7:	NET_VOLUME_LEVEL[i] = NBYTES_1M;		break;

			default:
			{
				NET_VOLUME_LEVEL[i] = NET_VOLUME_LEVEL[ i - 1 ] * 10;
			}
		}
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
get_net_volume_level_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	char tmp[1024];

	int index;

	if ( argc != 2 )
	{
		Tcl_SetResult( itp, "usage: get_net_volume_level <index>",
			TCL_STATIC );

		return( TCL_ERROR );
	}

	REFRESH_GLOBAL( NET_VOLUME_COLOR_SIZE );

	index = atoi( argv[1] );

	if ( index >= 0 && index < INT_GLOBVAL( NET_VOLUME_COLOR_SIZE ) )
	{
		sprintf( tmp, "%d", NET_VOLUME_LEVEL[ index ] );

		Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

		return( TCL_OK );
	}

	else
	{
		sprintf( tmp, "Network Volume Level Index %d - Out of Range",
			index );

		Tcl_SetResult( itp, trc_copy_str( tmp ), TCL_DYNAMIC );

		return( TCL_ERROR );
	}
}

