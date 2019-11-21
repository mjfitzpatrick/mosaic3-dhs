
static char rcsid[] = 
	"$Id: pvm.c,v 4.50 1998/04/09 21:11:48 kohl Exp $";

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
restart_coroutines()
{
	Tk_DoWhenIdle( recv_event_proc, (ClientData) NULL );
}


void
recv_timer_proc( clientData )
ClientData clientData;
{
	Tk_DoWhenIdle( recv_event_proc, clientData );
}


void
recv_event_proc( clientData )
ClientData clientData;
{
	int activity;
	int status;
	int ecnt;

	/* printf( "+" ); fflush( stdout ); */

	if ( STARTUP )
	{
		/* printf( "S" ); fflush( stdout ); */

		return;
	}

	if ( RECV_LOCK || PROCESS_LOCK )
	{
		/* printf( "L" ); fflush( stdout ); */

		return;
	}

	if ( INTERFACE_LOCK )
	{
		/* printf( "I" ); fflush( stdout ); */

		return;
	}

	RECV_LOCK = TRUE;

	/* Receive Events & Notifies */

	ecnt = trc_recv_messages( ID, RECV_COUNTER_INTERVAL, &status );

	if ( ecnt < 0 )
		exit( -1 );
	
	if ( status & TRC_MSG_STATUS_OVERWRITE )
		TRACE_OVERWRITE_FLAG = FALSE;

	TRACE_MSG_ACTIVE = ( status & TRC_MSG_STATUS_ACTIVE )
		? TRUE : FALSE;

	if ( ecnt )
	{
		TRACE_PENDING += ecnt;

		if ( ID->trace_out != NULL )
			fflush( ID->trace_out );

		if ( TRACE_ACTIVE == FALSE && TRACE_STATUS == TRACE_FASTFWD )
		{
			Tcl_Eval( interp, "setMsg { Fast-Forwarding Trace... }" );

			Tcl_Eval( interp, "raiseFastForwardFrames" );
		}

		TRACE_ACTIVE = TRUE;

		activity = TRUE;

		Tcl_Eval( interp, "active TRUE" );
	}

	else
	{
		activity = FALSE;

		if ( status & TRC_MSG_STATUS_CLEANUP )
			Tcl_Eval( interp, "active CLEANUP" );

		else
			Tcl_Eval( interp, "active FALSE" );
	}

	/* Set Up Next Event Recv Handler */

	if ( TRACE_ACTIVE &&
		( TRACE_STATUS == TRACE_FASTFWD
			|| TRACE_STATUS == TRACE_FWD
			|| TRACE_STATUS == TRACE_FWDSTEP ) )
	{
		if ( TRACE_PENDING
			|| TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
		{
			/* printf( "P" ); fflush( stdout ); */

			TICK_LOCK = UNLOCKED;

			Tk_DoWhenIdle( (Tk_IdleProc *) process_event_proc,
				(ClientData) NULL );
		}

		else
		{
			if ( !TICK_LOCK )
			{
				/* printf( "T" ); fflush( stdout ); */

				TICK_LOCK = LOCKED;

				Tk_CreateTimerHandler( RECV_TIMER_INTERVAL,
					(Tk_TimerProc *) process_timer_proc,
					(ClientData) NULL );
			}

			if ( activity )
			{
				/* printf( "R" ); fflush( stdout ); */

				Tk_DoWhenIdle( (Tk_IdleProc *) recv_event_proc,
					clientData );
			}

			else
			{
				/* printf( "t" ); fflush( stdout ); */

				Tk_CreateTimerHandler( RECV_TIMER_INTERVAL,
					(Tk_TimerProc *) recv_timer_proc,
					(ClientData) NULL );
			}
		}
	}

	else
	{
		/* printf( "s" ); fflush( stdout ); */

		TICK_LOCK = UNLOCKED;

		Tk_CreateTimerHandler( RECV_TIMER_INTERVAL,
			(Tk_TimerProc *) recv_timer_proc,
			(ClientData) NULL );
	}

	RECV_LOCK = FALSE;

	/* printf( "W" ); fflush( stdout ); */
}


/* ARGSUSED */
void
process_timer_proc( clientData )
ClientData clientData;
{
	Tk_DoWhenIdle( process_event_proc, (ClientData) NULL );
}


/* ARGSUSED */
void
process_event_proc( clientData )
ClientData clientData;
{
	struct timeval tm;

	int valid;
	int done;
	int cnt;
	int eof;

	/* printf( "-" ); fflush( stdout ); */

	if ( STARTUP )
	{
		/* printf( "S" ); fflush( stdout ); */

		return;
	}

	TICK_LOCK = UNLOCKED;

	if ( PROCESS_LOCK || RECV_LOCK )
	{
		/* printf( "L" ); fflush( stdout ); */

		return;
	}

	if ( INTERFACE_LOCK )
	{
		/* printf( "I" ); fflush( stdout ); */

		return;
	}

	PROCESS_LOCK = TRUE;

	check_host_status();

	if ( TRACE_STATUS == TRACE_FASTFWD
		|| TRACE_STATUS == TRACE_FWD
		|| TRACE_STATUS == TRACE_FWDSTEP )
	{
		/* printf( "F" ); fflush( stdout ); */

		if ( TRACE_PENDING || TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
		{
			/* printf( "E" ); fflush( stdout ); */

			done = 0;

			cnt = 0;

			while ( cnt < PROCESS_COUNTER_INTERVAL && !done )
			{
				/* printf( "E(%d)", TRACE_PENDING ); */
				/* fflush( stdout ); */

				TRACE_PROCESS_LOCK = LOCKED;

				valid = trc_read_trace_event( ID, &eof );

				TRACE_PROCESS_LOCK = UNLOCKED;

				/* Flag Error if EOF during Trace Overwrite */

				if ( eof && TRACE_FILE_STATUS != TRACE_FILE_PLAYBACK )
					printf( "EOF Reading Trace File\n" );

				/* Check for Mode Pending, Return if Not None */

				if ( check_mode_pending() )
				{
					/* Set Up Next Event Receiving Proc */

					Tk_DoWhenIdle( (Tk_IdleProc *) recv_event_proc,
						(ClientData) NULL );

					PROCESS_LOCK = FALSE;

					/* printf( "=" ); fflush( stdout ); */

					return;
				}

				/* Handle Event Process Return Code */

				if ( valid )
				{
					/* Update Trace Globals */

					if ( TRACE_STATUS == TRACE_FWDSTEP )
					{
						TRACE_STATUS = TRACE_STOP;

						set_trace_controls();

						done++;
					}

					if ( TRACE_FILE_STATUS != TRACE_FILE_PLAYBACK )
					{
						if ( TRACE_PENDING > 0 )
							TRACE_PENDING--;

						if ( !TRACE_PENDING )
							done++;
					}

					/* printf( "(%d)", TRACE_PENDING ); */
					/* fflush( stdout ); */
				}

				else
				{
					if ( TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
					{
						/* Clean Up Undead Tasks */

						tasks_alive();

						/* Playback Done */

						status_msg( ID, "Trace Playback Complete" );

						if ( TRACE_STATUS == TRACE_FASTFWD )
							redraw_views();

						TRACE_STATUS = TRACE_STOP;

						set_trace_controls();
					}

					TRACE_ACTIVE = FALSE;

					done++;
				}

				/* printf( "%d.", cnt ); fflush( stdout ); */

				cnt++;
			}

			/* printf( "U" ); fflush( stdout ); */
		}

		/* Update Current Time */

		else if ( !TRACE_MSG_ACTIVE )
		{
			/* printf( "T" ); fflush( stdout ); */

			if ( tasks_alive() )
			{
				/* printf( "t" ); fflush( stdout ); */

				gettimeofday( &tm, (struct timezone *) NULL );

				normalize_time( (TASK) NULL,
					(int *) &(tm.tv_sec), (int *) &(tm.tv_usec) );

				check_time( tm.tv_sec, tm.tv_usec );
			}

			else
			{
				if ( TASK_LIST != NULL )
				{
					status_msg( ID, "Trace Play Complete" );

					if ( TRACE_STATUS == TRACE_FASTFWD )
						redraw_views();
				}

				TRACE_ACTIVE = FALSE;
			}
		}
	}

	PROCESS_LOCK = FALSE;

	/* Set Up Next Event Receiving Proc */

	Tk_DoWhenIdle( (Tk_IdleProc *) recv_event_proc, (ClientData) NULL );

	/* printf( "_" ); fflush( stdout ); */
}


void
set_task_tracing( tids, ntasks, tstat )
int *tids;
int ntasks;
char *tstat;
{
	int i;

	pvm_initsend( PvmDataDefault );

	/* Set Modified Trace Mask */

	if ( !strcmp( tstat, "On" ) )
	{
		if ( TRACE_FORMAT == TEV_FMT_33 )
			pvm_pkstr( TRACE33_MASK );

		else
			pvm_pkstr( TRACE_MASK );
	}

	/* Set EMPTY Trace Mask */

	else
	{
		if ( TRACE_FORMAT == TEV_FMT_33 )
			pvm_pkstr( TRACE33_MASK_CLEAR );

		else
			pvm_pkstr( TRACE_MASK_CLEAR );
	}

	pvm_mcast( tids, ntasks, TC_SETTMASK );
}


void
set_trace_buffer( buf, do_register )
int buf;
int do_register;
{

#ifndef USE_PVM_33

	pvm_setopt( PvmTraceBuffer, buf );

	if ( do_register )
		re_register_tracer( CURRENT_TRACE_MASK );

#endif

}


void
set_trace_options( opt, do_register )
int opt;
int do_register;
{

#ifndef USE_PVM_33

	pvm_setopt( PvmTraceOptions, opt );

	if ( do_register )
		re_register_tracer( CURRENT_TRACE_MASK );

#endif

}


void
re_register_tracer( TM )
char *TM;
{

#ifndef USE_PVM_33

	/* Re-Register Tracer */

	if ( REGISTERED )
	{
		if ( pvm_reg_tracer( -1, -1, -1, -1,
			( TRACE_FORMAT == TEV_FMT_33 )
				? TRACE33_MASK_CLEAR : TRACE_MASK_CLEAR,
			-1, -1 ) > 0 )
		{
			printf( "Warning:  Error Unregistering Tracer...\n" );
		}

		else
			REGISTERED = FALSE;
	}

	if ( !REGISTERED )
	{
		if ( pvm_reg_tracer( pvm_getcontext(), 666,
			pvm_getcontext(), 667, TM, TRACE_BUF, TRACE_OPT ) < 0 )
		{
			printf( "Warning:  Tracer Already Connected...\n" );
		}

		else
			REGISTERED = TRUE;
	}

#endif

}


void
add_all_hosts()
{
	TRC_HOST H;

	int num;

	H = HOST_LIST;

	num = 0;

	while ( H != NULL )
	{
		if ( H->in_pvm == TRC_NOT_IN_PVM )
		{
			check_add_hosts( NADDS + 1 );

			ADD_HOSTS[ NADDS ] = H;

			ADD_HOSTS_NAMES[ NADDS ] = trc_copy_str( H->refname );

			NADDS++;

			num++;
		}

		H = H->next;
	}

	if ( num )
		Tk_DoWhenIdle( add_host, (ClientData) NULL );
}


void
do_add_host( H )
TRC_HOST H;
{
	check_add_hosts( NADDS + 1 );

	ADD_HOSTS[ NADDS ] = H;

	ADD_HOSTS_NAMES[ NADDS ] = trc_copy_str( H->refname );

	NADDS++;

	Tk_DoWhenIdle( add_host, (ClientData) H );
}


void
check_add_hosts( size )
int size;
{
	TRC_HOST *newlist;

	char **newnamelist;

	int newsize;
	int i;

	if ( size >= ADD_HOSTS_SIZE )
	{
		newsize = ADD_HOSTS_SIZE;

		while ( size >= newsize )
			newsize = size * 2;

		newlist = (TRC_HOST *) malloc( (unsigned) newsize
			* sizeof( TRC_HOST ) );
		trc_memcheck( newlist, "Add Hosts List" );

		newnamelist = (char **) malloc( (unsigned) newsize
			* sizeof( char * ) );
		trc_memcheck( newnamelist, "Add Hosts Names List" );

		for ( i=0 ; i < NADDS ; i++ )
		{
			newlist[i] = ADD_HOSTS[i];
			ADD_HOSTS[i] = (TRC_HOST) NULL;

			newnamelist[i] = ADD_HOSTS_NAMES[i];
			ADD_HOSTS_NAMES[i] = (char *) NULL;
		}
		
		for ( i=NADDS ; i < newsize ; i++ )
		{
			newlist[i] = (TRC_HOST) NULL;
			newnamelist[i] = (char *) NULL;
		}

		ADD_HOSTS = newlist;

		ADD_HOSTS_NAMES = newnamelist;

		ADD_HOSTS_SIZE = newsize;
	}
}


void
add_host( clientData )
ClientData clientData;
{
	HOST_EXT HE;

	TRC_HOST H;

	struct pvmhostinfo *HP;

	struct pvmhostinfo *hostp;

	char result[4096];
	char tmp[4192];

	int *infos;

	int arrange;
	int found;
	int narch;
	int nhost;
	int save;
	int i, j;
	int chk;
	int cc;

	LOCK_INTERFACE( save );

	/* Set Status Message */

	if ( NADDS )
	{
		if ( NADDS == 1 )
			sprintf( result, "Adding Host %s...", ADD_HOSTS_NAMES[0] );

		else
			sprintf( result, "Adding Hosts..." );

		status_msg( ID, result );
	}

	/* Add Hosts, If Still Any Left After Status Message Update */

	if ( NADDS )
	{
		infos = (int *) malloc( (unsigned) NADDS * sizeof(int) );
		trc_memcheck( infos, "Add Hosts Infos" );

		/* printf( "Host Add\n" ); */

		cc = pvm_addhosts( ADD_HOSTS_NAMES, NADDS, infos );

		if ( cc < 0 )
		{
			/* Set Status Message String */

			if ( NADDS == 1 )
			{
				pvm_perror( ADD_HOSTS_NAMES[0] );

				sprintf( result, "Error Adding Host %s",
					ADD_HOSTS_NAMES[0] );
			}

			else
			{
				pvm_perror( "Error Adding Hosts" );

				sprintf( result, "Error Adding Hosts" );
			}

			/* Reset Host Menuvars */

			for ( i=0 ; i < NADDS ; i++ )
			{
				HE = (HOST_EXT) (ADD_HOSTS[i])->ext;

				if ( HE->menuvar != NULL )
					SET_TCL_GLOBAL( interp, HE->menuvar, "OFF" );
			}
		}

		else
		{
			/* Determine # of Successful Adds */

			chk = NADDS - cc;

			/* Set Status Message String */

			if ( chk )
				strcpy( result, "Error Adding Hosts: " );

			else
				strcpy( result, "All Hosts Added." );

			/* Get PVM Configuration & Check Hosts */

			if ( chk < NADDS )
			{
				PVMCKERR( pvm_config( &nhost, &narch, &hostp ),
					"Error Checking Configuration", exit( -1 ) );
			}

			arrange = 0;

			for ( i=0 ; i < NADDS ; i++ )
			{
				if ( infos[i] < 0 )
				{
					sprintf( tmp, "%s ", ADD_HOSTS_NAMES[i] );

					trc_append_str( result, tmp, 4096 );

					HE = (HOST_EXT) (ADD_HOSTS[i])->ext;

					if ( HE->menuvar != NULL )
						SET_TCL_GLOBAL( interp, HE->menuvar, "OFF" );
				}

				else
				{
					H = ADD_HOSTS[i];

					H->in_pvm = TRC_IN_PVM;

					found = 0;

					for ( j=0 ; j < nhost && !found ; j++ )
					{
						HP = &(hostp[j]);

						if ( TRC_HOST_COMPARE( H, HP->hi_name ) )
						{
							if ( strlen( HP->hi_name )
								> strlen( H->name ) )
							{
								free( H->name );

								H->name = trc_copy_str( HP->hi_name );
							}

							H->arch = trc_copy_str( HP->hi_arch );

							H->pvmd_tid = HP->hi_tid;

							H->speed = HP->hi_speed;

							trc_do_host_sync( ID, H, FALSE );

							found++;
						}
					}

					if ( !found )
						H->pvmd_tid = infos[i];

					HE = (HOST_EXT) H->ext;

					HE->status = HOST_ADDED;

					if ( TRACE_FILE_STATUS != TRACE_FILE_PLAYBACK )
					{
						extract_network_host( H );

						draw_network_host( HE->nethost );

						REFRESH_GLOBAL( NET_ALIVE_COLOR );

						SET_HOST_BOX( interp,
							MAIN_NET->NET_C, HE->nethost,
							CHAR_GLOBVAL( NET_ALIVE_COLOR ), 2 );

						arrange++;
					}

					if ( HE->menuvar != NULL )
						SET_TCL_GLOBAL( interp, HE->menuvar, "ON" );
				}
			}

			if ( arrange )
				arrange_network_hosts( MAIN_NET );
		}

		free( infos );

		/* Clear Add Hosts Lists */

		for ( i=0 ; i < NADDS ; i++ )
		{
			ADD_HOSTS[i] = (TRC_HOST) NULL;

			if ( ADD_HOSTS_NAMES[i] != NULL )
			{
				free( ADD_HOSTS_NAMES[i] );

				ADD_HOSTS_NAMES[i] = (char *) NULL;
			}
		}

		NADDS = 0;

		/* Dump Status Message & Update */

		status_msg( ID, result );
	}

	UNLOCK_INTERFACE( save );
}


void
do_delete_host( H )
TRC_HOST H;
{
	Tk_DoWhenIdle( delete_host, (ClientData) H );
}


void
delete_host( clientData )
ClientData clientData;
{
	HOST_EXT HE;

	TRC_HOST H;

	struct pvmhostinfo *HP;

	struct pvmhostinfo *hostp;

	char result[4096];
	char tmp[1024];

	char *menustate;

	char *tmpptr;

	int found;
	int narch;
	int nhost;
	int info;
	int save;
	int cc;
	int i;

	LOCK_INTERFACE( save );

	H = (TRC_HOST) clientData;

	HE = (HOST_EXT) H->ext;

	if ( TRC_LOCALHOST( H->name ) )
	{
		sprintf( result, "Error Deleting XPVM's Host, %s.",
			host_name( H, TRUE ) );

		status_msg( ID, result );

		if ( HE->menuvar != NULL )
			SET_TCL_GLOBAL( interp, HE->menuvar, "ON" );
	}

	else
	{
		sprintf( result, "Deleting Host %s...", host_name( H, TRUE ) );
	
		status_msg( ID, result );
	
		/* printf( "Host Delete\n" ); */

		sscanf( H->refname, "%s", tmp );

		tmpptr = tmp;

		cc = pvm_delhosts( &tmpptr, 1, &info );

		if ( cc < 0 || info < 0 )
		{
			pvm_perror( H->name );
	
			PVMCKERR( pvm_config( &nhost, &narch, &hostp ),
				"Error Checking Configuration", exit( -1 ) );
	
			found = 0;

			for ( i=0 ; i < nhost && !found ; i++ )
			{
				HP = &(hostp[i]);

				if ( TRC_HOST_COMPARE( H, HP->hi_name ) )
					found++;
			}

			if ( !found )
			{
				H->in_pvm = TRC_NOT_IN_PVM;

				HE->status = HOST_DELETED;

				if ( HE->nethost != NULL )
				{
					((HOST_EXT_DRAW) (HE->nethost->ext))->status =
						HOST_DELETED;

					REFRESH_GLOBAL( NET_DEAD_COLOR );

					SET_HOST_BOX( interp, MAIN_NET->NET_C, HE->nethost,
						CHAR_GLOBVAL( NET_DEAD_COLOR ), 2 );
				}

				sprintf( result, "Host %s Deleted.",
					host_name( H, TRUE ) );

				menustate = "OFF";
			}

			else
			{
				sprintf( result, "Error Deleting Host %s.",
					host_name( H, TRUE ) );

				menustate = "ON";
			}
		}
	
		else
		{
			/* Mark Host as Deleted */

			H->in_pvm = TRC_NOT_IN_PVM;

			HE->status = HOST_DELETED;

			if ( HE->nethost != NULL )
			{
				((HOST_EXT_DRAW) (HE->nethost->ext))->status =
					HOST_DELETED;

				REFRESH_GLOBAL( NET_DEAD_COLOR );

				SET_HOST_BOX( interp, MAIN_NET->NET_C, HE->nethost,
					CHAR_GLOBVAL( NET_DEAD_COLOR ), 2 );
			}

			sprintf( result, "Host %s Deleted.", host_name( H, TRUE ) );

			menustate = "OFF";
		}

		if ( HE->menuvar != NULL )
			SET_TCL_GLOBAL( interp, HE->menuvar, menustate );

		status_msg( ID, result );
	}

	UNLOCK_INTERFACE( save );
}


void
handle_host_add_notify( H )
TRC_HOST H;
{
	HOST_EXT HE;

	TRC_HOST Hnew;

	int arrange = 0;
	int new;

	new = update_host( &Hnew, &HOST_LIST, H );

	if ( new )
	{
		/* Assign Index to New Host */

		Hnew->ext = (void *) (HE = create_host_ext());

		HE->status = HOST_OFF;

		HE->index = NHOSTS++;

		if ( !trc_compare( "0x", H->name ) )
			Tcl_Eval( interp, "refreshHostsMenu" );
	}

	else
	{
		/* If unknown host, but XPVM knows it, copy over primary info */

		if ( trc_compare( "0x", H->name ) )
		{
			if ( !trc_compare( "0x", Hnew->name ) )
			{
				free( H->name );
				free( H->alias );
				free( H->refname );

				H->name = trc_copy_str( Hnew->name );

				H->alias = trc_copy_str( Hnew->alias );

				H->refname = trc_copy_str( Hnew->refname );
			}

			if ( strcmp( Hnew->arch, "DUMMY" ) )
			{
				free( H->arch );

				H->arch = trc_copy_str( Hnew->arch );
			}

			if ( Hnew->speed != 0 )
				H->speed = Hnew->speed;
		}

		HE = (HOST_EXT) Hnew->ext;
	}

	/* Draw Host */

	if ( HE->status != HOST_DELETED )
	{
		HE->status = HOST_NOTIFY_ADD;

		if ( TRACE_FILE_STATUS != TRACE_FILE_PLAYBACK
			&& HE->nethost == NULL )
		{
			extract_network_host( Hnew );

			draw_network_host( HE->nethost );

			REFRESH_GLOBAL( NET_ALIVE_COLOR );

			SET_HOST_BOX( interp, MAIN_NET->NET_C, HE->nethost,
				CHAR_GLOBVAL( NET_ALIVE_COLOR ), 2 );

			arrange++;
		}

		if ( HE->menuvar != NULL )
			SET_TCL_GLOBAL( interp, HE->menuvar, "ON" );
	}

	if ( arrange )
		arrange_network_hosts( MAIN_NET );
}


int
update_host( Hret, host_list, H )
TRC_HOST *Hret;
TRC_HOST *host_list;
TRC_HOST H;
{
	TRC_HOST Hlast;
	TRC_HOST Hptr;

	char tmp[1024];
	char tmp2[1024];

	int too_quick;
	int found;
	int i, j;
	int new;

	new = 0;

	/* Determine Whether Host Too Quick */

	too_quick = 0;

	if ( trc_compare( "0x", H->name ) )
		too_quick++;

	/* Check Host List */

	Hptr = *host_list;

	Hlast = (TRC_HOST) NULL;

	found = 0;

	if ( !too_quick )
	{
		while ( Hptr != NULL && !found )
		{
			if ( TRC_HOST_COMPARE( Hptr, H->name ) )
				found++;

			else
			{
				Hlast = Hptr;

				Hptr = Hptr->next;
			}
		}
	}

	else
	{
		while ( Hptr != NULL && !found )
		{
			if ( Hptr->pvmd_tid == H->pvmd_tid )
				found++;

			else
			{
				Hlast = Hptr;

				Hptr = Hptr->next;
			}
		}
	}

	/* Host Already in Host List, Scope Out Name */

	if ( found )
	{
		if ( !too_quick )
		{
			if ( strlen( H->name ) > strlen( Hptr->name ) )
			{
				free( Hptr->name );
		
				Hptr->name = trc_copy_str( H->name );
			}

			sscanf( Hptr->refname, "%s", tmp );
			sscanf( H->name, "%s", tmp2 );

			if ( strcmp( tmp, tmp2 ) )
			{
				free( Hptr->refname );

				Hptr->refname = trc_copy_str( H->name );
			}
		}
	}

	/* Host Not Found, Add to Hosts List */

	else
	{
		if ( Hlast != NULL )
			Hptr = Hlast->next = trc_create_host();
		
		else
			Hptr = *host_list = trc_create_host();

		Hptr->name = trc_copy_str( H->name );

		Hptr->alias = trc_copy_str( H->alias );

		Hptr->refname = trc_copy_str( H->refname );

		new++;
	}

	/* Update Fields */

	if ( !too_quick || !found )
	{
		Hptr->arch = trc_copy_str( H->arch );

		Hptr->speed = H->speed;
	}

	Hptr->pvmd_tid = H->pvmd_tid;

	Hptr->delta.tv_sec = H->delta.tv_sec;
	Hptr->delta.tv_usec = H->delta.tv_usec;

	Hptr->in_pvm = TRC_IN_PVM;

	*Hret = Hptr;

	return( new );
}


void
handle_host_del_notify( Hdel )
TRC_HOST Hdel;
{
	HOST_EXT HE;

	TRC_HOST H;

	H = get_host_tid( HOST_LIST, Hdel->pvmd_tid );

	if ( H != NULL )
	{
		if ( H->in_pvm == TRC_IN_PVM )
			H->in_pvm = TRC_NOT_IN_PVM;

		HE = (HOST_EXT) H->ext;

		if ( TRACE_FILE_STATUS != TRACE_FILE_PLAYBACK )
		{
			if ( HE->status != HOST_ADDED )
			{
				if ( HE->menuvar != NULL )
					SET_TCL_GLOBAL( interp, HE->menuvar, "OFF" );

				HE->status = HOST_NOTIFY_DEL;

				if ( HE->nethost != NULL )
				{
					((HOST_EXT_DRAW) (HE->nethost->ext))->status =
						HOST_NOTIFY_DEL;

					REFRESH_GLOBAL( NET_DEAD_COLOR );

					SET_HOST_BOX( interp, MAIN_NET->NET_C, HE->nethost,
						CHAR_GLOBVAL( NET_DEAD_COLOR ), 2 );
				}
			}
		}

		else
		{
			if ( HE->menuvar != NULL )
				SET_TCL_GLOBAL( interp, HE->menuvar, "OFF" );

			HE->status = HOST_OFF;
		}
	}

	else
	{
		printf( "Missing Host for Notify Delete, (%s) tid=0x%x\n",
			Hdel->name, Hdel->pvmd_tid );
	}
}


void
handle_host_add( refname, alias, arch, tid, speed )
char *refname;
char *alias;
char *arch;
int tid;
int speed;
{
	HOST_EXT_DRAW HED;

	HOST_EXT HE;

	TRC_HOST H;

	char name[1024];

	if ( TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
	{
		sscanf( refname, "%s", name );

		H = create_network_host( refname, name, alias, arch, tid, speed,
			HOST_ON );

		draw_network_host( H );

		arrange_network_hosts( MAIN_NET );

		REFRESH_GLOBAL( NET_ALIVE_COLOR );

		flash_host( H, CHAR_GLOBVAL( NET_ALIVE_COLOR ), TRUE );
	}

	else
	{
		H = get_host_tid( HOST_LIST, tid );

		if ( H != NULL )
		{
			HE = (HOST_EXT) H->ext;
			
			if ( HE->status == HOST_NOTIFY_ADD )
			{
				HE->status = HOST_ON;

				/* Handle Network Host */

				H = HE->nethost;

				H->in_pvm = TRC_IN_PVM;

				HED = (HOST_EXT_DRAW) H->ext;
			
				HED->status = HOST_ON;

				REFRESH_GLOBAL( NET_ALIVE_COLOR );

				flash_host( H, CHAR_GLOBVAL( NET_ALIVE_COLOR ), TRUE );

				REFRESH_GLOBAL( NET_FG_COLOR );

				SET_HOST_BOX( interp, MAIN_NET->NET_C, H,
					CHAR_GLOBVAL( NET_FG_COLOR ), 1 );
			}
		}
	}
}


void
handle_host_del( name, tid, tsec, tusec )
char *name;
int tid;
int tsec;
int tusec;
{
	HOST_EXT_DRAW HED;

	HOST_EXT HE;

	STATE S;

	TRC_HOST H;

	TASK T;

	int ntasks;
	int cktm;

	/* Mark any soon-to-be-dead tasks */

	normalize_time( (TASK) NULL, &tsec, &tusec );

	T = TASK_LIST;

	ntasks = 0;

	cktm = 0;

	while ( T != NULL )
	{
		if ( T->pvmd_tid == tid && T->status != TASK_DEAD )
		{
			if ( !cktm )
			{
				check_time( tsec, tusec );

				cktm++;
			}

			T->deadtime.tv_sec = tsec;
			T->deadtime.tv_usec = tusec;

			S = T->states;

			if ( S != NULL )
			{
				if ( S->rect != NULL )
				{
					REFRESH_GLOBAL( SCALE );

					X2_COORD( S->rect ) = X_OF_TIME( tsec, tusec,
							INT_GLOBVAL( SCALE ) );

					CK_X_TIME( X2_COORD( S->rect ), tsec, tusec );

					SET_COORDS( interp, ST_C, S->rect );
				}

				S->endtime.tv_sec = tsec;
				S->endtime.tv_usec = tusec;
			}

			ntasks++;
		}

		T = T->next;
	}

	/* Turn Off Host */

	if ( TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
	{
		H = get_host_tid( MAIN_NET->host_list, tid );

		if ( H != NULL )
		{
			if ( ntasks )
			{
				H->in_pvm = TRC_NOT_IN_PVM;

				((HOST_EXT_DRAW) (H->ext))->status = HOST_DELETED;

				REFRESH_GLOBAL( NET_DEAD_COLOR );

				SET_HOST_BOX( interp, MAIN_NET->NET_C, H,
					CHAR_GLOBVAL( NET_DEAD_COLOR ), 2 );
			}

			else
			{
				destroy_network_host( &H, TRUE, FALSE );

				arrange_network_hosts( MAIN_NET );
			}
		}
	}

	else
	{
		H = get_host_tid( HOST_LIST, tid );

		if ( H != NULL )
		{
			if ( ntasks )
			{
				HE = (HOST_EXT) H->ext;

				if ( HE->status == HOST_NOTIFY_DEL )
				{
					HE->status = HOST_OFF;

					if ( HE->nethost != NULL )
					{
						((HOST_EXT_DRAW) (HE->nethost->ext))->status
							= HOST_DELETED;

						HE->nethost->in_pvm = TRC_NOT_IN_PVM;
					}
				}
			}

			else
			{
				HE = (HOST_EXT) H->ext;

				if ( HE->nethost != NULL )
					destroy_network_host( &(HE->nethost), TRUE, FALSE );

				arrange_network_hosts( MAIN_NET );
			}
		}
	}
}


void
check_for_host_dead( T )
TASK T;
{
	TRC_HOST Hlast;

	TASK Tptr;

	int pvmd_tid;
	int alive;

	/* See if Host is "Dead" */

	if ( T->host->in_pvm != TRC_NOT_IN_PVM )
		return;

	pvmd_tid = T->pvmd_tid;

	/* Look for any living tasks on host */

	Tptr = TASK_LIST;

	alive = 0;

	while ( Tptr != NULL && !alive )
	{
		if ( Tptr->pvmd_tid == pvmd_tid && Tptr->status != TASK_DEAD )
			alive++;

		Tptr = Tptr->next;
	}

	/* Host can finally rest in peace... */

	if ( !alive )
	{
		destroy_network_host( &(T->host), TRUE, FALSE );

		arrange_network_hosts( MAIN_NET );
	}
}


void
handle_host_sync( tid, sec, usec )
int tid;
int sec;
int usec;
{
	HOST_EXT HE;

	TRC_HOST H;

	int err;

	err = 0;

	if ( TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
	{
		H = get_host_tid( MAIN_NET->host_list, tid );

		if ( H != NULL )
		{
			/* Don't Change in Mid-Stream, Need Smooth Timelines... */

			if ( H->delta.tv_sec == 0 && H->delta.tv_usec == 0 )
			{
				H->delta.tv_sec = sec;
				H->delta.tv_usec = usec;
			}
		}

		else
			err++;
	}

	else
	{
		H = get_host_tid( HOST_LIST, tid );

		if ( H != NULL )
		{
			/* Don't Change in Mid-Stream, Need Smooth Timelines... */

			if ( H->delta.tv_sec == 0 && H->delta.tv_usec == 0 )
			{
				/* Set Host File Host Delta */

				H->delta.tv_sec = sec;
				H->delta.tv_usec = usec;

				/* Set Network Host Delta */

				HE = (HOST_EXT) H->ext;

				if ( HE->nethost != NULL )
				{
					HE->nethost->delta.tv_sec = sec;
					HE->nethost->delta.tv_usec = usec;
				}
			}
		}

		else
			err++;
	}

	if ( err )
	{
		printf( "Host Sync Delta (%d/%d) Error: TID=0x%x Not Found\n",
			sec, usec, tid );
	}
}


void
clear_network_hosts()
{
	HOST_EXT HE;

	TRC_HOST Hnext;
	TRC_HOST H;

	/* Don't Wipe Out Hosts in Mid-Trace-Play...  D-Oh! */

	if ( TRACE_PROCESS_LOCK == LOCKED )
	{
		TRACE_MODE_PENDING |= TRACE_MODE_CLEAR_HOSTS;

		return;
	}

	/* Do Host File Hosts first, to clear "nethost" ptrs... */

	H = HOST_LIST;

	while ( H != NULL )
	{
		HE = (HOST_EXT) H->ext;

		if ( HE->nethost != NULL )
			destroy_network_host( &(HE->nethost), FALSE, TRUE );

		if ( trc_compare( "0x", H->name ) )
			HE->status = HOST_OFF;

		H = H->next;
	}

	/* Now wipe out any plain playback hosts */

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		Hnext = H->next;

		destroy_network_host( &H, FALSE, TRUE );

		H = Hnext;
	}

	/* "Arrange" Empty Network :-) */

	arrange_network_hosts( MAIN_NET );
}


void
restore_hosts_from_playback()
{
	HOST_EXT HE;

	TRC_HOST H;

	int i;

	/* Clear Any Existing Network Hosts */

	clear_network_hosts();

	/* Draw Current Hosts */

	H = HOST_LIST;

	while ( H != NULL )
	{
		HE = (HOST_EXT) H->ext;

		/* Handle Partial Notifies */

		if ( HE->status == HOST_NOTIFY_DEL )
			HE->status = HOST_OFF;

		else if ( HE->status == HOST_NOTIFY_ADD )
			HE->status = HOST_ON;

		/* Create New Network Hosts for Active Hosts */

		if ( HE->status != HOST_OFF )
		{
			extract_network_host( H );

			draw_network_host( HE->nethost );
		}

		H = H->next;
	}

	arrange_network_hosts( MAIN_NET );
}


TASK
handle_newtask( tid, name, ptid, pvmd_tid, flags, tsec, tusec )
int tid;
char *name;
int ptid;
int pvmd_tid;
int flags;
int *tsec;
int *tusec;
{
	TASK T;

	if ( !(ID->group_tasks) && TRC_GROUPTASK( name ) )
		return( (TASK) NULL );

	/* Create New Task Structure */

	T = create_task();

	T->tid = tid;

	T->name = trc_copy_str( name );

	T->ptid = ptid;

	T->pvmd_tid = pvmd_tid;

	T->host = get_host_tid( MAIN_NET->host_list, pvmd_tid );

	T->flags = flags;

	T->status = TASK_ALIVE;

	/* Handle New Task Times */

	if ( BASE_TIME.tv_sec == -1 )
	{
		sync_time( T->host, tsec, tusec );

		BASE_TIME.tv_sec = *tsec;
		BASE_TIME.tv_usec = *tusec;

		update_time( 0, 0 );

		*tsec = *tusec = 0;
	}

	else
	{
		normalize_time( T, tsec, tusec );

		check_time( *tsec, *tusec );
	}

	return( T );
}


char *
host_name( H, full )
TRC_HOST H;
int full;
{
	char str[1024];

	if ( full && strcmp( H->name, H->alias ) )
		sprintf( str, "%s (%s)", H->name, H->alias );

	else
		sprintf( str, "%s", H->alias );

	return( trc_copy_str( str ) );
}

