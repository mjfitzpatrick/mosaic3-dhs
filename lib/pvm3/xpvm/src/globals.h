
/* $Id: globals.h,v 4.50 1998/04/09 21:12:29 kohl Exp $ */

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


/* Global Variables */

extern	char		*SIGNAL_STRS[NUM_SIGNALS];

extern	char		*HOST_CMD_STRS[MAX_HID];

extern	char		*HOST_CMD_HELP[MAX_HID];

extern	char		*HOST_MENU_HELP_STR;

extern	char		TMP_CMD[1024];

extern	char		*TMP_COLOR;

extern	char		*TMP_HOST;

extern	struct timeval	CURRENT_TIME;
extern	struct timeval	BASE_TIME;

extern	int			PROCESS_COUNTER_OVERWRITE;
extern	int			PROCESS_COUNTER_PLAYBACK;

extern	int			RECV_COUNTER_OVERWRITE;
extern	int			RECV_COUNTER_PLAYBACK;

extern	int			RECV_TIMER_OVERWRITE;
extern	int			RECV_TIMER_PLAYBACK;

extern	int			PROCESS_COUNTER_INTERVAL;
extern	int			RECV_COUNTER_INTERVAL;

extern	int			RECV_TIMER_INTERVAL;

extern	int			CORRELATE_LOCK;
extern	int			INTERFACE_LOCK;
extern	int			TASK_OUT_LOCK;
extern	int			PROCESS_LOCK;
extern	int			RECV_LOCK;
extern	int			TICK_LOCK;
extern	int			STARTUP;

extern	int			TRACE_OVERWRITE_FLAG;
extern	int			TRACE_PROCESS_LOCK;
extern	int			TRACE_MODE_PENDING;
extern	int			TRACE_FILE_STATUS;
extern	int			TRACE_MSG_ACTIVE;
extern	int			TRACE_DO_SPAWN;
extern	int			TRACE_PENDING;
extern	int			TRACE_ACTIVE;
extern	int			TRACE_STATUS;
extern	int			TRACE_FORMAT;
extern	int			TRACE_BUF;
extern	int			TRACE_OPT;

extern	int			NEEDS_REDRAW_NT;
extern	int			NEEDS_REDRAW_ST;
extern	int			NEEDS_REDRAW_UT;
extern	int			NEEDS_REDRAW_MQ;
extern	int			NEEDS_REDRAW_CT;

extern	int			REGISTERED;

extern	char		*CURRENT_TRACE_MASK;

extern	Pvmtmask	TRACE_MASK;
extern	Pvmtmask	TRACE33_MASK;

extern	Pvmtmask	TRACE_MASK_CLEAR;
extern	Pvmtmask	TRACE33_MASK_CLEAR;

extern	TRC_ID		ID;

extern	int			TEV33_TYPE[TRC_OLD_TRACE_MAX];

extern	int			GROUPS_ALIVE;

extern	Tcl_Interp	*interp;

extern	Tk_Window	Top;

extern	Display		*Disp;

extern	TRACE_MASK_GROUP	*CURRENT_GROUP_LIST;

extern	TRACE_MASK_GROUP	TM33_GROUP_LIST;
extern	TRACE_MASK_GROUP	TM_GROUP_LIST;

extern	TRACE_MASK_INDEX	*CURRENT_INDEX_LIST;

extern	TRACE_MASK_INDEX	TM33_INDEX_LIST;
extern	TRACE_MASK_INDEX	TM_INDEX_LIST;

extern	TWIN		TASK_OUTPUT;

extern	TWIN		TASK_TEVHIST;

extern	MSG			PENDING_MSG_LIST;

extern	MSG_GRID	**PENDING_MSG_GRID;

extern	int			MSG_GRID_SIZE;

extern	MSG			MSG_LIST;

extern	TASK		TASK_LIST;

extern	int			NTASKS;

extern	UT_STATE	UT_LIST;

extern	ARCH		ARCH_LIST;

extern	TRC_HOST	HOST_LIST;

extern	int			NHOSTS;

extern	NETWORK		MAIN_NET;

extern	TRC_HOST	*ADD_HOSTS;
extern	char		**ADD_HOSTS_NAMES;
extern	int			ADD_HOSTS_SIZE;
extern	int			NADDS;

extern	char		*HOME_DIR;

extern	char		*XPVM_DIR;

extern	char		*HOSTFILE;

extern	char		*screen_name;
extern	char		*base_name;

extern	char		**Argv;
extern	int 		Argc;

extern	int			ST_SCROLL_MARK;
extern	int			UT_SCROLL_MARK;

extern	int			TASK_SORT;

extern	int			TIMEIDX;

extern	int			NEW_PVM;

extern	int			TMP_CMP;

extern	int			TMP_CC;

extern	int			TMP_I;

extern	int 		MYTID;

extern	int			Nflag;
extern	int 		eflag;
extern	int 		tflag;
extern	int 		vflag;


/* TCL Globals */

extern	TCL_GLOBAL	TMP_GLOBAL;

extern	TCL_GLOBAL	DEPTH;

extern	TCL_GLOBAL	FIXED_ROW_HEIGHT;

extern	TCL_GLOBAL	FIXED_COL_WIDTH;

extern	TCL_GLOBAL	FRAME_BORDER;

extern	TCL_GLOBAL	FRAME_OFFSET;

extern	TCL_GLOBAL	BORDER_SPACE;

extern	TCL_GLOBAL	ROW_HEIGHT;

extern	TCL_GLOBAL	COL_WIDTH;

extern	TCL_GLOBAL	ICON_BORDER;

extern	TCL_GLOBAL	ICON_SPACE;

extern	TCL_GLOBAL	NET_CHEIGHT;

extern	TCL_GLOBAL	NET_HHEIGHT;

extern	TCL_GLOBAL	NET_CWIDTH;

extern	TCL_GLOBAL	NET_HWIDTH;

extern	TCL_GLOBAL	NET_ACTIVE;

extern	TCL_GLOBAL	NET_SCROLL;

extern	TCL_GLOBAL	NET_FLASH;

extern	TCL_GLOBAL	NET_SPACE;

extern	TCL_GLOBAL	NET_XVIEW;

extern	TCL_GLOBAL	NET_YVIEW;

extern	TCL_GLOBAL	NET_SIZE;

extern	int			UT_MAX_NTASKS;

extern	int			MSG_DEFAULT_NBYTES;

extern	int			MQ_MAX_NBYTES;

extern	TCL_GLOBAL	NET_RUNNING_COLOR;
extern	TCL_GLOBAL	NET_SYSTEM_COLOR;
extern	TCL_GLOBAL	NET_IDLE_COLOR;
extern	TCL_GLOBAL	NET_EMPTY_COLOR;
extern	TCL_GLOBAL	NET_FG_COLOR;

extern	TCL_GLOBAL	NET_ALIVE_COLOR;
extern	TCL_GLOBAL	NET_DEAD_COLOR;

extern	TCL_GLOBAL	NET_VOLUME_COLOR_SIZE;
extern	TCL_GLOBAL	*NET_VOLUME_COLOR;
extern	int			*NET_VOLUME_LEVEL;

extern	TCL_GLOBAL	ST_USER_DEFINED_COLOR;
extern	TCL_GLOBAL	ST_OUTLINE_COLOR;
extern	TCL_GLOBAL	ST_RUNNING_COLOR;
extern	TCL_GLOBAL	ST_SYSTEM_COLOR;
extern	TCL_GLOBAL	ST_IDLE_COLOR;
extern	TCL_GLOBAL	ST_COMM_COLOR;

extern	TCL_GLOBAL	UT_RUNNING_COLOR;
extern	TCL_GLOBAL	UT_SYSTEM_COLOR;
extern	TCL_GLOBAL	UT_IDLE_COLOR;

extern	TCL_GLOBAL	MQ_MSG_NOSIZE_COLOR;
extern	TCL_GLOBAL	MQ_MSG_SIZE_COLOR;
extern	TCL_GLOBAL	MQ_OUTLINE_COLOR;

extern	TCL_GLOBAL	FG_COLOR;

extern	TCL_GLOBAL	FIXED_FONT;
extern	TCL_GLOBAL	MAIN_FONT;

extern	TCL_GLOBAL	ST_CANVAS_HEIGHT;

extern	TCL_GLOBAL	ST_TIMELINE;

extern	TCL_GLOBAL	ST_CHEIGHT;

extern	TCL_GLOBAL	ST_CWIDTH;

extern	TCL_GLOBAL	ST_LABEL_HT;

extern	TCL_GLOBAL	ST_RECT_HT;

extern	TCL_GLOBAL	STL_YVIEW;

extern	TCL_GLOBAL	ST_ACTIVE;

extern	TCL_GLOBAL	ST_TIMEID;

extern	TCL_GLOBAL	ST_XVIEW;

extern	TCL_GLOBAL	TIMEID_PRIM;

extern	TCL_GLOBAL	TIMEX;

extern	TCL_GLOBAL	SCALE;

extern	TCL_GLOBAL	UT_CANVAS_HEIGHT;

extern	TCL_GLOBAL	UT_CHEIGHT;

extern	TCL_GLOBAL	UT_CWIDTH;

extern	TCL_GLOBAL	UT_ACTIVE;

extern	TCL_GLOBAL	UT_TIMEID;

extern	TCL_GLOBAL	UT_XVIEW;

extern	TCL_GLOBAL	MQ_TASK_WT;

extern	TCL_GLOBAL	MQ_CHEIGHT;

extern	TCL_GLOBAL	MQ_ACTIVE;

extern	TCL_GLOBAL	MQ_CWIDTH;

extern	TCL_GLOBAL	MQ_XVIEW;

extern	TCL_GLOBAL	CT_QUERY_INDEX;

extern	TCL_GLOBAL	CT_CHEIGHT;

extern	TCL_GLOBAL	CT_ACTIVE;

extern	TCL_GLOBAL	CT_YVIEW;

extern	TCL_GLOBAL	TO_CHEIGHT;

extern	TCL_GLOBAL	TO_ACTIVE;

extern	TCL_GLOBAL	TO_YVIEW;

extern	TCL_GLOBAL	TO_LASTY;

extern	TCL_GLOBAL	TV_CHEIGHT;

extern	TCL_GLOBAL	TV_ACTIVE;

extern	TCL_GLOBAL	TV_YVIEW;

extern	TCL_GLOBAL	TV_LASTY;

extern	char		*NET;
extern	char		*NET_C;
extern	char		*NET_SBH;
extern	char		*NET_SBV;

extern	char		*CTRL;

extern	char		*ST;
extern	char		*ST_L;
extern	char		*ST_C;
extern	char		*ST_SBH;
extern	char		*ST_SBV;

extern	char		*SP;

extern	char		*KL;
extern	char		*KL_C;
extern	char		*KL_SBH;
extern	char		*KL_SBV;

extern	char		*SG;
extern	char		*SG_C;
extern	char		*SG_SBH;
extern	char		*SG_SBV;
extern	char		*SG_LF;
extern	char		*SG_LFC;
extern	char		*SG_LFCW;
extern	char		*SG_LSBV;

extern	char		*OV;

extern	char		*OF;

extern	char		*UT;
extern	char		*UT_F;
extern	char		*UT_C;
extern	char		*UT_SBH;

extern	char		*MQ;
extern	char		*MQ_F;
extern	char		*MQ_C;
extern	char		*MQ_SBH;

extern	char		*CT;
extern	char		*CT_F;
extern	char		*CT_L;
extern	char		*CT_C;
extern	char		*CT_SBH;
extern	char		*CT_SBV;

extern	char		*TO;
extern	char		*TO_F;
extern	char		*TO_C;
extern	char		*TO_SBH;
extern	char		*TO_SBV;

extern	char		*TV;
extern	char		*TV_F;
extern	char		*TV_C;
extern	char		*TV_SBH;
extern	char		*TV_SBV;

