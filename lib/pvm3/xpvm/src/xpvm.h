
/* $Id: xpvm.h,v 4.50 1998/04/09 21:12:26 kohl Exp $ */

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


/* System Header Files */

#include <stdio.h>
#include <sys/types.h>
#ifndef IMA_WIN32
#include <sys/time.h>
#endif
#include <time.h>
#include <malloc.h>
#include <signal.h>
#ifndef IMA_WIN32
#include <pwd.h>
#endif
#include <math.h>


#ifndef PI
#define PI (M_PI)
#endif


#ifdef IMA_WIN32
/* Structure used in select call, taken from the BSD file sys/time.h. */
struct timeval
{
	long tv_sec;	/* seconds */
	long tv_usec;	/* and microseconds */
};
#endif


/* Header Files */

#include <tk.h>

#if ( TCL_MAJOR_VERSION == 7 ) && ( TCL_MINOR_VERSION == 3 )
typedef void *Tcl_RegExp;
#endif

#include "const.h"

#include <pvm3.h>

#ifdef USE_PVM_33
#include "../src/tdpro.h"
#else
#include <pvmproto.h>
#include <pvmtev.h>
#endif

#include "trcdef.h"

#include "trccompat.h"

#include "trace.h"

#include "compat.pvmtev.h"


/* Program Data Structures */

typedef void (*vfp)();
typedef int (*ifp)();


struct tcl_glob_struct
{
	char *name;
	int type;
	char *char_value;
	int int_value;
};

typedef struct tcl_glob_struct *TCL_GLOBAL;


struct trace_mask_group_struct
{
	char *name;
	char *tmask;
	/* TRACE_MASK_GROUP */ struct trace_mask_group_struct *next;
};

typedef struct trace_mask_group_struct *TRACE_MASK_GROUP;


struct trace_mask_index_struct
{
	char *name;
	int index;
	/* TRACE_MASK_INDEX */ struct trace_mask_index_struct *next;
};

typedef struct trace_mask_index_struct *TRACE_MASK_INDEX;


struct gobj_struct
{
	int id;
	int coords[MAX_GOBJ_COORDS];
	char *color;
};

typedef struct gobj_struct *GOBJ;


struct tobj_struct
{
	int id;
	int x, y;
	char *text;
	int selected;
	/* TOBJ */ struct tobj_struct *prev;
	/* TOBJ */ struct tobj_struct *next;
};

typedef struct tobj_struct *TOBJ;


struct search_struct
{
	int direction;
	int changed;
	char *search;
	Tcl_RegExp regexp;
	TOBJ tobj;
	char *marker;
	int match_id;
	int x1, x2;
	int ycoord;
	int yview;
};

typedef struct search_struct *SEARCH;


struct twin_struct
{
	TOBJ text;
	TOBJ last_text;
	char *filter;
	Tcl_RegExp regexp;
	SEARCH search;
	char *canvas;
	char *sbv;
	char *file;
	FILE *fp;
	TCL_GLOBAL active;
	TCL_GLOBAL cheight;
	TCL_GLOBAL lasty;
	TCL_GLOBAL yview;
	int needs_redraw;
};

typedef struct twin_struct *TWIN;


struct msg_struct
{
	/* TASK */ struct task_struct *ST;
	/* TASK */ struct task_struct *DT;
	int src_tid;
	int dst_tid;
	int msgtag;
	int msgctx;
	int nbytes;
	int assbytes;
	struct timeval sendtime;
	struct timeval recvtime;
	GOBJ line;
	/* MSG */ struct msg_struct *prev;
	/* MSG */ struct msg_struct *next;
};

typedef struct msg_struct *MSG;


struct msgq_struct
{
	MSG msg;
	GOBJ rect;
	/* MSGQ */ struct msgq_struct *next;
};

typedef struct msgq_struct *MSGQ;


struct msg_grid_struct
{
	MSG msg;
	/* MSG_GRID */ struct msg_grid_struct *next;
};

typedef struct msg_grid_struct *MSG_GRID;


struct state_struct
{
	int status;
	char *callstr;
	GOBJ rect;
	int user_defined;
	struct timeval starttime;
	struct timeval endtime;
	/* STATE */ struct state_struct *next;
};

typedef struct state_struct *STATE;


struct task_struct
{
	char *name;
	int tid;
	int ptid;
	int pvmd_tid;
	TRC_HOST host;
	int flags;
	int status;
	struct timeval deadtime;

	/* Space-Time / Call Trace Info */
	int index;
	int y1, y2;
	GOBJ st_label;
	GOBJ ct_label;
	GOBJ ct_callstr;
	STATE states;

	/* Message Queue Info */
	MSGQ msgq;
	GOBJ mqmark;
	int mqcnt;
	int mqbytes;

	/* TASK */ struct task_struct *next;
};

typedef struct task_struct *TASK;


struct ut_state_struct
{
	GOBJ running;
	GOBJ system;
	GOBJ idle;
	int counts[MAX_STATE];
	struct timeval starttime;
	struct timeval endtime;
	/* UT_STATE */ struct ut_state_struct *next;
};

typedef struct ut_state_struct *UT_STATE;


struct arch_struct
{
	char *code;
	char *name;
	int height;
	int width;
	/* ARCH */ struct arch_struct *next;
};

typedef struct arch_struct *ARCH;


struct netlink_struct
{
	GOBJ link;
	int msg_bytes;
	int correlate_bytes;
	int msg_bw;
	int correlate_bw;
	struct timeval bw_start;
	struct timeval bw_end;
	struct timeval correlate_bw_start;
	struct timeval correlate_bw_end;
};

typedef struct netlink_struct *NETLINK;


struct host_ext_struct
{
	int index;
	int status;
	char *menuvar;
	TRC_HOST nethost;
};

typedef struct host_ext_struct *HOST_EXT;


struct host_ext_draw_struct
{
	/* Arch Field */
	ARCH arch;

	/* Network Fields */
	int status;
	int correlate_status;
	int counts[MAX_STATE];
	int correlate_counts[MAX_STATE];
	char *color;
	GOBJ icon;
	GOBJ nme;
	GOBJ box;
	NETLINK links[MAX_LINK];

	/* NETWORK */ struct network_struct *subnet;
};

typedef struct host_ext_draw_struct *HOST_EXT_DRAW;


struct network_struct
{
	int type;
	char *NET_C;
	TRC_HOST host_list;
	TRC_HOST last_host;
	NETLINK *links;
	int nlinks;
};

typedef struct network_struct *NETWORK;


/* Routines */


TCL_GLOBAL		create_tcl_global __ProtoGlarp__(( void ));


TRACE_MASK_GROUP	create_trace_mask_group __ProtoGlarp__(( void ));
TRACE_MASK_GROUP	lookup_trace_mask_group __ProtoGlarp__((
						TRACE_MASK_GROUP, char * ));


TRACE_MASK_INDEX	create_trace_mask_index __ProtoGlarp__(( void ));
TRACE_MASK_INDEX	lookup_trace_mask_name __ProtoGlarp__((
						TRACE_MASK_INDEX, char * ));


GOBJ			create_gobj __ProtoGlarp__(( void ));


TOBJ			create_tobj __ProtoGlarp__(( void ));


SEARCH			create_search __ProtoGlarp__(( void ));


TWIN			create_twin __ProtoGlarp__(( void ));


MSG				create_msg __ProtoGlarp__(( void ));


MSGQ			create_msgq __ProtoGlarp__(( void ));


MSG_GRID		create_msg_grid __ProtoGlarp__(( void ));


STATE			create_state __ProtoGlarp__(( void ));


TASK			create_task __ProtoGlarp__(( void ));

TASK			handle_newtask __ProtoGlarp__(( int, char *,
					int, int, int, int *, int * ));

TASK			get_task_tid __ProtoGlarp__(( int ));


UT_STATE		create_ut_state __ProtoGlarp__(( void ));


ARCH			create_arch __ProtoGlarp__(( void ));
ARCH			get_arch_code __ProtoGlarp__(( char * ));


NETLINK			create_netlink __ProtoGlarp__(( void ));


TRC_HOST		create_network_host __ProtoGlarp__(( char *, char *,
					char *, char *, int, int, int ));

TRC_HOST		get_host_index __ProtoGlarp__(( int ));

TRC_HOST		get_host_tid __ProtoGlarp__(( TRC_HOST, int ));


HOST_EXT		create_host_ext __ProtoGlarp__(( void ));


HOST_EXT_DRAW	create_host_ext_draw __ProtoGlarp__(( void ));


NETWORK			create_network __ProtoGlarp__(( void ));


void			add_all_hosts __ProtoGlarp__(( void ));

void			add_host __ProtoGlarp__(( ClientData ));

void			add_twin_text __ProtoGlarp__(( TWIN, char * ));

void			arrange_network_hosts __ProtoGlarp__(( NETWORK ));

void			calcUtilCoords __ProtoGlarp__(( int *, int,
					int *, int *, int * ));

void			check_add_hosts __ProtoGlarp__(( int ));

void			check_for_host_dead __ProtoGlarp__(( TASK ));

void			check_host_status __ProtoGlarp__(( void ));

void			check_time __ProtoGlarp__(( int, int ));

void			clear_network_hosts __ProtoGlarp__(( void ));

void			color_network_host __ProtoGlarp__(( TRC_HOST, char * ));

void			define_tcl_globals __ProtoGlarp__(( void ));

void			delete_host __ProtoGlarp__(( ClientData ));

void			dequeue_message __ProtoGlarp__(( MSG, int ));

void			destroy_network_host __ProtoGlarp__(( TRC_HOST *,
					int, int ));

void			do_add_host __ProtoGlarp__(( TRC_HOST ));

void			do_delete_host __ProtoGlarp__(( TRC_HOST ));

void			do_trace_overwrite __ProtoGlarp__(( void ));

void			do_trace_playback __ProtoGlarp__(( void ));

void			draw_network_host __ProtoGlarp__(( TRC_HOST ));

void			error_exit __ProtoGlarp__(( void ));

void			extract_network_host __ProtoGlarp__(( TRC_HOST ));

void			fix_twin_search __ProtoGlarp__(( TWIN ));

void			flash_host __ProtoGlarp__(( TRC_HOST, char *, int ));

void			free_gobj __ProtoGlarp__(( GOBJ * ));
void			free_host_ext __ProtoGlarp__(( HOST_EXT * ));
void			free_host_ext_draw __ProtoGlarp__(( HOST_EXT_DRAW * ));
void			free_host_list __ProtoGlarp__(( TRC_HOST * ));
void			free_msg __ProtoGlarp__(( MSG * ));
void			free_msg_grid __ProtoGlarp__(( MSG_GRID * ));
void			free_msgq __ProtoGlarp__(( MSGQ * ));
void			free_netlink __ProtoGlarp__(( NETLINK * ));
void			free_network __ProtoGlarp__(( NETWORK * ));
void			free_search __ProtoGlarp__(( SEARCH * ));
void			free_state __ProtoGlarp__(( STATE * ));
void			free_task __ProtoGlarp__(( TASK * ));
void			free_tcl_global __ProtoGlarp__(( TCL_GLOBAL * ));
void			free_tobj __ProtoGlarp__(( TOBJ * ));
void			free_twin __ProtoGlarp__(( TWIN * ));
void			free_ut_state __ProtoGlarp__(( UT_STATE * ));

void			halt_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

void			handle_event __ProtoGlarp__(( TRC_ID,
					TRC_TEVDESC, TRC_TEVREC ));

void			handle_host_add __ProtoGlarp__(( char *, char *, char *,
					int, int ));

void			handle_host_add_notify __ProtoGlarp__(( TRC_HOST ));

void			handle_host_del __ProtoGlarp__(( char *,
					int, int, int ));

void			handle_host_del_notify __ProtoGlarp__(( TRC_HOST ));

void			handle_host_sync __ProtoGlarp__(( int, int, int ));

void			handle_hostfile_hosts __ProtoGlarp__(( void ));

void			handle_old_event __ProtoGlarp__(( TRC_ID,
					int, int, int, int ));

void			info_network_hosts __ProtoGlarp__(( NETWORK,
					int *, int *, int *, int *, int *, int *, int * ));

void			init_event_trie __ProtoGlarp__(( void ));

void			insert_task __ProtoGlarp__(( TASK ));

void			make_msgq __ProtoGlarp__(( MSG ));

void			mqArrangeTask __ProtoGlarp__(( TASK ));

void			mqTaskAdd __ProtoGlarp__(( TASK ));

void			msg_grid_add __ProtoGlarp__(( MSG ));

void			msg_grid_add_task __ProtoGlarp__(( TASK ));

void			msg_grid_del __ProtoGlarp__(( MSG ));

void			msg_grid_sort_tasks __ProtoGlarp__(( void ));

void			msg_grid_swap_task __ProtoGlarp__(( TASK, int ));

void			net_add_bw __ProtoGlarp__(( MSG ));

void			net_add_message __ProtoGlarp__(( MSG ));

void			net_add_volume __ProtoGlarp__(( MSG ));

void			net_del_message __ProtoGlarp__(( MSG, int ));

void			normalize_time __ProtoGlarp__(( TASK, int *, int * ));

void			place_message __ProtoGlarp__(( MSG ));

void			process_event_proc __ProtoGlarp__(( ClientData ));

void			process_pvm_event __ProtoGlarp__(( int, int, int, int,
					TASK ));

void			process_timer_proc __ProtoGlarp__(( ClientData ));

void			queue_message __ProtoGlarp__(( MSG, int ));

void			quit_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

void			re_register_tracer __ProtoGlarp__(( char * ));

void			read_hostfile __ProtoGlarp__(( void ));

void			recv_event_proc __ProtoGlarp__(( ClientData ));

void			recv_message __ProtoGlarp__(( TASK,
					int, int, int, int, int, int, int ));

void			recv_timer_proc __ProtoGlarp__(( ClientData ));

void			redraw_calltrace __ProtoGlarp__(( void ));
void			redraw_messagequeue __ProtoGlarp__(( void ));
void			redraw_network __ProtoGlarp__(( void ));
void			redraw_spacetime __ProtoGlarp__(( void ));
void			redraw_taskoutput __ProtoGlarp__(( void ));
void			redraw_tevhist __ProtoGlarp__(( void ));
void			redraw_timelines __ProtoGlarp__(( void ));
void			redraw_utilization __ProtoGlarp__(( void ));
void			redraw_views __ProtoGlarp__(( void ));

void			reset_groups __ProtoGlarp__(( void ));

void			reset_search __ProtoGlarp__(( TWIN ));

void			restart_coroutines __ProtoGlarp__(( void ));

void			restore_hosts_from_playback __ProtoGlarp__(( void ));

void			search_twin __ProtoGlarp__(( TWIN ));

void			send_message __ProtoGlarp__(( TASK,
					int, int, int, int, int, int, int ));

void			set_host_box_status __ProtoGlarp__(( TRC_HOST, int ));

void			set_task_tracing __ProtoGlarp__(( int *, int, char * ));

void			set_trace_buffer __ProtoGlarp__(( int, int ));

void			set_trace_controls __ProtoGlarp__(( void ));

void			set_trace_format __ProtoGlarp__(( int, int ));

void			set_trace_mask_buttons __ProtoGlarp__(( Tcl_Interp *,
					char *, TRACE_MASK_GROUP, TRACE_MASK_INDEX, int ));

void			set_trace_mask_index __ProtoGlarp__((
					TRACE_MASK_INDEX *, char *, int ));

void			set_trace_options __ProtoGlarp__(( int, int ));

void			set_twin_yview_to_search __ProtoGlarp__(( TWIN ));

void			set_view_time __ProtoGlarp__(( int, int ));

void			sort_tasks __ProtoGlarp__(( void ));

void			status_msg __ProtoGlarp__(( TRC_ID, char * ));

void			status_msg_safe __ProtoGlarp__(( TRC_ID, char * ));

void			swap_tasks __ProtoGlarp__(( int, int ));

void			sync_time __ProtoGlarp__(( TRC_HOST, int *, int * ));

void			taskSTIncr __ProtoGlarp__(( int, int ));

void			taskState __ProtoGlarp__(( TASK, int, int, int,
					char * ));

void			taskExit __ProtoGlarp__(( TASK, int, int, char * ));

void			task_output __ProtoGlarp__(( int, char * ));

void			task_tevhist __ProtoGlarp__(( int, int, int, char * ));

void			text_win_auto_scroll __ProtoGlarp__(( TWIN ));

void			trace_mask_init __ProtoGlarp__(( char *,
					TRACE_MASK_GROUP *, int ));

void			twin_clear __ProtoGlarp__(( TWIN ));

void			undraw_network_host __ProtoGlarp__(( TRC_HOST, int ));

void			update_message __ProtoGlarp__(( MSG ));

void			update_state_rects __ProtoGlarp__(( UT_STATE,
					int, int ));

void			update_twin_text __ProtoGlarp__(( TWIN, int ));

void			usage __ProtoGlarp__(( void ));

void			utHandle __ProtoGlarp__(( int, int, int, int, int ));

void			wait_kill __ProtoGlarp__(( int ));


char			*task_label_str __ProtoGlarp__(( TASK ));
char			*strip33_name __ProtoGlarp__(( char *, int * ));
char			*host_name __ProtoGlarp__(( TRC_HOST, int ));
char			*trunc_str __ProtoGlarp__(( char *, int ));
char			*upper_str __ProtoGlarp__(( char * ));


int				Tcl_AppInit __ProtoGlarp__(( Tcl_Interp * ));

int				Xpvm_Init __ProtoGlarp__(( Tcl_Interp * ));

int				append_cmd_str __ProtoGlarp__(( char *, char *, int,
					char *, int ));

int				check_file_overwrite_exists __ProtoGlarp__(( char *,
					char * ));

int				check_max_scale_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				check_mode_pending __ProtoGlarp__(( void ));

int				check_trace_overwrite_exists __ProtoGlarp__(( void ));

int				comm_arrange_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				comm_query_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				comm_zoom_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				correlate_views_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				define_arch __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				define_trace_mask_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				do_reset_views __ProtoGlarp__(( Tcl_Interp * ));

int				double_click_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				file_overwrite_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				fix_help_line_cmd __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				get_hosts_menu_list __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				get_initial_trace_file_status_proc __ProtoGlarp__((
					ClientData, Tcl_Interp *, int, char ** ));

int				get_net_volume_level_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				get_plength __ProtoGlarp__(( int, int ));

int				get_tasks_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				get_trace_format_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				get_xpvm_default_dir_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				hosts_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				initialize_tcl_globals_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				interface_lock_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				ipwr __ProtoGlarp__(( int, int ));

int				load_bitmap_file __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				match_text __ProtoGlarp__(( char *,
					char **, char **, char *, Tcl_RegExp ));

int				mq_arrange_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				mq_query_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				mq_reset_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				mq_update_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				netHandle __ProtoGlarp__(( TRC_HOST, int, int ));

int				network_host_find __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				network_reset_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				network_set_menuvar __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				network_update_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				output_file_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				output_filter_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				output_search_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				pad_cmd __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				playback_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				program_init __ProtoGlarp__(( void ));

int				pvm_init __ProtoGlarp__(( void ));

int				read_args __ProtoGlarp__(( void ));

int				reset_pvm_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				reset_trace_file_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				reset_views_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				search_text __ProtoGlarp__(( SEARCH, char *,
					char **, char ** ));

int				set_net_volume_levels_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				set_query_time_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				set_task_sort_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				setup_trace __ProtoGlarp__(( Tcl_Interp * ));

int				signal_by_name __ProtoGlarp__(( char * ));

int				strip_label_cmd __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				taskAdd __ProtoGlarp__(( TASK, int, int, char * ));

int				task_arrange_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_ct_query_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_ct_update_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_label_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_out_update_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_query_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_reset_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_st_update_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_zoom_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				task_tevhist_update_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				tasks_alive __ProtoGlarp__(( void ));

int				tasks_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				tcl_init __ProtoGlarp__(( void ));

int				tevhist_filter_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				tevhist_search_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				time_zoom_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				title_info_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				toggle_search_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				trace_buffer_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				trace_file_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				trace_file_status_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				trace_mask_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				trace_overwrite_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				trc_init __ProtoGlarp__(( void ));

int				update_host __ProtoGlarp__(( TRC_HOST *, TRC_HOST *,
					TRC_HOST ));

int				update_scroll_marks_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				update_time __ProtoGlarp__(( int, int ));

int				utIncr __ProtoGlarp__(( int, int ));

int				ut_adjust_height_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				ut_reset_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				ut_update_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				ut_zoom_proc __ProtoGlarp__(( ClientData,
					Tcl_Interp *, int, char ** ));

int				window_init __ProtoGlarp__(( void ));


/* Externals */

extern	struct Pvmtevinfo	pvmtevinfo[];

extern	struct Pvmtevdid	pvmtevdidlist[];

extern	char	*pvm_errlist[];

extern	int		pvm_nerr;

extern	int 	errno;

