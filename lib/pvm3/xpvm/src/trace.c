
static char rcsid[] = 
	"$Id: trace.c,v 4.50 1998/04/09 21:11:57 kohl Exp $";

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
init_event_trie()
{
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"newtask",		(void *) HANDLE_NEWTASK );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"spntask",		(void *) HANDLE_SPNTASK );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"endtask",		(void *) HANDLE_ENDTASK );

	trc_add_to_trie( TRC_HANDLE_TRIE,
		"output",		(void *) HANDLE_OUTPUT );

	trc_add_to_trie( TRC_HANDLE_TRIE,
		"user_defined",	(void *) HANDLE_USER_DEFINED );

	trc_add_to_trie( TRC_HANDLE_TRIE,
		"host_add",		(void *) HANDLE_HOST_ADD );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"host_del",		(void *) HANDLE_HOST_DEL );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"host_sync",	(void *) HANDLE_HOST_SYNC );

	trc_add_to_trie( TRC_HANDLE_TRIE,
		"exit",			(void *) HANDLE_EXIT );

	trc_add_to_trie( TRC_HANDLE_TRIE,
		"send",			(void *) HANDLE_SEND );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"mcast",		(void *) HANDLE_MCAST );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"psend",		(void *) HANDLE_PSEND );

	trc_add_to_trie( TRC_HANDLE_TRIE,
		"recv",			(void *) HANDLE_RECV );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"nrecv",		(void *) HANDLE_NRECV );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"precv",		(void *) HANDLE_PRECV );
	trc_add_to_trie( TRC_HANDLE_TRIE,
		"trecv",		(void *) HANDLE_TRECV );
}


/* Trace Processing Routines */

void
handle_event( trcid, TD, TR )
TRC_ID trcid;
TRC_TEVDESC TD;
TRC_TEVREC TR;
{
	TRC_TEVREC tr_dtids;
	TRC_TEVREC tr_name;
	TRC_TEVREC tr_hna;
	TRC_TEVREC tr_hn;
	TRC_TEVREC tr_ha;
	TRC_TEVREC tr_os;

	TRC_HOST H;

	TASK T;

	char event_str[1024];
	char tmp[1024];

	char *u_usec_pad;
	char *s_usec_pad;
	char *usec_pad;

	char *alias;
	char *arch;
	char *name;
	char *os;

	char *str;

	int u_sec, u_usec;
	int s_sec, s_usec;
	int tsec, tusec;
	int sec, usec;

	int pvmd_tid;
	int msgtag;
	int msgctx;
	int nbytes;
	int status;
	int flags;
	int speed;
	int dtid;
	int stid;
	int ptid;
	int len;
	int num;
	int tid;
	int buf;
	int dt;

	int do_state;
	int do_exit;
	int state;
	int i;

	if ( trcid != ID )
	{
		printf( "Error: Unknown Trace ID\n" );

		return;
	}

	alias = (char *) NULL;
	arch = (char *) NULL;
	name = (char *) NULL;
	os = (char *) NULL;

	str = (char *) NULL;

	tsec = TRC_GET_TEVREC_VALUE( TR, "TS", int );
	tusec = TRC_GET_TEVREC_VALUE( TR, "TU", int );

	tid = -1;

	do_state = TRUE;

	do_exit = FALSE;

	switch ( TD->hid )
	{
		case HANDLE_NEWTASK:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( !TRC_GET_TEVREC( TR, tr_name, "TN" ) )
				name = "(Unknown Task)";

			ptid = TRC_GET_TEVREC_VALUE( TR, "PT", int );

			pvmd_tid = pvm_tidtohost( tid );

			flags = TRC_GET_TEVREC_VALUE( TR, "TF", int );

			if ( (T = handle_newtask( tid,
				name ? name : (char *) tr_name->value,
				ptid, pvmd_tid, flags, &tsec, &tusec )) == NULL )
			{
				return;
			}

			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str,
				"New Task %s @ %d.%s: PT=0x%x Host=0x%x TF=0x%x.",
				name ? name : (char *) tr_name->value,
				tsec, usec_pad, ptid, pvmd_tid, flags );

			free( usec_pad );

			taskAdd( T, tsec, tusec, event_str );

			do_state = FALSE;

			str = event_str;

			break;
		}

		case HANDLE_SPNTASK:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			T = get_task_tid( tid );

			normalize_time( T, &tsec, &tusec );

			str = trc_tevrec_string( TD, TR, tid, tsec, tusec );

			do_state = FALSE;

			break;
		}

		case HANDLE_ENDTASK:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			if ( T->status != TASK_DEAD )
				check_time( tsec, tusec );

			flags = TRC_GET_TEVREC_VALUE( TR, "TF", int );

			u_sec = TRC_GET_TEVREC_VALUE( TR, "TUS", int );
			u_usec = TRC_GET_TEVREC_VALUE( TR, "TUU", int );

			s_sec = TRC_GET_TEVREC_VALUE( TR, "TSS", int );
			s_usec = TRC_GET_TEVREC_VALUE( TR, "TSU", int );

			u_usec_pad = trc_pad_num( u_usec, 6 );
			s_usec_pad = trc_pad_num( s_usec, 6 );
			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str,
			"End Task @ %d.%s: TF=0x%x User=%d.%s Sys=%d.%s.",
				tsec, usec_pad, flags, u_sec, u_usec_pad,
				s_sec, s_usec_pad );

			free( u_usec_pad );
			free( s_usec_pad );
			free( usec_pad );

			do_exit = TRUE;

			str = event_str;

			break;
		}

		case HANDLE_OUTPUT:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			if ( !TRC_GET_TEVREC( TR, tr_os, "OS" ) )
				os = "(Unknown Output)";

			sprintf( event_str, "Task Output: OS=\"%s\"",
				os ? os : (char *) tr_os->value );

			task_output( tid, os ? os : (char *) tr_os->value );

			do_state = FALSE;

			str = event_str;

			break;
		}

		case HANDLE_USER_DEFINED:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			str = trc_tevrec_string( TD, TR, tid, tsec, tusec );

			taskState( T, tsec, tusec, STATE_USER_DEFINED, str );

			do_state = FALSE;

			break;
		}

		case HANDLE_HOST_ADD:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( !TRC_GET_TEVREC( TR, tr_hn, "HN" ) )
				name = "(Unknown Host Name)";

			if ( !TRC_GET_TEVREC( TR, tr_hna, "HNA" ) )
				alias = "(Unknown Host Alias)";

			if ( !TRC_GET_TEVREC( TR, tr_ha, "HA" ) )
				arch = "(Unknown Host Arch)";

			speed = TRC_GET_TEVREC_VALUE( TR, "HSP", int );

			sprintf( event_str,
				"Host %s (%s) Added: HA=%s TID=0x%x HSP=%d",
				name ? name : (char *) tr_hn->value,
				alias ? alias : (char *) tr_hna->value,
				arch ? arch : (char *) tr_ha->value,
				tid, speed );

			handle_host_add( name ? name : (char *) tr_hn->value,
				alias ? alias : (char *) tr_hna->value,
				arch ? arch : (char *) tr_ha->value,
				tid, speed );

			do_state = FALSE;

			str = event_str;

			break;
		}

		case HANDLE_HOST_DEL:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( !TRC_GET_TEVREC( TR, tr_hn, "HN" ) )
				name = "(Unknown Host)";

			sprintf( event_str, "Host %s Deleted: TID=0x%x",
				name ? name : (char *) tr_hn->value, tid );

			handle_host_del( name ? name : (char *) tr_hn->value,
				tid, tsec, tusec );

			do_state = FALSE;

			str = event_str;

			break;
		}

		case HANDLE_HOST_SYNC:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			sec = TRC_GET_TEVREC_VALUE( TR, "HDS", int );

			usec = TRC_GET_TEVREC_VALUE( TR, "HDU", int );

			sprintf( event_str,
				"Host Sync Delta: TID=0x%x HDS=%d HDU=%d",
				tid, sec, usec );
			
			handle_host_sync( tid, sec, usec );
			
			do_state = FALSE;

			str = event_str;

			break;
		}

		case HANDLE_EXIT:
		{
			if ( TD->entry_exit != ENTRY_TEV )
				break;

			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str, "exit.0 @ %d.%s.", tsec, usec_pad );

			free( usec_pad );

			str = event_str;

			break;
		}

		case HANDLE_SEND:
		{
			if ( TD->entry_exit != ENTRY_TEV )
				break;

			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			dtid = TRC_GET_TEVREC_VALUE( TR, "DST", int );

			msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int );

			msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int );

			nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

			send_message( T, tid, dtid, msgtag, msgctx, nbytes,
				tsec, tusec );

			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str,
				"send.0 @ %d.%s: to 0x%x code=%d ctxt=%d nbytes=%d.",
				tsec, usec_pad, dtid, msgtag, msgctx, nbytes );

			free( usec_pad );

			str = event_str;

			break;
		}

		case HANDLE_MCAST:
		{
			if ( TD->entry_exit != ENTRY_TEV )
				break;

			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int );

			msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int );

			nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str,
				"mcast.0 @ %d.%s: code=%d ctxt=%d nbytes=%d to ",
				tsec, usec_pad, msgtag, msgctx, nbytes );

			free( usec_pad );

			num = ( TRC_GET_TEVREC( TR, tr_dtids, "MDL" ) )
				? tr_dtids->num : 0;
			
			for ( i=0 ; i < num ; i++ )
			{
				dtid = TRC_ARR_VALUE_OF( tr_dtids->value, int, i );

				if ( dtid != tid )
				{
					send_message( T, tid, dtid, msgtag, msgctx, nbytes,
						tsec, tusec );
				}

				if ( i < tr_dtids->num - 1 )
					sprintf( tmp, "0x%x, ", dtid );

				else
					sprintf( tmp, "0x%x.", dtid );

				trc_append_str( event_str, tmp, 1024 );
			}

			str = event_str;

			break;
		}

		case HANDLE_PSEND:
		{
			if ( TRUE )
				break;

			if ( TD->entry_exit != ENTRY_TEV )
				break;

			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			dtid = TRC_GET_TEVREC_VALUE( TR, "DST", int );

			msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int );

			msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int );

			len = TRC_GET_TEVREC_VALUE( TR, "PC", int );

			dt = TRC_GET_TEVREC_VALUE( TR, "PDT", int );

			nbytes = get_plength( len, dt );

			send_message( T, tid, dtid, msgtag, msgctx, nbytes,
				tsec, tusec );

			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str,
				"psend.0 @ %d.%s: to 0x%x code=%d ctxt=%d nbytes=%d.",
				tsec, usec_pad, dtid, msgtag, msgctx, nbytes );

			free( usec_pad );

			str = event_str;

			break;
		}

		case HANDLE_RECV:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			if ( TD->entry_exit == ENTRY_TEV )
			{
				str = trc_tevrec_string( TD, TR, tid, tsec, tusec );

				if ( str == NULL )
					return;

				taskState( T, tsec, tusec, STATE_IDLE, str );

				do_state = FALSE;

				break;
			}

			else if ( TD->entry_exit != EXIT_TEV )
				break;

			buf = TRC_GET_TEVREC_VALUE( TR, "MB", int );

			msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int );

			msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int );

			nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

			stid = TRC_GET_TEVREC_VALUE( TR, "SRC", int );

			recv_message( T, tid, stid, msgtag, msgctx, nbytes,
				tsec, tusec );

			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str,
		"recv.1 @ %d.%s: from 0x%x buf=%d code=%d ctxt=%d nbytes=%d.",
				tsec, usec_pad, stid, buf, msgtag, msgctx, nbytes );
			
			free( usec_pad );

			str = event_str;

			break;
		}

		case HANDLE_NRECV:
		{
			if ( TD->entry_exit != EXIT_TEV )
				break;

			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			buf = TRC_GET_TEVREC_VALUE( TR, "MB", int );

			if ( buf > 0 )
			{
				msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int );

				msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int );

				nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

				stid = TRC_GET_TEVREC_VALUE( TR, "SRC", int );

				recv_message( T, tid, stid, msgtag, msgctx, nbytes,
					tsec, tusec );

				usec_pad = trc_pad_num( tusec, 6 );

				sprintf( event_str,
		"nrecv.1 @ %d.%s: from 0x%x buf=%d code=%d ctxt=%d nbytes=%d.",
					tsec, usec_pad, stid, buf, msgtag, msgctx, nbytes );

				free( usec_pad );

				str = event_str;
			}

			else
			{
				usec_pad = trc_pad_num( tusec, 6 );

				sprintf( event_str, "nrecv.1 @ %d.%s: no message.",
					tsec, usec_pad );

				free( usec_pad );

				str = event_str;
			}

			break;
		}

		case HANDLE_PRECV:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			if ( TD->entry_exit == ENTRY_TEV )
			{
				str = trc_tevrec_string( TD, TR, tid, tsec, tusec );

				if ( str == NULL )
					return;

				taskState( T, tsec, tusec, STATE_IDLE, str );

				do_state = FALSE;

				break;
			}

			else if ( TD->entry_exit != EXIT_TEV )
				break;

			msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int );

			msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int );

			nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

			stid = TRC_GET_TEVREC_VALUE( TR, "SRC", int );

			recv_message( T, tid, stid, msgtag, msgctx, nbytes,
				tsec, tusec );

			usec_pad = trc_pad_num( tusec, 6 );

			sprintf( event_str,
				"precv.1 @ %d.%s: from 0x%x code=%d ctxt=%d nbytes=%d.",
				tsec, usec_pad, stid, msgtag, msgctx, nbytes );
			
			free( usec_pad );

			str = event_str;

			break;
		}

		case HANDLE_TRECV:
		{
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			if ( TD->entry_exit == ENTRY_TEV )
			{
				str = trc_tevrec_string( TD, TR, tid, tsec, tusec );

				if ( str == NULL )
					return;

				taskState( T, tsec, tusec, STATE_IDLE, str );

				do_state = FALSE;

				break;
			}

			else if ( TD->entry_exit != EXIT_TEV )
				break;

			buf = TRC_GET_TEVREC_VALUE( TR, "MB", int );

			if ( buf > 0 )
			{
				msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int );

				msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int );

				nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

				stid = TRC_GET_TEVREC_VALUE( TR, "SRC", int );

				recv_message( T, tid, stid, msgtag, msgctx, nbytes,
					tsec, tusec );

				usec_pad = trc_pad_num( tusec, 6 );

				sprintf( event_str,
		"trecv.1 @ %d.%s: from 0x%x buf=%d code=%d ctxt=%d nbytes=%d.",
					tsec, usec_pad, stid, buf, msgtag, msgctx, nbytes );

				free( usec_pad );

				str = event_str;
			}

			else
			{
				usec_pad = trc_pad_num( tusec, 6 );

				sprintf( event_str,
					"trecv.1 @ %d.%s: no message, timed out.",
					tsec, usec_pad );

				free( usec_pad );

				str = event_str;
			}

			break;
		}

		default: break;
	}

	if ( do_state )
	{
		if ( str == NULL )
		{
			/* Get Task and Do Time */

			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

			if ( (T = get_task_tid( tid )) == NULL )
				return;

			normalize_time( T, &tsec, &tusec );

			check_time( tsec, tusec );

			/* Check for Unknown Send / Receive Events */

			if ( (msgtag = TRC_GET_TEVREC_VALUE( TR, "MC", int )) != -1
				&& (msgctx = TRC_GET_TEVREC_VALUE( TR, "MCX", int ))
						!= -1 )
			{
				/* (P)SEND */
				if ( (dtid = TRC_GET_TEVREC_VALUE( TR, "DST", int ))
						!= -1 )
				{
					nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

					if ( nbytes == -1
						&& (len = TRC_GET_TEVREC_VALUE( TR, "PC", int ))
								!= -1
						&& (dt = TRC_GET_TEVREC_VALUE( TR, "PDT", int ))
								!= -1 )
					{
						nbytes = get_plength( len, dt );
					}

					send_message( T, tid, dtid, msgtag, msgctx, nbytes,
						tsec, tusec );
				}

				/* (P)MCAST */
				else if ( TRC_GET_TEVREC( TR, tr_dtids, "MDL" ) )
				{
					nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

					if ( nbytes == -1
						&& (len = TRC_GET_TEVREC_VALUE( TR, "PC", int ))
								!= -1
						&& (dt = TRC_GET_TEVREC_VALUE( TR, "PDT", int ))
								!= -1 )
					{
						nbytes = get_plength( len, dt );
					}

					for ( i=0 ; i < tr_dtids->num ; i++ )
					{
						dtid = TRC_ARR_VALUE_OF( tr_dtids->value,
							int, i );

						if ( dtid != tid )
						{
							send_message( T, tid,
								dtid, msgtag, msgctx, nbytes,
								tsec, tusec );
						}
					}
				}

				/* RECV */
				else if (
					(stid = TRC_GET_TEVREC_VALUE( TR, "SRC", int ))
						!= -1 )
				{
					nbytes = TRC_GET_TEVREC_VALUE( TR, "MNB", int );

					recv_message( T, tid, stid, msgtag, msgctx, nbytes,
						tsec, tusec );
				}
			}

			/* Generate Generic Event String */

			str = trc_tevrec_string( TD, TR, tid, tsec, tusec );

			if ( str == NULL )
				return;
		}

		if ( TD->entry_exit == ENTRY_TEV )
			state = STATE_SYSTEM;

		else if ( TD->entry_exit == EXIT_TEV )
			state = STATE_RUNNING;

		else
			state = STATE_DEAD;

		taskState( T, tsec, tusec, state, str );
	}

	if ( do_exit )
		taskExit( T, tsec, tusec, str );

	if ( eflag )
	{
		if ( tid == -1 )
			tid = TRC_GET_TEVREC_VALUE( TR, "TID", int );

		if ( tid != -1 )
			printf( "PVM Event TID=0x%x: %s\n", tid, str );

		else
			printf( "PVM Event: %s\n", str );

		fflush( stdout );
	}

	if ( str != event_str && str != NULL )
		free( str );
}


int
get_plength( len, dt )
int len;
int dt;
{
	int nbytes;

	nbytes = len;

	switch ( dt )
	{
		case PVM_BYTE:
			nbytes *= sizeof(char);
			break;

		case PVM_SHORT:
		case PVM_USHORT:
			nbytes *= sizeof(short);
			break;

		case PVM_INT:
		case PVM_UINT:
			nbytes *= sizeof(int);
			break;

		case PVM_LONG:
		case PVM_ULONG:
			nbytes *= sizeof(long);
			break;

		case PVM_FLOAT:
			nbytes *= sizeof(float);
			break;

		case PVM_CPLX:
			nbytes *= sizeof(float) * 2;
			break;

		case PVM_DOUBLE:
			nbytes *= sizeof(double);
			break;

		case PVM_DCPLX:
			nbytes *= sizeof(double) * 2;
			break;

		case PVM_STR:
			nbytes = -1;
			break;

		default:
			nbytes = -1;
			break;
	}

	return( nbytes );
}


/* Handle Task Output */

void
task_output( tid, str )
int tid;
char *str;
{
	char tmp[4096];

	/* Insert TID Identifier */

	if ( !strcmp( str, "GOTEOF" ) )
		sprintf( tmp, "[0x%x] EOF", tid );

	else if ( strcmp( str, "CREATION" ) && strcmp( str, "GOTSPAWN" ) )
		sprintf( tmp, "[0x%x] %s", tid, str );

	else
		return;

	/* Create Output String Obj */

	add_twin_text( TASK_OUTPUT, tmp );
}


void
redraw_taskoutput()
{
	REFRESH_GLOBAL( TASK_OUTPUT->active );

	if ( !strcmp( CHAR_GLOBVAL( TASK_OUTPUT->active ), "TRUE" ) )
		Tcl_Eval( interp, "toUpdate" );
}


/* Handle Trace Event History */

void
task_tevhist( tid, sec, usec, str )
int tid;
int sec;
int usec;
char *str;
{
	char result[4096];
	char tmp[255];

	char *usec_pad;

	int buf;
	int len;
	int i;

	/* Make TID String Correct Length */

	sprintf( tmp, "[0x%x]", tid );

	len = strlen( tmp );

	buf = 11 - len;

	for ( i=0 ; i < buf ; i++ )
		*(tmp + len + i) = ' ';
	
	*(tmp + len + buf) = '\0';

	/* Create Trace Event String Obj */

	usec_pad = trc_pad_num( usec, 6 );

	sprintf( result, "%s(%d.%s) %s", tmp, sec, usec_pad, str );

	free( usec_pad );

	add_twin_text( TASK_TEVHIST, result );
}


void
redraw_tevhist()
{
	REFRESH_GLOBAL( TASK_TEVHIST->active );

	if ( !strcmp( CHAR_GLOBVAL( TASK_TEVHIST->active ), "TRUE" ) )
		Tcl_Eval( interp, "tvUpdate" );
}


/* Time Routines */

void
normalize_time( T, psec, pusec )
TASK T;
int *psec;
int *pusec;
{
	/* Adjust Host Delay Time */

	if ( T != NULL && T->host != NULL )
		sync_time( T->host, psec, pusec );

	/* Adjust Time Relative to Base Time */

	if ( BASE_TIME.tv_sec != -1 )
	{
		*psec -= BASE_TIME.tv_sec;
		*pusec -= BASE_TIME.tv_usec;

		if ( *pusec < 0 )
		{
			*pusec += 1000000;

			(*psec)--;
		}
	}

	else
	{
		*psec = 0;
		*pusec = 0;
	}
}


void
sync_time( H, psec, pusec )
TRC_HOST H;
int *psec;
int *pusec;
{
	if ( H != NULL )
	{
		*psec += H->delta.tv_sec;
		*pusec += H->delta.tv_usec;

		if ( *pusec > 1000000 )
		{
			*pusec -= 1000000;

			(*psec)++;
		}

		else if ( *pusec < 0 )
		{
			*pusec += 1000000;

			(*psec)--;
		}
	}
}


void
check_time( tsec, tusec )
int tsec;
int tusec;
{
	int flag;

	if ( BASE_TIME.tv_sec == -1 )
		return;

	flag = 0;

	if ( tsec > CURRENT_TIME.tv_sec )
		flag++;

	else if ( tsec == CURRENT_TIME.tv_sec
		&& tusec > CURRENT_TIME.tv_usec )
	{
		flag++;
	}

	if ( flag )
		update_time( tsec, tusec );
}


int
update_time( sec, usec )
int sec;
int usec;
{
	int new_timex;

	REFRESH_GLOBAL( SCALE );
	REFRESH_GLOBAL( TIMEX );

	CURRENT_TIME.tv_sec = sec;
	CURRENT_TIME.tv_usec = usec;

	if ( tflag )
		return( TCL_OK );

	new_timex = X_OF_TIME( sec, usec, INT_GLOBVAL( SCALE ) );

	CK_X_TIME( new_timex, sec, usec );

	if ( new_timex != INT_GLOBVAL( TIMEX ) )
		set_view_time( new_timex, FALSE );

	return( TCL_OK );
}


void
redraw_timelines()
{
	int timex;

	REFRESH_GLOBAL( SCALE );

	timex = X_OF_TIME( CURRENT_TIME.tv_sec, CURRENT_TIME.tv_usec,
		INT_GLOBVAL( SCALE ) );

	CK_X_TIME( timex, CURRENT_TIME.tv_sec, CURRENT_TIME.tv_usec );

	set_view_time( timex, TRUE );
}


void
set_view_time( timex, force )
int timex;
int force;
{
	char cmd[1024];

	char *usec_pad;

	int tmp_csize;
	int flag;
	int qtx;
	int wt;
	int fo;

	REFRESH_GLOBAL( ST_CANVAS_HEIGHT );
	REFRESH_GLOBAL( UT_CANVAS_HEIGHT );
	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FRAME_OFFSET );
	REFRESH_GLOBAL( TIMEID_PRIM );
	REFRESH_GLOBAL( ST_ACTIVE );
	REFRESH_GLOBAL( UT_ACTIVE );
	REFRESH_GLOBAL( ST_CWIDTH );
	REFRESH_GLOBAL( UT_CWIDTH );
	REFRESH_GLOBAL( ST_TIMEID );
	REFRESH_GLOBAL( UT_TIMEID );
	REFRESH_GLOBAL( ST_XVIEW );
	REFRESH_GLOBAL( UT_XVIEW );

	/* Adjust Control Panel Time Display */

	usec_pad = trc_pad_num( CURRENT_TIME.tv_usec, 6 );

	sprintf( cmd, "%s.time configure -text {Time: %d.%s}",
		CTRL, CURRENT_TIME.tv_sec, usec_pad );

	free( usec_pad );

	Tcl_Eval( interp, cmd );

	/* Skip Rest if Fast Forwarding... */

	if ( TRACE_STATUS == TRACE_FASTFWD && !force )
		return;

	/* Adjust TIMEX (timex) */

	SET_INT_TCL_GLOBAL( interp, TIMEX, timex );

	/* Get Frame Offset */

	fo = INT_GLOBVAL( FRAME_OFFSET ) * INT_GLOBVAL( FRAME_BORDER );

	tmp_csize = timex + ( 2 * ( INT_GLOBVAL( FRAME_BORDER ) + fo ) );

	flag = 0;

	/* Adjust Space-Time View Scrolling */

	if ( !strcmp( CHAR_GLOBVAL( ST_ACTIVE ), "TRUE" ) )
	{
		wt = tmp_csize - ST_SCROLL_MARK + 1;

		if ( wt >= INT_GLOBVAL( ST_CWIDTH ) )
		{
			ST_SCROLL_MARK = tmp_csize - INT_GLOBVAL( ST_CWIDTH ) + fo;

			if ( TIMEIDX != -1
				&& !strcmp( CHAR_GLOBVAL( TIMEID_PRIM ), "ST" ) )
			{
				qtx = TIMEIDX - INT_GLOBVAL( ST_XVIEW )
					+ ST_SCROLL_MARK;

				sprintf( cmd, "%s coords %d %d 0 %d %d",
					ST_C, INT_GLOBVAL( ST_TIMEID ), qtx, qtx,
					INT_GLOBVAL( ST_CANVAS_HEIGHT ) );

				Tcl_Eval( interp, cmd );

				sprintf( cmd, "%s raise %d",
					ST_C, INT_GLOBVAL( ST_TIMEID ) );

				Tcl_Eval( interp, cmd );

				sprintf( cmd, "%s coords %d %d 0 %d %d",
					UT_C, INT_GLOBVAL( UT_TIMEID ), qtx, qtx,
					INT_GLOBVAL( UT_CANVAS_HEIGHT ) );

				Tcl_Eval( interp, cmd );

				sprintf( cmd, "%s raise %d",
					UT_C, INT_GLOBVAL( UT_TIMEID ) );

				Tcl_Eval( interp, cmd );

				qtx -= INT_GLOBVAL( FRAME_BORDER ) + fo;

				if ( CORRELATE_LOCK == LOCKED )
					sprintf( cmd, "correlate_views %d", qtx );
				
				else
					sprintf( cmd, "set_query_time %d", qtx );

				Tcl_Eval( interp, cmd );
			}

			sprintf( cmd, "do_xview %s %d", ST_C, ST_SCROLL_MARK );

			Tcl_Eval( interp, cmd );

			SET_INT_TCL_GLOBAL( interp, ST_XVIEW, ST_SCROLL_MARK );

			sprintf( cmd, "scrollSet %s %d %d 1 %d",
				ST_SBH, tmp_csize, INT_GLOBVAL( ST_CWIDTH ),
				ST_SCROLL_MARK );

			Tcl_Eval( interp, cmd );
		}

		taskSTIncr( CURRENT_TIME.tv_sec, CURRENT_TIME.tv_usec );

		flag++;
	}

	/* Adjust Utilization View Scrolling */

	if ( !strcmp( CHAR_GLOBVAL( UT_ACTIVE ), "TRUE" ) )
	{
		wt = tmp_csize - UT_SCROLL_MARK + 1;

		if ( wt >= INT_GLOBVAL( UT_CWIDTH ) )
		{
			UT_SCROLL_MARK = tmp_csize - INT_GLOBVAL( UT_CWIDTH ) + fo;

			if ( TIMEIDX != -1
				&& !strcmp( CHAR_GLOBVAL( TIMEID_PRIM ), "UT" ) )
			{
				qtx = TIMEIDX - INT_GLOBVAL( UT_XVIEW )
					+ UT_SCROLL_MARK;

				sprintf( cmd, "%s coords %d %d 0 %d %d",
					UT_C, INT_GLOBVAL( UT_TIMEID ), qtx, qtx,
					INT_GLOBVAL( UT_CANVAS_HEIGHT ) );

				Tcl_Eval( interp, cmd );

				sprintf( cmd, "%s raise %d",
					UT_C, INT_GLOBVAL( UT_TIMEID ) );

				Tcl_Eval( interp, cmd );

				sprintf( cmd, "%s coords %d %d 0 %d %d",
					ST_C, INT_GLOBVAL( ST_TIMEID ), qtx, qtx,
					INT_GLOBVAL( ST_CANVAS_HEIGHT ) );

				Tcl_Eval( interp, cmd );

				sprintf( cmd, "%s raise %d",
					ST_C, INT_GLOBVAL( ST_TIMEID ) );

				Tcl_Eval( interp, cmd );

				qtx -= INT_GLOBVAL( FRAME_BORDER ) + fo;

				if ( CORRELATE_LOCK == LOCKED )
					sprintf( cmd, "correlate_views %d", qtx );
				
				else
					sprintf( cmd, "set_query_time %d", qtx );

				Tcl_Eval( interp, cmd );
			}

			sprintf( cmd, "do_xview %s %d", UT_C, UT_SCROLL_MARK );

			Tcl_Eval( interp, cmd );

			SET_INT_TCL_GLOBAL( interp, UT_XVIEW, UT_SCROLL_MARK );

			sprintf( cmd, "scrollSet %s %d %d 1 %d",
				UT_SBH, tmp_csize, INT_GLOBVAL( UT_CWIDTH ),
				UT_SCROLL_MARK );

			Tcl_Eval( interp, cmd );
		}

		utIncr( CURRENT_TIME.tv_sec, CURRENT_TIME.tv_usec );

		flag++;
	}

	if ( flag )
		Tcl_Eval( interp, "update" );
}


/* ARGSUSED */
int
update_scroll_marks_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FRAME_OFFSET );
	REFRESH_GLOBAL( TIMEX );

	ST_SCROLL_MARK = INT_GLOBVAL( TIMEX ) - INT_GLOBVAL( ST_CWIDTH )
		+ ( INT_GLOBVAL( FRAME_BORDER )
			* ( 2 + ( 3 * INT_GLOBVAL( FRAME_OFFSET ) ) ) );

	ST_SCROLL_MARK = ST_SCROLL_MARK < 0 ? 0 : ST_SCROLL_MARK;

	UT_SCROLL_MARK = INT_GLOBVAL( TIMEX ) - INT_GLOBVAL( UT_CWIDTH )
		+ ( INT_GLOBVAL( FRAME_BORDER )
			* ( 2 + ( 3 * INT_GLOBVAL( FRAME_OFFSET ) ) ) );

	UT_SCROLL_MARK = UT_SCROLL_MARK < 0 ? 0 : UT_SCROLL_MARK;

	return( TCL_OK );
}


/* Trace Utility Routines */

int
check_trace_overwrite_exists()
{
	FILE *fp;

	char tmp[1024];

	if ( TRACE_OVERWRITE_FLAG )
		return( TRUE );

	fp = fopen( ID->trace_file, "r" );

	if ( fp != NULL )
	{
		fclose( fp );

		sprintf( tmp, "prompt_trace_overwrite %s", ID->trace_file );

		Tcl_Eval( interp, tmp );

		return( FALSE );
	}

	else
		return( TRUE );
}

