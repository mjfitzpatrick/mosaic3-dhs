
static char rcsid[] =
	"$Id: xpvm.c,v 4.50 1998/04/09 21:12:11 kohl Exp $";

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


/* MAIN */

#ifndef IMA_WIN32

#if ( TCL_MAJOR_VERSION == 7 ) && ( TCL_MINOR_VERSION == 3 )

extern int main();

int *tclDummyMainPtr = (int *) main;

#else

int
main( argc, argv )
int argc;
char **argv;
{
	char **mung_argv;
	int mung_argc;
	int i;

	/* Mung Argv to Circumvent TK Stupidity */

	mung_argc = argc + 1;

	mung_argv = (char **) malloc( (unsigned) ( mung_argc + 1 )
		* sizeof( char * ) );
	trc_memcheck( mung_argv, "Munged Argv List" );

	mung_argv[0] = argv[0];

	mung_argv[1] = trc_copy_str( "-" );

	for ( i=2 ; i < mung_argc ; i++ )
		mung_argv[i] = argv[ i - 1 ];

	mung_argv[mung_argc] = (char *) NULL;

	/* Pass to TK */

	Tk_Main( mung_argc, mung_argv, Tcl_AppInit );

	return( 0 );
}

#endif


int
Tcl_AppInit( itp )
Tcl_Interp *itp;
{
	/* Initialize TCL / TK */

	if ( Tcl_Init( itp ) == TCL_ERROR )
		return( TCL_ERROR );

	if ( Tk_Init( itp ) == TCL_ERROR )
		return( TCL_ERROR );

	/* Initialize XPVM */

	if ( Xpvm_Init( itp ) == TCL_ERROR )
		return( TCL_ERROR );

	/* Set Up User-Specific Startup File */

#if ( TCL_MAJOR_VERSION > 7 ) || ( TCL_MINOR_VERSION >= 5 )

	Tcl_StaticPackage( itp, "Tk", Tk_Init,
		(Tcl_PackageInitProc *) NULL );

	Tcl_StaticPackage( itp, "XPVM", Xpvm_Init,
		(Tcl_PackageInitProc *) NULL );

	SET_TCL_GLOBAL( interp, "tcl_rcFileName", "~/.xpvmrc" );

#else

	tcl_RcFileName = "~/.xpvmrc";

#endif

	/* Return to tkMain to Start Tk_MainLoop() */

	return( TCL_OK );
}

#endif


int
Xpvm_Init( itp )
Tcl_Interp *itp;
{
	/* Save Interpreter */

	interp = itp;

	/* Read Command Line Args */

	if ( read_args() == TCL_ERROR )
		return( TCL_ERROR );

	/* Initialize Program Constants & Structs */

	if ( program_init() == TCL_ERROR )
		return( TCL_ERROR );

	/* Initialize TCL / TK */

	if ( tcl_init() == TCL_ERROR )
		return( TCL_ERROR );

	/* Get Main Window */

	Top = Tk_MainWindow( interp );

	if ( Top == NULL )
		return( TCL_ERROR );

	Disp = Tk_Display( Top );

	/* Initialize PVM */

	if ( pvm_init() == TCL_ERROR )
		return( TCL_ERROR );

	/* Initialize Tracer */

	if ( trc_init() == TCL_ERROR )
		return( TCL_ERROR );

	/* Set Up Interface */

	if ( window_init() == TCL_ERROR )
		return( TCL_ERROR );

	/* Return */

	return( TCL_OK );
}


int
read_args()
{
	char tmp[1024];

	char *argv_str;

	int i, j, k;
	int do_usage;
	int len;

	argv_str = GET_TCL_GLOBAL( interp, "argv" );

	if ( Tcl_SplitList( interp, argv_str, &Argc, &Argv ) == TCL_ERROR )
		return( TCL_ERROR );

	HOSTFILE = (char *) NULL;

	TRACE_FILE_STATUS = TRACE_FILE_OVERWRITE;

	MSG_DEFAULT_NBYTES = 64;

	Nflag = 0;

	eflag = 0;
	tflag = 0;
	vflag = 0;

	do_usage = 0;

	for ( i=0 ; i < Argc ; i++ )
	{
		if ( Argv[i][0] == '-' )
		{
			k = i + 1;

			len = strlen( Argv[i] );

			for ( j=0 ; j < len ; j++ )
			{
				switch ( Argv[i][j] )
				{
					case 'H': usage(); break;

					case 'N':
					{
						if ( TRC_HOST_NAME != NULL )
							free( TRC_HOST_NAME );

						TRC_HOST_NAME = trc_copy_str( Argv[k++] );

						Nflag++;

						break;
					}

					case 'P':
					{
						TRACE_FILE_STATUS = TRACE_FILE_PLAYBACK;

						break;
					}

					case 'O':
					{
						TRACE_FILE_STATUS = TRACE_FILE_OVERWRITE;

						break;
					}

					case 'T':
					{
						sprintf( tmp, "set trace_file \"%s\"",
							Argv[k++] );

						Tcl_Eval( interp, tmp );

						break;
					}

					case 'M':
					{
						MSG_DEFAULT_NBYTES = atoi( Argv[k++] );

						break;
					}

					case 'S':
						trc_dump_old_sddf_headers();
						break;

					case 'e': eflag++; break;

					case 't': tflag++; break;

					case 'v':
					{
						vflag++;

						break;
					}	

					case '-': break;

					default:
					{
						printf( "Unknown Option -%c\n", Argv[i][j] );

						do_usage++;

						break;
					}
				}
			}

			i = k - 1;
		}

		/* Hostfile */
		else
		{
			if ( HOSTFILE != NULL )
				free( HOSTFILE );

			HOSTFILE = trc_copy_str( Argv[i] );
		}
	}

	if ( do_usage )
		usage();

	return( TCL_OK );
}


void
usage()
{
	printf( "\nusage:  xpvm [ hostfile ] [ -PO ] [ -T trace ]" );
	printf( " [ -N name ]" );
	printf( "\n\t" );
	printf( " [ -M nbytes ]" );
	printf( " [ -HSetv ]" );
	printf( "\n" );

	printf( "\n" );

	printf( "where:\n" );
	printf( "------\n" );

	printf( "hostfile   = Alternate XPVM Hostfile (~/.xpvm_hosts)\n" );

	printf( "-P\t   = Start XPVM in PlayBack Mode\n" );
	printf( "-O\t   = Start XPVM in OverWrite Mode (default)\n" );

	printf( "-T trace   = Use \"trace\" as Default Trace File\n" );

	printf( "-N name\t   = Use \"name\" as Local Network Hostname\n" );

	printf( "-M nbytes  = Set Default Message Size to \"nbytes\"\n" );

	printf( "-H\t   = Print This Help Information\n" );
	printf( "-S\t   = Dump SDDF Trace File Headers (and exit)\n" );

	/* printf( "-Q\t   = Use Cached Group Servers\n" ); */

	printf( "-e\t   = Dump Raw PVM Event Text\n" );
	printf( "-t\t   = Text Mode, No Animation in Views\n" );
	printf( "-v\t   = Verbose Operation\n" );

	printf( "\n" );

	exit( 0 );
}


int
program_init()
{
#ifndef IMA_WIN32
	struct passwd *pw;
#endif

	char pvstr[255];
	char tmp[2048];

	char *getenv();

	char *xpvm_dir;
	char *stripped;
	char *uppered;
	char *home;

	int entry_exit;
	int release;
	int version;
	int fmt;
	int i;

	/*
	 * Signals Set in pvm_init() - after pvmd started 
	 */

	/* Get Home Directory */

	home = getenv("HOME");

	if ( home == NULL )
	{
#ifndef IMA_WIN32
		if ( (pw = getpwuid( getuid() )) != NULL )
			home = pw->pw_dir;

		else
		{
#endif
			sprintf( tmp, "%c", FILE_SEP_CHAR );
			home = tmp;
#ifndef IMA_WIN32
		}
#endif

		HOME_DIR = trc_copy_str( home );
	}

	else
		HOME_DIR = home;

	/* Get XPVM Main Directory */

	xpvm_dir = getenv("XPVM_ROOT");

	if ( xpvm_dir == NULL )
	{
		sprintf( tmp, "%s", XPVM_DEFAULT_DIR );

		if ( tmp[0] != FILE_SEP_CHAR )
		{
			sprintf( tmp, "%s%c%s",
				HOME_DIR, FILE_SEP_CHAR, XPVM_DEFAULT_DIR );
		}

		XPVM_DIR = trc_copy_str( tmp );
	}

	else
		XPVM_DIR = xpvm_dir;

	/* Check Hostfile */

	if ( HOSTFILE == NULL )
	{
		sprintf( tmp, "%s", DEFAULT_HOSTFILE );

		if ( tmp[0] != FILE_SEP_CHAR )
		{
			sprintf( tmp, "%s%c%s",
				HOME_DIR, FILE_SEP_CHAR, DEFAULT_HOSTFILE );
		}
		
		HOSTFILE = trc_copy_str( tmp );
	}

	/* Allocate Initial Add Hosts Array */

	ADD_HOSTS_SIZE = 16;

	ADD_HOSTS = (TRC_HOST *) malloc( (unsigned) ADD_HOSTS_SIZE
		* sizeof( TRC_HOST ) );
	trc_memcheck( ADD_HOSTS, "Add Hosts List" );

	ADD_HOSTS_NAMES = (char **) malloc( (unsigned) ADD_HOSTS_SIZE
		* sizeof( char * ) );
	trc_memcheck( ADD_HOSTS_NAMES, "Add Hosts Names List" );

	for ( i=0 ; i < ADD_HOSTS_SIZE ; i++ )
	{
		ADD_HOSTS[i] = (TRC_HOST) NULL;

		ADD_HOSTS_NAMES[i] = (char *) NULL;
	}

	NADDS = 0;

	/* Messages List */

	PENDING_MSG_LIST = (MSG) NULL;

	PENDING_MSG_GRID = (MSG_GRID **) NULL;

	MSG_GRID_SIZE = 0;

	MSG_LIST = (MSG) NULL;

	MQ_MAX_NBYTES = 1;

	/* Tasks List */

	TASK_LIST = (TASK) NULL;

	NTASKS = 0;

	TASK_OUTPUT = create_twin();

	TASK_OUTPUT->search = create_search();

	TASK_OUTPUT->search->direction = SEARCH_FORWARD;

	TASK_OUTPUT->needs_redraw = FALSE;

	TASK_TEVHIST = create_twin();

	TASK_TEVHIST->search = create_search();

	TASK_TEVHIST->search->direction = SEARCH_FORWARD;

	TASK_TEVHIST->needs_redraw = FALSE;

	/* Utilization State List */

	UT_LIST = (UT_STATE) NULL;

	UT_MAX_NTASKS = 1;

	/* Main Network */

	MAIN_NET = create_network();

	MAIN_NET->type = NETWORK_BUS;

	/* Host File Hosts List */

	HOST_LIST = (TRC_HOST) NULL;

	NHOSTS = 0;

	/* Architectures List */

	ARCH_LIST = (ARCH) NULL;

	/* Trace Mask Group Lists */

	CURRENT_GROUP_LIST = (TRACE_MASK_GROUP *) NULL;

	TM33_GROUP_LIST = (TRACE_MASK_GROUP) NULL;
	TM_GROUP_LIST = (TRACE_MASK_GROUP) NULL;

	/* PVM 3.3 Trace Mask Index List */

	CURRENT_INDEX_LIST = (TRACE_MASK_INDEX *) NULL;

	TM33_INDEX_LIST = (TRACE_MASK_INDEX) NULL;
	TM_INDEX_LIST = (TRACE_MASK_INDEX) NULL;

	/* PVM 3.3 TRACING INIT */

	/* PVM 3.3 Trace Mask Indices */

	for ( i=TRC_OLD_TEV_FIRST ; i <= TRC_OLD_TEV_MAX ; i++ )
	{
		stripped = strip33_name( TRC_OLD_TEV_TRACE_NAMES[i],
			&entry_exit );

		if ( entry_exit == TRC_ENTRY_TEV )
			set_trace_mask_index( &TM33_INDEX_LIST, stripped, i );

		TEV33_TYPE[i] = entry_exit;
	}

	for ( i=(TRC_OLD_TEV_MAX + 1) ; i < TRC_OLD_TRACE_MAX ; i++ )
		TEV33_TYPE[i] = TRC_IGNORE_TEV;

	/* Special PVM 3.3 Event Types */

	TEV33_TYPE[ TRC_OLD_TEV_NEWTASK ]	= TRC_IGNORE_TEV;
	TEV33_TYPE[ TRC_OLD_TEV_SPNTASK ]	= TRC_IGNORE_TEV;
	TEV33_TYPE[ TRC_OLD_TEV_ENDTASK ]	= TRC_IGNORE_TEV;

	/* PVM 3.4 TRACING INIT */

	/* PVM 3.4 Trace Mask Indices */

	for ( i=TEV_FIRST ; i <= TEV_MAX ; i++ )
	{
		uppered = upper_str( pvmtevinfo[i].name );

		set_trace_mask_index( &TM_INDEX_LIST, uppered, i );
	}

	/* Set Default Trace Buffering */

	TRACE_BUF = 0;

#ifndef USE_PVM_33

	/* Set Default Trace Option */

	TRACE_OPT = PvmTraceFull;

#endif

	/* Set Trace Format */

	strcpy( pvstr, pvm_version() );

	if ( sscanf( pvstr, "%d.%d", &release, &version ) != 2 )
	{
		printf( "\nError: Unknown PVM Version Identifier \"%s\"\n\n",
			pvstr );
	}

	else
	{
		if ( release == 3 )
		{
			if ( version < 3 )
			{
				printf( "Error: Tracing Not Supported for PVM %d.%d\n",
					release, version );

				exit( 1 );
			}

			else if ( version == 3 )
				fmt = TEV_FMT_33;

			else
				fmt = TEV_FMT_34;
		}

		else
			fmt = TEV_FMT_34;

		set_trace_format( fmt, FALSE );
	}

	/* Initialize Time Vars */

	PROCESS_COUNTER_OVERWRITE = 20;
	PROCESS_COUNTER_PLAYBACK = 100;

	RECV_COUNTER_OVERWRITE = 20;
	RECV_COUNTER_PLAYBACK = 50;

	RECV_TIMER_OVERWRITE = 40;
	RECV_TIMER_PLAYBACK = 100;

	CURRENT_TIME.tv_sec = -1;
	CURRENT_TIME.tv_usec = -1;

	BASE_TIME.tv_sec = -1;
	BASE_TIME.tv_usec = -1;

	ST_SCROLL_MARK = 0;
	UT_SCROLL_MARK = 0;

	TASK_SORT = TASK_SORT_ALPHA;

	TIMEIDX = -1;

	/* Initialize Trace Globals */

	if ( TRACE_FILE_STATUS == TRACE_FILE_OVERWRITE )
	{
		PROCESS_COUNTER_INTERVAL = PROCESS_COUNTER_OVERWRITE;
		RECV_COUNTER_INTERVAL = RECV_COUNTER_OVERWRITE;

		RECV_TIMER_INTERVAL = RECV_TIMER_OVERWRITE;
	}

	else if ( TRACE_FILE_STATUS == TRACE_FILE_PLAYBACK )
	{
		PROCESS_COUNTER_INTERVAL = PROCESS_COUNTER_PLAYBACK;
		RECV_COUNTER_INTERVAL = RECV_COUNTER_PLAYBACK;

		RECV_TIMER_INTERVAL = RECV_TIMER_PLAYBACK;
	}

	TRACE_MODE_PENDING = TRACE_MODE_NONE;

	TRACE_PROCESS_LOCK = UNLOCKED;

	TRACE_OVERWRITE_FLAG = FALSE;

	TRACE_STATUS = TRACE_FWD;

	TRACE_DO_SPAWN = FALSE;

	TRACE_PENDING = 0;

	TRACE_MSG_ACTIVE = FALSE;

	TRACE_ACTIVE = FALSE;

	/* Software Locks */

	CORRELATE_LOCK = UNLOCKED;

	INTERFACE_LOCK = UNLOCKED;

	TASK_OUT_LOCK = UNLOCKED;

	PROCESS_LOCK = UNLOCKED;

	RECV_LOCK = UNLOCKED;

	TICK_LOCK = UNLOCKED;

	STARTUP = LOCKED;

	/* Misc Globals */

	NEEDS_REDRAW_NT = FALSE;
	NEEDS_REDRAW_ST = FALSE;
	NEEDS_REDRAW_UT = FALSE;
	NEEDS_REDRAW_MQ = FALSE;
	NEEDS_REDRAW_CT = FALSE;

	GROUPS_ALIVE = FALSE;

	REGISTERED = FALSE;

	return( TCL_OK );
}


int
tcl_init()
{
	char tmp[255];

	/* Set TCL versions globals */

	SET_TCL_GLOBAL( interp, "tcl_version", TCL_VERSION );

	sprintf( tmp, "%d", TCL_MAJOR_VERSION );
	SET_TCL_GLOBAL( interp, "tcl_major_version", tmp );

	sprintf( tmp, "%d", TCL_MINOR_VERSION );
	SET_TCL_GLOBAL( interp, "tcl_minor_version", tmp );

	/* Set TK versions globals */

	SET_TCL_GLOBAL( interp, "tk_version", TK_VERSION );

	sprintf( tmp, "%d", TK_MAJOR_VERSION );
	SET_TCL_GLOBAL( interp, "tk_major_version", tmp );

	sprintf( tmp, "%d", TK_MINOR_VERSION );
	SET_TCL_GLOBAL( interp, "tk_minor_version", tmp );

	/* Create TCL Commands for Action Routines */

	/* Main Console Commands */

	Tcl_CreateCommand( interp, "reset_trace_file",
		reset_trace_file_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "reset_views", reset_views_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "reset_pvm", reset_pvm_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "pvm_hosts", hosts_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "pvm_tasks", tasks_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "pvm_quit", (Tcl_CmdProc *) quit_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "pvm_halt", (Tcl_CmdProc *) halt_proc,
		(ClientData) NULL, (vfp) NULL );

	/* Trace Playback Commands */

	Tcl_CreateCommand( interp, "get_initial_trace_file_status",
		get_initial_trace_file_status_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_file_status_handle",
		trace_file_status_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_overwrite_result",
		trace_overwrite_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_file_handle", trace_file_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "get_trace_format",
		get_trace_format_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "pvm_trace_mask", trace_mask_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "set_trace_buffer", trace_buffer_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_rewind", playback_proc,
		(ClientData) TRACE_REWIND, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_stop", playback_proc,
		(ClientData) TRACE_STOP, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_fastfwd", playback_proc,
		(ClientData) TRACE_FASTFWD, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_fwd", playback_proc,
		(ClientData) TRACE_FWD, (vfp) NULL );

	Tcl_CreateCommand( interp, "trace_fwdstep", playback_proc,
		(ClientData) TRACE_FWDSTEP, (vfp) NULL );

	/* Network View Commands */

	Tcl_CreateCommand( interp, "netSetMenuvar",
		network_set_menuvar, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "netUpdate",
		network_update_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "netReset",
		network_reset_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "netHostFind",
		network_host_find, (ClientData) NULL, (vfp) NULL );

	/* Space-Time Task Commands */

	Tcl_CreateCommand( interp, "set_task_sort",
		set_task_sort_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskArrange",
		task_arrange_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskCTUpdate",
		task_ct_update_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskSTUpdate",
		task_st_update_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskCTQuery",
		task_ct_query_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskLabel",
		task_label_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskQuery",
		task_query_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskReset",
		task_reset_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "taskZoom",
		task_zoom_proc, (ClientData) NULL, (vfp) NULL );

	/* Space-Time Communication Commands */

	Tcl_CreateCommand( interp, "commArrange",
		comm_arrange_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "commQuery",
		comm_query_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "commZoom",
		comm_zoom_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "mqArrange",
		mq_arrange_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "mqUpdate",
		mq_update_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "mqQuery",
		mq_query_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "mqReset",
		mq_reset_proc, (ClientData) NULL, (vfp) NULL );

	/* Utilization View Commands */

	Tcl_CreateCommand( interp, "utAdjustHeight",
		ut_adjust_height_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "utUpdate",
		ut_update_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "utReset",
		ut_reset_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "utZoom",
		ut_zoom_proc, (ClientData) NULL, (vfp) NULL );

	/* Menu & Miscellaneous Commands */

	Tcl_CreateCommand( interp, "initialize_tcl_globals",
		initialize_tcl_globals_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "get_xpvm_default_dir",
		get_xpvm_default_dir_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "update_scroll_marks",
		update_scroll_marks_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "define_trace_mask",
		define_trace_mask_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "toUpdate", task_out_update_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "output_file_handle", output_file_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "output_filter_handle",
		output_filter_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "output_search_handle",
		output_search_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "tvUpdate",
		task_tevhist_update_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "tevhist_filter_handle",
		tevhist_filter_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "tevhist_search_handle",
		tevhist_search_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "file_overwrite_result",
		file_overwrite_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "toggle_search", toggle_search_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "load_bitmap_file", load_bitmap_file,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "pvm_get_tasks", get_tasks_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "fix_help_line", fix_help_line_cmd,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "strip_label", strip_label_cmd,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "title_info", title_info_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "get_hosts_menu_list",
		get_hosts_menu_list, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "define_arch", define_arch,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "double_click", double_click_proc,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "check_max_scale",
		check_max_scale_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "set_query_time",
		set_query_time_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "correlate_views",
		correlate_views_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "timeZoom",
		time_zoom_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "pad", pad_cmd,
		(ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "interface_lock",
		interface_lock_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "set_net_volume_levels",
		set_net_volume_levels_proc, (ClientData) NULL, (vfp) NULL );

	Tcl_CreateCommand( interp, "get_net_volume_level",
		get_net_volume_level_proc, (ClientData) NULL, (vfp) NULL );

	return( TCL_OK );
}


int
pvm_init()
{
	char tmp[1024];

	char *av[4];
	int ac;

	int inum;
	int se;
	int cc;
	int i;

	/* Start PVM (if necessary) */

	av[0] = (char *) NULL;

	ac = 0;

	if ( HOSTFILE != NULL )
		av[ac++] = trc_copy_str( HOSTFILE );

	if ( Nflag )
	{
		sprintf( tmp, "-n%s", TRC_HOST_NAME );

		av[ac++] = trc_copy_str( tmp );
	}

	pvm_setopt( PvmResvTids, 1 );

	pvm_setopt( PvmRoute, PvmDontRoute );

	se = pvm_setopt( PvmAutoErr, 0 );

	cc = pvm_start_pvmd( ac, av, FALSE );

	if ( cc < 0 )
	{
		if ( cc == PvmDupHost )
		{
			printf( "Connecting to PVMD already running... " );
			fflush( stdout );

			NEW_PVM = FALSE;
		}

		else
		{
			pvm_perror( "Can't Start PVM" );

			exit( -1 );
		}
	}

	else
	{
		printf( "New PVMD started... " );
		fflush( stdout );

		NEW_PVM = TRUE;
	}

	pvm_setopt( PvmAutoErr, se );

	/* Get My TID */

	MYTID = pvm_mytid();

	if ( MYTID < 0 )
	{
		pvm_perror( "Error Joining PVM" );

		exit( -1 );
	}

	else
	{
		printf( "XPVM %s connected as TID=0x%x.\n",
			XPVM_VERSION, MYTID );
	}

	/* Set Desired Signals After pvm_mytid() - match new console */

#ifndef USE_PVM_33
	pvm_setopt( PvmNoReset, 1 );
#else
	signal( SIGTERM, SIG_IGN );
#endif

#ifndef IMA_WIN32
	signal( SIGALRM, SIG_IGN );
#endif

	signal( SIGINT, quit_proc );
#ifndef IMA_WIN32
	signal( SIGQUIT, quit_proc );
#endif

	/* Set PVM Options */

	set_trace_buffer( TRACE_BUF, FALSE );

	set_trace_options( TRACE_OPT, FALSE );

	trace_mask_init( TRACE33_MASK, &TM33_GROUP_LIST, TEV_FMT_33 );

	trace_mask_init( TRACE_MASK, &TM_GROUP_LIST, TEV_FMT_34 );

	INIT_TRACE_MASK( TRACE33_MASK_CLEAR, TEV_FMT_33 );

	INIT_TRACE_MASK( TRACE_MASK_CLEAR, TEV_FMT_34 );

	pvm_settmask( PvmTaskChild, CURRENT_TRACE_MASK );

#ifdef USE_PVM_33

	/* Set Up XPVM Group, Start Group Server (if necessary) */

	printf( "Setting up XPVM Group... " );
	fflush( stdout );

	inum = pvm_joingroup( "xpvm" );

	if ( inum != 0 )
	{
		if ( inum < 0 )
			pvm_perror( "Joining XPVM Group" );
		
		else
		{
			printf( "\nWarning: XPVM Already in Group, Instance=%d\n\n",
				inum );
		}
	}

	else
		printf( "done.\n" );

#endif

	/* Check for Whacked PVM Before Proceeding */
	/* (in case group call hung and PVM was killed) */

	cc = pvm_config( (int *) NULL, (int *) NULL,
		(struct pvmhostinfo **) NULL );

	if ( cc < 0 )
		error_exit();

#ifdef USE_PVM_33

	/* Otherwise, Assume the Best. */

	GROUPS_ALIVE = TRUE;

#endif

	read_hostfile();

	return( TCL_OK );
}


void
trace_mask_init( TMASK, TMG_LIST, fmt )
char *TMASK;
TRACE_MASK_GROUP *TMG_LIST;
int fmt;
{
	TRACE_MASK_GROUP TMG;

	char name[255];

	int j;

	/* Initialize Trace Mask */

	INIT_TRACE_MASK( TMASK, fmt );

	/* Do ALL_EVENTS Group Mask Hard-Wired */

	TMG = *TMG_LIST = create_trace_mask_group();

	sprintf( name, "ALL_%s_EVENTS",
		fmt == TEV_FMT_33 ? "3_3" : "3_4" );

	TMG->name = trc_copy_str( name );

	TMG->tmask = (char *) malloc( (unsigned) TRACE_MASK_LENGTH( fmt )
		* sizeof(char) );
	trc_memcheck( TMG->tmask, "Group Trace Mask" );

	INIT_TRACE_MASK( TMG->tmask, fmt );

	for ( j=FIRST_EVENT( fmt ) ; j <= LAST_EVENT( fmt ) ;
		j += EVENT_INCR( fmt ) )
	{
		SET_TRACE_MASK( TMG->tmask, fmt, j );
	}
}


int
trc_init()
{
	/* Initialize Tracer */

	trc_tracer_init();

	/* Set Tracer Globals */

	TRC_HOST_ADD_NOTIFY_CODE = 99;
	TRC_HOST_DEL_NOTIFY_CODE = 100;

	TRC_VERSION = XPVM_VERSION;

	TRC_NAME = "XPVM";

	TRC_TID = MYTID;

	/* Get Tracer ID */

	ID = trc_get_tracer_id();

	/* Define User Handler Routines */

	ID->status_msg = status_msg_safe;
	ID->handle_host_add_notify = handle_host_add_notify;
	ID->handle_host_del_notify = handle_host_del_notify;
	ID->handle_event = handle_event;
	ID->handle_old_event = handle_old_event;

	/* Set Trace Event Message Codes */

#ifndef USE_PVM_33
	ID->event_ctx = pvm_getcontext();
#endif
	ID->event_tag = 666;

#ifndef USE_PVM_33
	ID->output_ctx = pvm_getcontext();
#endif
	ID->output_tag = 667;

	/* Set PVM Tracing Options */

	trc_set_tracing_codes( ID );

	/* Set Up Trace Event Lookup Tries */

	init_event_trie();

	/* Return O.K. Status */

	return( TCL_OK );
}


int
window_init()
{
	static char fname[1024];

	FILE *fptest;

	/* Set up GLOBAL structs */

	define_tcl_globals();

	/* Create Interface - TCL Script */

	fptest = fopen( "xpvm.tcl", "r" );

	if ( fptest != NULL )
	{
		strcpy( fname, "xpvm.tcl" );

		fclose( fptest );
	}

	else
		sprintf( fname, "%s/xpvm.tcl", XPVM_DIR );

	if ( Tcl_EvalFile( interp, fname ) == TCL_ERROR )
		return( TCL_ERROR );

	/* Check Hosts */

	trc_initialize_hosts( ID );

	/* Check Host File Hosts */

	handle_hostfile_hosts();

	/* If Host Status Events are Pending, Do Them Now */

	if ( !TRACE_PENDING )
		TRACE_PENDING = trc_save_host_status_events( ID );

	/* Initialize Trace Controls Setting */

	set_trace_controls();

	/* Release the Hounds...  :-) */

	STARTUP = UNLOCKED;

	/* Set Up Event Polling Proc */

	Tk_DoWhenIdle( (Tk_IdleProc *) recv_event_proc, (ClientData) NULL );

	return( TCL_OK );
}


void
define_tcl_globals()
{
	/* Drawing Constants */

	FRAME_BORDER = MAKE_TCL_GLOBAL( "frame_border", TCL_GLOBAL_INT );

	FRAME_OFFSET = MAKE_TCL_GLOBAL( "FRAME_OFFSET", TCL_GLOBAL_INT );

	BORDER_SPACE = MAKE_TCL_GLOBAL( "border_space", TCL_GLOBAL_INT );

	ROW_HEIGHT = MAKE_TCL_GLOBAL( "row_height", TCL_GLOBAL_INT );

	COL_WIDTH = MAKE_TCL_GLOBAL( "col_width", TCL_GLOBAL_INT );

	FIXED_ROW_HEIGHT = MAKE_TCL_GLOBAL( "fixed_row_height",
		TCL_GLOBAL_INT );

	FIXED_COL_WIDTH = MAKE_TCL_GLOBAL( "fixed_col_width",
		TCL_GLOBAL_INT );

	DEPTH = MAKE_TCL_GLOBAL( "depth", TCL_GLOBAL_INT );

	/* Network Constants */

	NET_CHEIGHT = MAKE_TCL_GLOBAL( "net_cheight", TCL_GLOBAL_INT );

	NET_HHEIGHT = MAKE_TCL_GLOBAL( "net_hheight", TCL_GLOBAL_INT );

	NET_CWIDTH = MAKE_TCL_GLOBAL( "net_cwidth", TCL_GLOBAL_INT );

	NET_HWIDTH = MAKE_TCL_GLOBAL( "net_hwidth", TCL_GLOBAL_INT );

	ICON_BORDER = MAKE_TCL_GLOBAL( "icon_border", TCL_GLOBAL_INT );

	ICON_SPACE = MAKE_TCL_GLOBAL( "icon_space", TCL_GLOBAL_INT );

	NET_ACTIVE = MAKE_TCL_GLOBAL( "net_active", TCL_GLOBAL_CHAR );

	NET_SCROLL = MAKE_TCL_GLOBAL( "net_scroll", TCL_GLOBAL_INT );

	NET_FLASH = MAKE_TCL_GLOBAL( "net_flash", TCL_GLOBAL_INT );

	NET_SPACE = MAKE_TCL_GLOBAL( "net_space", TCL_GLOBAL_INT );

	NET_XVIEW = MAKE_TCL_GLOBAL( "net_xview", TCL_GLOBAL_INT );

	NET_YVIEW = MAKE_TCL_GLOBAL( "net_yview", TCL_GLOBAL_INT );

	NET_SIZE = MAKE_TCL_GLOBAL( "net_size", TCL_GLOBAL_INT );

	/* Space-Time Constants */

	ST_CANVAS_HEIGHT = MAKE_TCL_GLOBAL( "st_canvas_height",
		TCL_GLOBAL_INT );

	ST_TIMELINE = MAKE_TCL_GLOBAL( "st_timeline", TCL_GLOBAL_INT );

	ST_CHEIGHT = MAKE_TCL_GLOBAL( "st_cheight", TCL_GLOBAL_INT );

	ST_CWIDTH = MAKE_TCL_GLOBAL( "st_cwidth", TCL_GLOBAL_INT );

	ST_LABEL_HT = MAKE_TCL_GLOBAL( "st_label_ht", TCL_GLOBAL_INT );

	ST_RECT_HT = MAKE_TCL_GLOBAL( "st_rect_ht", TCL_GLOBAL_INT );

	STL_YVIEW = MAKE_TCL_GLOBAL( "stl_yview", TCL_GLOBAL_INT );

	ST_ACTIVE = MAKE_TCL_GLOBAL( "st_active", TCL_GLOBAL_CHAR );

	ST_TIMEID = MAKE_TCL_GLOBAL( "st_timeid", TCL_GLOBAL_INT );

	ST_XVIEW = MAKE_TCL_GLOBAL( "st_xview", TCL_GLOBAL_INT );

	TIMEID_PRIM = MAKE_TCL_GLOBAL( "timeid_prim", TCL_GLOBAL_CHAR );

	TIMEX = MAKE_TCL_GLOBAL( "timex", TCL_GLOBAL_INT );

	SCALE = MAKE_TCL_GLOBAL( "scale", TCL_GLOBAL_INT );

	/* Utilization View Constants */

	UT_CANVAS_HEIGHT = MAKE_TCL_GLOBAL( "ut_canvas_height",
		TCL_GLOBAL_INT );

	UT_CHEIGHT = MAKE_TCL_GLOBAL( "ut_cheight", TCL_GLOBAL_INT );

	UT_CWIDTH = MAKE_TCL_GLOBAL( "ut_cwidth", TCL_GLOBAL_INT );

	UT_ACTIVE = MAKE_TCL_GLOBAL( "ut_active", TCL_GLOBAL_CHAR );

	UT_TIMEID = MAKE_TCL_GLOBAL( "ut_timeid", TCL_GLOBAL_INT );

	UT_XVIEW = MAKE_TCL_GLOBAL( "ut_xview", TCL_GLOBAL_INT );

	/* Message Queue Constants */

	MQ_TASK_WT = MAKE_TCL_GLOBAL( "mq_task_wt", TCL_GLOBAL_INT );

	MQ_CHEIGHT = MAKE_TCL_GLOBAL( "mq_cheight", TCL_GLOBAL_INT );

	MQ_ACTIVE = MAKE_TCL_GLOBAL( "mq_active", TCL_GLOBAL_CHAR );

	MQ_CWIDTH = MAKE_TCL_GLOBAL( "mq_cwidth", TCL_GLOBAL_INT );

	MQ_XVIEW = MAKE_TCL_GLOBAL( "mq_xview", TCL_GLOBAL_INT );

	/* Call Trace Constants */

	CT_QUERY_INDEX = MAKE_TCL_GLOBAL( "ct_query_index",
		TCL_GLOBAL_INT );

	CT_CHEIGHT = MAKE_TCL_GLOBAL( "ct_cheight", TCL_GLOBAL_INT );

	CT_ACTIVE = MAKE_TCL_GLOBAL( "ct_active", TCL_GLOBAL_CHAR );

	CT_YVIEW = MAKE_TCL_GLOBAL( "ct_yview", TCL_GLOBAL_INT );

	/* Task Output Constants */

	TO_CHEIGHT = MAKE_TCL_GLOBAL( "to_cheight", TCL_GLOBAL_INT );

	TO_ACTIVE = MAKE_TCL_GLOBAL( "to_active", TCL_GLOBAL_CHAR );

	TO_YVIEW = MAKE_TCL_GLOBAL( "to_yview", TCL_GLOBAL_INT );

	TO_LASTY = MAKE_TCL_GLOBAL( "to_lasty", TCL_GLOBAL_INT );

	TASK_OUTPUT->cheight = TO_CHEIGHT;
	TASK_OUTPUT->active = TO_ACTIVE;
	TASK_OUTPUT->yview = TO_YVIEW;
	TASK_OUTPUT->lasty = TO_LASTY;

	/* Trace Event History Constants */

	TV_CHEIGHT = MAKE_TCL_GLOBAL( "tv_cheight", TCL_GLOBAL_INT );

	TV_ACTIVE = MAKE_TCL_GLOBAL( "tv_active", TCL_GLOBAL_CHAR );

	TV_YVIEW = MAKE_TCL_GLOBAL( "tv_yview", TCL_GLOBAL_INT );

	TV_LASTY = MAKE_TCL_GLOBAL( "tv_lasty", TCL_GLOBAL_INT );

	TASK_TEVHIST->cheight = TV_CHEIGHT;
	TASK_TEVHIST->active = TV_ACTIVE;
	TASK_TEVHIST->yview = TV_YVIEW;
	TASK_TEVHIST->lasty = TV_LASTY;

	/* Colors */

	NET_RUNNING_COLOR = MAKE_TCL_GLOBAL( "net_running_color",
		TCL_GLOBAL_CHAR );

	NET_SYSTEM_COLOR = MAKE_TCL_GLOBAL( "net_system_color",
		TCL_GLOBAL_CHAR );

	NET_IDLE_COLOR = MAKE_TCL_GLOBAL( "net_idle_color",
		TCL_GLOBAL_CHAR );

	NET_EMPTY_COLOR = MAKE_TCL_GLOBAL( "net_empty_color",
		TCL_GLOBAL_CHAR );

	NET_FG_COLOR = MAKE_TCL_GLOBAL( "net_fg_color", TCL_GLOBAL_CHAR );

	NET_ALIVE_COLOR = MAKE_TCL_GLOBAL( "net_alive_color",
		TCL_GLOBAL_CHAR );

	NET_DEAD_COLOR = MAKE_TCL_GLOBAL( "net_dead_color",
		TCL_GLOBAL_CHAR );

	ST_OUTLINE_COLOR = MAKE_TCL_GLOBAL( "st_outline_color",
		TCL_GLOBAL_CHAR );

	ST_USER_DEFINED_COLOR = MAKE_TCL_GLOBAL( "st_user_defined_color",
		TCL_GLOBAL_CHAR );

	ST_RUNNING_COLOR = MAKE_TCL_GLOBAL( "st_running_color",
		TCL_GLOBAL_CHAR );

	ST_SYSTEM_COLOR = MAKE_TCL_GLOBAL( "st_system_color",
		TCL_GLOBAL_CHAR );

	ST_IDLE_COLOR = MAKE_TCL_GLOBAL( "st_idle_color", TCL_GLOBAL_CHAR );

	ST_COMM_COLOR = MAKE_TCL_GLOBAL( "st_comm_color", TCL_GLOBAL_CHAR );

	UT_RUNNING_COLOR = MAKE_TCL_GLOBAL( "ut_running_color",
		TCL_GLOBAL_CHAR );

	UT_SYSTEM_COLOR = MAKE_TCL_GLOBAL( "ut_system_color",
		TCL_GLOBAL_CHAR );

	UT_IDLE_COLOR = MAKE_TCL_GLOBAL( "ut_idle_color", TCL_GLOBAL_CHAR );

	MQ_MSG_NOSIZE_COLOR = MAKE_TCL_GLOBAL( "mq_msg_nosize_color",
		TCL_GLOBAL_CHAR );

	MQ_MSG_SIZE_COLOR = MAKE_TCL_GLOBAL( "mq_msg_size_color",
		TCL_GLOBAL_CHAR );

	MQ_OUTLINE_COLOR = MAKE_TCL_GLOBAL( "mq_outline_color",
		TCL_GLOBAL_CHAR );

	FG_COLOR = MAKE_TCL_GLOBAL( "fg_color", TCL_GLOBAL_CHAR );

	/* Fonts */

	FIXED_FONT = MAKE_TCL_GLOBAL( "fixed_font", TCL_GLOBAL_CHAR );

	MAIN_FONT = MAKE_TCL_GLOBAL( "main_font", TCL_GLOBAL_CHAR );
}


/* ARGSUSED */
int
initialize_tcl_globals_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	/* Window Names */

	NET = GET_TCL_GLOBAL( interp, "NET" );
	NET_C = GET_TCL_GLOBAL( interp, "NET_C" );
	NET_SBH = GET_TCL_GLOBAL( interp, "NET_SBH" );
	NET_SBV = GET_TCL_GLOBAL( interp, "NET_SBV" );

	MAIN_NET->NET_C = NET_C;

	CTRL = GET_TCL_GLOBAL( interp, "CTRL" );

	ST = GET_TCL_GLOBAL( interp, "ST" );
	ST_L = GET_TCL_GLOBAL( interp, "ST_L" );
	ST_C = GET_TCL_GLOBAL( interp, "ST_C" );
	ST_SBH = GET_TCL_GLOBAL( interp, "ST_SBH" );
	ST_SBV = GET_TCL_GLOBAL( interp, "ST_SBV" );

	SP = GET_TCL_GLOBAL( interp, "SP" );

	KL = GET_TCL_GLOBAL( interp, "KL" );
	KL_C = GET_TCL_GLOBAL( interp, "KL_C" );
	KL_SBH = GET_TCL_GLOBAL( interp, "KL_SBH" );
	KL_SBV = GET_TCL_GLOBAL( interp, "KL_SBV" );

	SG = GET_TCL_GLOBAL( interp, "SG" );
	SG_C = GET_TCL_GLOBAL( interp, "SG_C" );
	SG_SBH = GET_TCL_GLOBAL( interp, "SG_SBH" );
	SG_SBV = GET_TCL_GLOBAL( interp, "SG_SBV" );
	SG_LF = GET_TCL_GLOBAL( interp, "SG_LF" );
	SG_LFC = GET_TCL_GLOBAL( interp, "SG_LFC" );
	SG_LFCW = GET_TCL_GLOBAL( interp, "SG_LFCW" );
	SG_LSBV = GET_TCL_GLOBAL( interp, "SG_LSBV" );

	OV = GET_TCL_GLOBAL( interp, "OV" );

	OF = GET_TCL_GLOBAL( interp, "OF" );

	UT = GET_TCL_GLOBAL( interp, "UT" );
	UT_F = GET_TCL_GLOBAL( interp, "UT_F" );
	UT_C = GET_TCL_GLOBAL( interp, "UT_C" );
	UT_SBH = GET_TCL_GLOBAL( interp, "UT_SBH" );

	MQ = GET_TCL_GLOBAL( interp, "MQ" );
	MQ_F = GET_TCL_GLOBAL( interp, "MQ_F" );
	MQ_C = GET_TCL_GLOBAL( interp, "MQ_C" );
	MQ_SBH = GET_TCL_GLOBAL( interp, "MQ_SBH" );

	CT = GET_TCL_GLOBAL( interp, "CT" );
	CT_F = GET_TCL_GLOBAL( interp, "CT_F" );
	CT_L = GET_TCL_GLOBAL( interp, "CT_L" );
	CT_C = GET_TCL_GLOBAL( interp, "CT_C" );
	CT_SBH = GET_TCL_GLOBAL( interp, "CT_SBH" );
	CT_SBV = GET_TCL_GLOBAL( interp, "CT_SBV" );

	TO = GET_TCL_GLOBAL( interp, "TO" );
	TO_F = GET_TCL_GLOBAL( interp, "TO_F" );
	TO_C = GET_TCL_GLOBAL( interp, "TO_C" );
	TO_SBH = GET_TCL_GLOBAL( interp, "TO_SBH" );
	TO_SBV = GET_TCL_GLOBAL( interp, "TO_SBV" );

	TV = GET_TCL_GLOBAL( interp, "TV" );
	TV_F = GET_TCL_GLOBAL( interp, "TV_F" );
	TV_C = GET_TCL_GLOBAL( interp, "TV_C" );
	TV_SBH = GET_TCL_GLOBAL( interp, "TV_SBH" );
	TV_SBV = GET_TCL_GLOBAL( interp, "TV_SBV" );

	/* Grab Task Output Stuff */

	TASK_OUTPUT->canvas = trc_copy_str( TO_C );

	TASK_OUTPUT->sbv = trc_copy_str( TO_SBV );

	/* Grab Trace Event History Stuff */

	TASK_TEVHIST->canvas = trc_copy_str( TV_C );

	TASK_TEVHIST->sbv = trc_copy_str( TV_SBV );

	/* Done */

	return( TCL_OK );
}

/* ARGSUSED */
int
get_xpvm_default_dir_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	Tcl_SetResult( itp, XPVM_DEFAULT_DIR, TCL_STATIC );

	return( TCL_OK );
}

