
static char rcsid[] = 
	"$Id: compat.trace.c,v 4.50 1998/04/09 21:11:39 kohl Exp $";

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


/* Trace Processing Routines */

void
handle_old_event( trcid, eid, tsec, tusec, tid )
TRC_ID trcid;
int eid;
int tsec;
int tusec;
int tid;
{
	TASK T;

	if ( trcid != ID )
	{
		printf( "Error: Unknown Trace ID\n" );

		return;
	}

	/* Time Adjustment Special Cases */

	if ( eid == TRC_OLD_TEV_NEWTASK
		|| eid == TRC_OLD_TEV_SPNTASK
		|| eid == TRC_OLD_TEV_ENDTASK
		|| eid == TRC_OLD_TRACE_OUTPUT
		|| eid == TRC_OLD_TRACE_HOST_ADD
		|| eid == TRC_OLD_TRACE_HOST_DEL
		|| eid == TRC_OLD_TRACE_HOST_SYNC )
	{
		process_pvm_event( eid, tid, tsec, tusec, (TASK) NULL );
	}

	else
	{
		T = get_task_tid( tid );

		/* Normal Event */

		if ( T != NULL )
		{
			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			process_pvm_event( eid, tid, tsec, tusec, T );
		}

		/* Unknown Event */

		else
			trc_find_event_end( ID );
	}
}


void
process_pvm_event( eid, tid, tsec, tusec, T )
int eid;
int tid;
int tsec;
int tusec;
TASK T;
{
	static char cmd[4200];

	char refname[1024];
	char result[4096];
	char alias[1024];
	char where[1024];
	char name[1024];
	char tmp[1024];

	char *u_usec_pad;
	char *s_usec_pad;
	char *tusec_pad;

	char *str;

	char c;

	int u_sec, u_usec;
	int s_sec, s_usec;

	int do_state;
	int pvmd_tid;
	int do_exit;
	int msgtag;
	int nbytes;
	int count;
	int flags;
	int speed;
	int state;
	int ptid;
	int dtid;
	int stid;
	int usec;
	int sec;
	int buf;
	int num;
	int cc;
	int i;

	if ( ID->trace_in == NULL )
		return;

	str = (char *) NULL;

	do_state = TRUE;

	do_exit = FALSE;

	switch ( eid )
	{
		case TRC_OLD_TEV_NEWTASK:
		{
			fscanf( ID->trace_in, ", %d, %d, %d",
				&ptid, &pvmd_tid, &flags );

			trc_read_old_trace_str( ID, name, 1024, TRUE );

			trc_find_event_end( ID );

			if ( (T = handle_newtask( tid, name, ptid, pvmd_tid, flags,
				&tsec, &tusec )) == NULL )
			{
				return;
			}

			sprintf( result,
				"New Task %s: ptid=0x%x pvmd_tid=0x%x flags=0x%x",
				name, ptid, pvmd_tid, flags );

			taskAdd( T, tsec, tusec, result );

			do_state = FALSE;

			str = result;

			break;
		}

		case TRC_OLD_TEV_SPNTASK:
		{
			T = get_task_tid( tid );

			normalize_time( T, &tsec, &tusec );

			str = trc_old_pvm_event_string( ID, eid );

			do_state = FALSE;

			break;
		}

		case TRC_OLD_TEV_ENDTASK:
		{
			T = get_task_tid( tid );

			if ( T != NULL )
			{
				fscanf( ID->trace_in, ", %d, %d, %d, %d, %d",
					&flags, &u_sec, &u_usec, &s_sec, &s_usec );

				trc_find_event_end( ID );

				u_usec_pad = trc_pad_num( u_usec, 6 );
				s_usec_pad = trc_pad_num( s_usec, 6 );

				sprintf( result,
					"End Task: status=0x%x user=%d.%s sys=%d.%s",
					flags, u_sec, u_usec_pad, s_sec, s_usec_pad );

				free( u_usec_pad );
				free( s_usec_pad );

				normalize_time( T, &tsec, &tusec );

				if ( T->status != TASK_DEAD )
					check_time( tsec, tusec );

				do_exit = TRUE;

				str = result;
			}

			else
			{
				trc_find_event_end( ID );

				return;
			}

			break;
		}

		case TRC_OLD_TRACE_OUTPUT:
		{
			T = get_task_tid( tid );

			if ( T != NULL )
			{
				trc_read_old_trace_str( ID, tmp, 1024, TRUE );

				trc_find_event_end( ID );

				sprintf( result, "task output: tid=0x%x str=\"%s\"",
					tid, tmp );

				normalize_time( T, &tsec, &tusec );

				task_output( tid, tmp );

				do_state = FALSE;

				str = result;
			}

			else
			{
				trc_find_event_end( ID );

				return;
			}

			break;
		}

		case TRC_OLD_TRACE_HOST_ADD:
		{
			/* printf( "Trace Host Add\n" ); */

			trc_read_old_trace_str( ID, refname, 1024, TRUE );

			trc_read_old_trace_str( ID, alias, 1024, TRUE );

			trc_read_old_trace_str( ID, tmp, 1024, TRUE );

			fscanf( ID->trace_in, ", %d", &speed );

			trc_find_event_end( ID );

			sprintf( result,
				"host %s (%s) added: arch=%s tid=0x%x speed=%d",
					refname, alias, tmp, tid, speed );

			handle_host_add( refname, alias, tmp, tid, speed );

			do_state = FALSE;

			str = result;

			break;
		}

		case TRC_OLD_TRACE_HOST_DEL:
		{
			/* printf( "Trace Host Delete\n" ); */

			trc_read_old_trace_str( ID, name, 1024, TRUE );

			trc_find_event_end( ID );

			sprintf( result, "host %s deleted: tid=0x%x", name, tid );

			handle_host_del( name, tid, tsec, tusec );

			do_state = FALSE;

			str = result;

			break;
		}

		case TRC_OLD_TRACE_HOST_SYNC:
		{
			fscanf( ID->trace_in, ", %d, %d", &sec, &usec );

			trc_find_event_end( ID );

			sprintf( result, "Host Sync Delta: tid=0x%x %d %d",
				tid, sec, usec );

			handle_host_sync( tid, sec, usec );

			do_state = FALSE;

			str = result;

			break;
		}

		case TRC_OLD_TEV_ADDHOSTS0:
		{
			strcpy( result, "pvm_addhosts0()" );

			fscanf( ID->trace_in, ", %d", &num );

			if ( !trc_find_event_str( ID, "{" ) )
				return;

			for ( i=0 ; i < num ; i++ )
			{
				trc_read_old_trace_str( ID, name, 1024, FALSE );

				sprintf( tmp, " %s", name );

				trc_append_str( result, tmp, 4096 );
			}

			if ( !trc_find_event_str( ID, "}" ) )
				return;

			trc_find_event_end( ID );

			str = result;

			break;
		}

		case TRC_OLD_TEV_ADDHOSTS1:
		{
			fscanf( ID->trace_in, ", %d", &cc );

			trc_find_event_end( ID );

			sprintf( result, "pvm_addhosts1() cc=%d", cc );

			str = result;

			break;
		}

		case TRC_OLD_TEV_SPAWN0:
		{
			trc_read_old_trace_str( ID, name, 1024, TRUE );

			fscanf( ID->trace_in, ", %d", &flags );

			trc_read_old_trace_str( ID, where, 1024, TRUE );

			fscanf( ID->trace_in, ", %d", &count );

			trc_find_event_end( ID );

			sprintf( result, "pvm_spawn0() %d copies of %s",
				count, name );

			if ( *where != '\0' )
			{
				sprintf( tmp, " on %s", where );

				trc_append_str( result, tmp, 4096 );
			}

			str = result;

			break;
		}

		case TRC_OLD_TEV_SPAWN1:
		{
			fscanf( ID->trace_in, ", %d", &num );

			sprintf( result, "pvm_spawn1() %d successful", num );

			if ( !trc_find_event_str( ID, "{" ) )
				return;

			if ( num > 0 )
			{
				trc_append_str( result, ":", 4096 );

				for ( i=0 ; i < num ; i++ )
				{
					if ( i )
						fscanf( ID->trace_in, ", %d", &stid );

					else
						fscanf( ID->trace_in, "%d", &stid );

					sprintf( tmp, " 0x%x", stid );

					trc_append_str( result, tmp, 4096 );
				}
			}

			if ( !trc_find_event_str( ID, "}" ) )
				return;

			trc_find_event_end( ID );

			str = result;

			break;
		}

		case TRC_OLD_TEV_EXIT0:
		{
			trc_find_event_end( ID );

			strcpy( result, "pvm_exit()" );

			str = result;

			break;
		}

		case TRC_OLD_TEV_SEND0:
		{
			fscanf( ID->trace_in, ", %d, %d", &dtid, &msgtag );

			nbytes = -1;

			trc_find_event_end( ID );

			sprintf( result,
				"pvm_send0() to 0x%x, msgtag=%d, nbytes=%d",
				dtid, msgtag, nbytes );

			send_message( T, tid, dtid, msgtag, -1, nbytes,
				tsec, tusec );

			str = result;

			break;
		}

		case TRC_OLD_TEV_MCAST0:
		{
			fscanf( ID->trace_in, ", %d, %d", &num, &msgtag );

			nbytes = -1;

			sprintf( result, "pvm_mcast0() msgtag=%d, nbytes=%d",
				msgtag, nbytes );

			if ( !trc_find_event_str( ID, "{" ) )
				return;

			if ( num > 0 )
			{
				trc_append_str( result, " to:", 4096 );

				for ( i=0 ; i < num ; i++ )
				{
					if ( i )
						fscanf( ID->trace_in, ", %d", &dtid );

					else
						fscanf( ID->trace_in, "%d", &dtid );

					sprintf( tmp, " 0x%x", dtid );

					trc_append_str( result, tmp, 4096 );

					if ( dtid != tid )
					{
						send_message( T, tid, dtid, msgtag, -1, nbytes,
							tsec, tusec );
					}
				}
			}

			if ( !trc_find_event_str( ID, "}" ) )
				return;

			trc_find_event_end( ID );

			str = result;

			break;
		}

		case TRC_OLD_TEV_PSEND0:
		{
			fscanf( ID->trace_in, ", %d, %d", &dtid, &msgtag );

			nbytes = -1;

			trc_find_event_end( ID );

			sprintf( result,
				"pvm_psend0() to 0x%x, msgtag=%d, nbytes=%d",
				dtid, msgtag, nbytes );

			send_message( T, tid, dtid, msgtag, -1, nbytes,
				tsec, tusec );

			str = result;

			break;
		}

		case TRC_OLD_TEV_RECV1:
		{
			fscanf( ID->trace_in, ", %d, %d, %d, %d",
				&buf, &nbytes, &msgtag, &stid );

			trc_find_event_end( ID );

			sprintf( result, "pvm_recv1() buf=%d", buf );

			if ( nbytes >= 0 )
			{
				sprintf( tmp, ", %d bytes from 0x%x, msgtag=%d",
					nbytes, stid, msgtag );

				trc_append_str( result, tmp, 4096 );

				recv_message( T, tid, stid, msgtag, -1, nbytes,
					tsec, tusec );
			}

			str = result;

			break;
		}

		case TRC_OLD_TEV_NRECV1:
		{
			fscanf( ID->trace_in, ", %d, %d, %d, %d",
				&buf, &nbytes, &msgtag, &stid );

			trc_find_event_end( ID );

			strcpy( result, "pvm_nrecv1()" );

			if ( buf > 0 )
			{
				sprintf( tmp, " buf=%d", buf );

				trc_append_str( result, tmp, 4096 );

				if ( nbytes >= 0 )
				{
					sprintf( tmp, ", %d bytes from 0x%x, msgtag=%d",
						nbytes, stid, msgtag );

					trc_append_str( result, tmp, 4096 );

					recv_message( T, tid, stid, msgtag, -1, nbytes,
						tsec, tusec );
				}
			}

			else
				trc_append_str( result, " no message", 4096 );

			str = result;

			break;
		}

		case TRC_OLD_TEV_PRECV1:
		{
			fscanf( ID->trace_in, ", %d, %d, %d, %d",
				&buf, &nbytes, &msgtag, &stid );

			trc_find_event_end( ID );

			sprintf( result, "pvm_precv1() buf=%d", buf );

			if ( nbytes >= 0 )
			{
				sprintf( tmp, ", %d bytes from 0x%x, msgtag=%d",
					nbytes, stid, msgtag );

				trc_append_str( result, tmp, 4096 );

				recv_message( T, tid, stid, msgtag, -1, nbytes,
					tsec, tusec );
			}

			str = result;

			break;
		}

		case TRC_OLD_TEV_TRECV1:
		{
			fscanf( ID->trace_in, ", %d, %d, %d, %d",
				&buf, &nbytes, &msgtag, &stid );

			trc_find_event_end( ID );

			sprintf( result, "pvm_trecv1() buf=%d", buf );

			if ( nbytes >= 0 )
			{
				sprintf( tmp, ", %d bytes from 0x%x, msgtag=%d",
					nbytes, stid, msgtag );

				trc_append_str( result, tmp, 4096 );

				recv_message( T, tid, stid, msgtag, -1, nbytes,
					tsec, tusec );
			}

			str = result;

			break;
		}

		default:
			str = trc_old_pvm_event_string( ID, eid );
	}

	if ( !tflag && do_state )
	{
		switch ( TEV33_TYPE[ eid ] )
		{
			case ENTRY_TEV:
			{
				if ( eid == TRC_OLD_TEV_RECV0
					|| eid == TRC_OLD_TEV_PRECV0
					|| eid == TRC_OLD_TEV_TRECV0 )
				{
					state = STATE_IDLE;
				}

				else
					state = STATE_SYSTEM;

				taskState( T, tsec, tusec, state, str );

				break;
			}

			case EXIT_TEV:
			{
				taskState( T, tsec, tusec, STATE_RUNNING, str );

				break;
			}

			case IGNORE_TEV:
			{
				taskState( T, tsec, tusec, STATE_DEAD, str );

				break;
			}
		}
	}

	if ( do_exit )
		taskExit( T, tsec, tusec, str );

	if ( eflag )
	{
		tusec_pad = trc_pad_num( tusec, 6 );

		printf( "PVM Event TID=0x%x @ %d.%s: %s\n",
			tid, tsec, tusec_pad, str );

		free( tusec_pad );

		fflush( stdout );
	}

	if ( str != NULL && str != result )
		free( str );
}


char *
strip33_name( name, entry_exit )
char *name;
int *entry_exit;
{
	char *tmp;
	char *ptr;

	*entry_exit = IGNORE_TEV;

	tmp = trc_copy_str( name );

	ptr = tmp;

	while ( *ptr != '\0' )
	{
		if ( *ptr >= 'a' && *ptr <= 'z' )
			*ptr += 'A' - 'a';

		else if ( ( *ptr == '0' || *ptr == '1' ) && *(ptr + 1) == '\0' )
		{
			if ( *ptr == '0' )
				*entry_exit = ENTRY_TEV;
			
			else
				*entry_exit = EXIT_TEV;

			*ptr = '\0';
		}

		ptr++;
	}

	return( tmp );
}

