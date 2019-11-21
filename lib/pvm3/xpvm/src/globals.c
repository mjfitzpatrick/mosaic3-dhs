
static char rcsid[] =
	"$Id: globals.c,v 4.50 1998/04/09 21:12:16 kohl Exp $";

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


/* XPVM Header */

#include "xpvm.h"


/* Global Variables */

char	*SIGNAL_STRS[NUM_SIGNALS] =
{
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGABRT",
	"SIGEMT",
	"SIGFPE",
	"SIGKILL",
	"SIGBUS",
	"SIGSEGV",
	"SIGSYS",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM",
	"SIGURG",
	"SIGSTOP",
	"SIGTSTP",
	"SIGCONT",
	"SIGCHLD",
	"SIGTTIN",
	"SIGTTOU",
	"SIGIO",
	"SIGXCPU",
	"SIGXFSZ",
	"SIGVTALRM",
	"SIGPROF",
	"SIGWINCH",
	"SIGLOST",
	"SIGUSR1",
	"SIGUSR2"
};

char	*HOST_CMD_STRS[MAX_HID] =
{
	"Add All Hosts",
	"Other Hosts..."
};

char	*HOST_CMD_HELP[MAX_HID] =
{
"Simultaneously Add All Hosts on the Menu to the PVM Virtual Machine",
"Raise / Lower Dialog to Add Another Host (Not Already on Menu)"
};

char		*HOST_MENU_HELP_STR = "Add / Delete Host";

char		TMP_CMD[1024];

char		*TMP_COLOR;

char		*TMP_HOST;

struct timeval	CURRENT_TIME;
struct timeval	BASE_TIME;

int			PROCESS_COUNTER_OVERWRITE;
int			PROCESS_COUNTER_PLAYBACK;

int			RECV_COUNTER_OVERWRITE;
int			RECV_COUNTER_PLAYBACK;

int			RECV_TIMER_OVERWRITE;
int			RECV_TIMER_PLAYBACK;

int			PROCESS_COUNTER_INTERVAL;
int			RECV_COUNTER_INTERVAL;

int			RECV_TIMER_INTERVAL;

int			CORRELATE_LOCK;
int			INTERFACE_LOCK;
int			TASK_OUT_LOCK;
int			PROCESS_LOCK;
int			RECV_LOCK;
int			TICK_LOCK;
int			STARTUP;

int			TRACE_OVERWRITE_FLAG;
int			TRACE_PROCESS_LOCK;
int			TRACE_MODE_PENDING;
int			TRACE_FILE_STATUS;
int			TRACE_MSG_ACTIVE;
int			TRACE_DO_SPAWN;
int			TRACE_PENDING;
int			TRACE_ACTIVE;
int			TRACE_STATUS;
int			TRACE_FORMAT;
int			TRACE_BUF;
int			TRACE_OPT;

int			NEEDS_REDRAW_NT;
int			NEEDS_REDRAW_ST;
int			NEEDS_REDRAW_UT;
int			NEEDS_REDRAW_MQ;
int			NEEDS_REDRAW_CT;

int			REGISTERED;

char		*CURRENT_TRACE_MASK;

Pvmtmask	TRACE_MASK;
Pvmtmask	TRACE33_MASK;

Pvmtmask	TRACE_MASK_CLEAR;
Pvmtmask	TRACE33_MASK_CLEAR;

TRC_ID		ID;

int			TEV33_TYPE[TRC_OLD_TRACE_MAX];

int			GROUPS_ALIVE;

Tcl_Interp	*interp;

Tk_Window	Top;

Display		*Disp;

TRACE_MASK_GROUP	*CURRENT_GROUP_LIST;

TRACE_MASK_GROUP	TM33_GROUP_LIST;
TRACE_MASK_GROUP	TM_GROUP_LIST;

TRACE_MASK_INDEX	*CURRENT_INDEX_LIST;

TRACE_MASK_INDEX	TM33_INDEX_LIST;
TRACE_MASK_INDEX	TM_INDEX_LIST;

TWIN		TASK_OUTPUT;

TWIN		TASK_TEVHIST;

MSG			PENDING_MSG_LIST;

MSG_GRID	**PENDING_MSG_GRID;

int			MSG_GRID_SIZE;

MSG			MSG_LIST;

TASK		TASK_LIST;

int			NTASKS;

UT_STATE	UT_LIST;

ARCH		ARCH_LIST;

TRC_HOST	HOST_LIST;

int			NHOSTS;

NETWORK		MAIN_NET;

TRC_HOST	*ADD_HOSTS;
char		**ADD_HOSTS_NAMES;
int			ADD_HOSTS_SIZE;
int			NADDS;

char		*HOME_DIR;

char		*XPVM_DIR;

char		*HOSTFILE;

char		*screen_name = (char *) NULL;
char		*base_name = (char *) NULL;

char		**Argv;
int 		Argc;

int			ST_SCROLL_MARK;
int			UT_SCROLL_MARK;

int			TASK_SORT;

int			TIMEIDX;

int			NEW_PVM;

int			TMP_CMP;

int			TMP_CC;

int			TMP_I;

int 		MYTID;

int			Nflag;
int 		eflag;
int 		tflag;
int 		vflag;


/* TCL Globals */

TCL_GLOBAL	TMP_GLOBAL;

TCL_GLOBAL	DEPTH;

TCL_GLOBAL	FIXED_ROW_HEIGHT;

TCL_GLOBAL	FIXED_COL_WIDTH;

TCL_GLOBAL	FRAME_BORDER;

TCL_GLOBAL	FRAME_OFFSET;

TCL_GLOBAL	BORDER_SPACE;

TCL_GLOBAL	ROW_HEIGHT;

TCL_GLOBAL	COL_WIDTH;

TCL_GLOBAL	ICON_BORDER;

TCL_GLOBAL	ICON_SPACE;

TCL_GLOBAL	NET_CHEIGHT;

TCL_GLOBAL	NET_HHEIGHT;

TCL_GLOBAL	NET_CWIDTH;

TCL_GLOBAL	NET_HWIDTH;

TCL_GLOBAL	NET_ACTIVE;

TCL_GLOBAL	NET_SCROLL;

TCL_GLOBAL	NET_FLASH;

TCL_GLOBAL	NET_SPACE;

TCL_GLOBAL	NET_XVIEW;

TCL_GLOBAL	NET_YVIEW;

TCL_GLOBAL	NET_SIZE;

int			UT_MAX_NTASKS;

int			MSG_DEFAULT_NBYTES;

int			MQ_MAX_NBYTES;

TCL_GLOBAL	NET_RUNNING_COLOR;
TCL_GLOBAL	NET_SYSTEM_COLOR;
TCL_GLOBAL	NET_IDLE_COLOR;
TCL_GLOBAL	NET_EMPTY_COLOR;
TCL_GLOBAL	NET_FG_COLOR;

TCL_GLOBAL	NET_ALIVE_COLOR;
TCL_GLOBAL	NET_DEAD_COLOR;

TCL_GLOBAL	NET_VOLUME_COLOR_SIZE;
TCL_GLOBAL	*NET_VOLUME_COLOR;
int			*NET_VOLUME_LEVEL;

TCL_GLOBAL	ST_USER_DEFINED_COLOR;
TCL_GLOBAL	ST_OUTLINE_COLOR;
TCL_GLOBAL	ST_RUNNING_COLOR;
TCL_GLOBAL	ST_SYSTEM_COLOR;
TCL_GLOBAL	ST_IDLE_COLOR;
TCL_GLOBAL	ST_COMM_COLOR;

TCL_GLOBAL	UT_RUNNING_COLOR;
TCL_GLOBAL	UT_SYSTEM_COLOR;
TCL_GLOBAL	UT_IDLE_COLOR;

TCL_GLOBAL	MQ_MSG_NOSIZE_COLOR;
TCL_GLOBAL	MQ_MSG_SIZE_COLOR;
TCL_GLOBAL	MQ_OUTLINE_COLOR;

TCL_GLOBAL	FG_COLOR;

TCL_GLOBAL	FIXED_FONT;
TCL_GLOBAL	MAIN_FONT;

TCL_GLOBAL	ST_CANVAS_HEIGHT;

TCL_GLOBAL	ST_TIMELINE;

TCL_GLOBAL	ST_CHEIGHT;

TCL_GLOBAL	ST_CWIDTH;

TCL_GLOBAL	ST_LABEL_HT;

TCL_GLOBAL	ST_RECT_HT;

TCL_GLOBAL	STL_YVIEW;

TCL_GLOBAL	ST_ACTIVE;

TCL_GLOBAL	ST_TIMEID;

TCL_GLOBAL	ST_XVIEW;

TCL_GLOBAL	TIMEID_PRIM;

TCL_GLOBAL	TIMEX;

TCL_GLOBAL	SCALE;

TCL_GLOBAL	UT_CANVAS_HEIGHT;

TCL_GLOBAL	UT_CHEIGHT;

TCL_GLOBAL	UT_CWIDTH;

TCL_GLOBAL	UT_ACTIVE;

TCL_GLOBAL	UT_TIMEID;

TCL_GLOBAL	UT_XVIEW;

TCL_GLOBAL	MQ_TASK_WT;

TCL_GLOBAL	MQ_CHEIGHT;

TCL_GLOBAL	MQ_ACTIVE;

TCL_GLOBAL	MQ_CWIDTH;

TCL_GLOBAL	MQ_XVIEW;

TCL_GLOBAL	CT_QUERY_INDEX;

TCL_GLOBAL	CT_CHEIGHT;

TCL_GLOBAL	CT_ACTIVE;

TCL_GLOBAL	CT_YVIEW;

TCL_GLOBAL	TO_CHEIGHT;

TCL_GLOBAL	TO_ACTIVE;

TCL_GLOBAL	TO_YVIEW;

TCL_GLOBAL	TO_LASTY;

TCL_GLOBAL	TV_CHEIGHT;

TCL_GLOBAL	TV_ACTIVE;

TCL_GLOBAL	TV_YVIEW;

TCL_GLOBAL	TV_LASTY;

char		*NET;
char		*NET_C;
char		*NET_SBH;
char		*NET_SBV;

char		*CTRL;

char		*ST;
char		*ST_L;
char		*ST_C;
char		*ST_SBH;
char		*ST_SBV;

char		*SP;

char		*KL;
char		*KL_C;
char		*KL_SBH;
char		*KL_SBV;

char		*SG;
char		*SG_C;
char		*SG_SBH;
char		*SG_SBV;
char		*SG_LF;
char		*SG_LFC;
char		*SG_LFCW;
char		*SG_LSBV;

char		*OV;

char		*OF;

char		*UT;
char		*UT_F;
char		*UT_C;
char		*UT_SBH;

char		*MQ;
char		*MQ_F;
char		*MQ_C;
char		*MQ_SBH;

char		*CT;
char		*CT_F;
char		*CT_L;
char		*CT_C;
char		*CT_SBH;
char		*CT_SBV;

char		*TO;
char		*TO_F;
char		*TO_C;
char		*TO_SBH;
char		*TO_SBV;

char		*TV;
char		*TV_F;
char		*TV_C;
char		*TV_SBH;
char		*TV_SBV;

