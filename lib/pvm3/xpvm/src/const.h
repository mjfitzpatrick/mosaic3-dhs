
/* $Id: const.h,v 4.50 1998/04/09 21:12:20 kohl Exp $ */

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


/* XPVM Version */

#define XPVM_VERSION		"1.2.5"


/* Define Default Location of XPVM Main Directory */

#define XPVM_DEFAULT_DIR	"pvm3/xpvm"


/* Define Default Location of Hostfile */

#define DEFAULT_HOSTFILE	".xpvm_hosts"


/* Define File System Separator Char (for WIN32 Compat) */

#ifndef IMA_WIN32
#define FILE_SEP_CHAR		'/'
#else
#define FILE_SEP_CHAR		'\\'
#endif


/* Hosts Console Command IDs */

#define ADD_ALL_HID			0
#define OTHER_HOST_HID		1
#define MAX_HID				2


/* Playback Controls Command IDs */

#define TRACE_REWIND		0
#define TRACE_STOP			1
#define TRACE_FASTFWD		2
#define TRACE_FWD			3
#define TRACE_FWDSTEP		4
#define MAX_TRACE			5


/* Trace File Status Flag Values */

#define TRACE_FILE_OVERWRITE	0
#define TRACE_FILE_PLAYBACK		1
#define MAX_TRACE_FILE			2


/* Trace Mode Pending Marker Bit Values */

#define TRACE_MODE_NONE			0
#define TRACE_MODE_RESET		1
#define TRACE_MODE_CLEAR_HOSTS	2


/* Text Searching Constants */

#define SEARCH_FORWARD		0
#define SEARCH_BACKWARD		1


/* Lock Constants */

#define UNLOCKED			0
#define LOCKED				1


/* Interface Lock Macros */

#define LOCK_INTERFACE( _save ) \
{ \
	(_save) = INTERFACE_LOCK; \
\
	INTERFACE_LOCK = LOCKED; \
} \

#define UNLOCK_INTERFACE( _save ) \
{ \
	INTERFACE_LOCK = (_save); \
\
	if ( INTERFACE_LOCK == UNLOCKED ) \
		restart_coroutines();\
} \


/* PVM Error Check Macro */

#define NOP    (TMP_CC = TMP_CC)


#define PVMCKERR( _cmd, _str, _handle ) \
{ \
	TMP_CC = _cmd; \
\
	if ( TMP_CC < 0 ) \
	{ \
		pvm_perror( _str ); \
\
		_handle; \
	} \
} \
\


/* TEV Format Constants */

#define TEV_FMT_33			0
#define TEV_FMT_34			1


/* TEV Type Constants */

#define ENTRY_TEV			0
#define EXIT_TEV			1
#define IGNORE_TEV			2
#define MAX_TEV				3


/* Pending Host Status Constant */

#define TO_BE_ADDED			-1


/* Network View Layout Types */

#define NETWORK_BUS			0
#define NETWORK_MESH		1
#define NETWORK_SWITCH		2


/* Network View Host Link ID Constants */

#define MAIN_LINK			0
#define LINK_NORTH			0
#define LINK_EAST			1
#define LINK_SOUTH			2
#define LINK_WEST			3
#define MAX_LINK			4


/* Host Drawn Status Constants */

#define HOST_OFF			0
#define HOST_ON				1
#define HOST_NOTIFY_DEL		2
#define HOST_NOTIFY_ADD		3
#define HOST_DELETED		4
#define HOST_ADDED			5
#define HOST_CORRELATE		6


/* Task Status Constants */

#define TASK_DEAD			0
#define TASK_ALIVE			1
#define TASK_IGNORE			2


/* Task Output Status Constants */

#define TASK_NOOUT			0
#define TASK_OUT			1
#define TASK_EOF			2


/* Task State Constants */

#define STATE_USER_DEFINED	-1
#define STATE_RUNNING		0
#define STATE_SYSTEM		1
#define STATE_IDLE			2
#define STATE_DEAD			3
#define STATE_NONE			4
#define MAX_STATE			5


/* Task Order Constants */

#define TASK_SORT_ALPHA		0
#define TASK_SORT_TID		1
#define TASK_SORT_CUSTOM	2


/* Network Layout Constants */

#define ARRANGE_TOP			0
#define ARRANGE_BOT			1


/* True / False Constants */

#define FALSE				0
#define TRUE				1


/* Number of Unix Signals */

#define NUM_SIGNALS			31


/* Max Number of Coordinates for GOBJs */

#define MAX_GOBJ_COORDS		4


/* Handy Byte References */

#define NBYTES_1K			1024
#define NBYTES_1M			1048576


/* GOBJ Coordinate Identifiers */

#define X_COORD( _gobj )    ( (_gobj)->coords[0] )
#define Y_COORD( _gobj )    ( (_gobj)->coords[1] )


#define X1_COORD( _gobj )    ( (_gobj)->coords[0] )
#define Y1_COORD( _gobj )    ( (_gobj)->coords[1] )
#define X2_COORD( _gobj )    ( (_gobj)->coords[2] )
#define Y2_COORD( _gobj )    ( (_gobj)->coords[3] )


/* TCL Globals Constants & Macros */

#define TCL_GLOBAL_CHAR		0
#define TCL_GLOBAL_INT		1


#define MAKE_TCL_GLOBAL( _name, _type ) \
	( TMP_GLOBAL = create_tcl_global(), \
		TMP_GLOBAL->name = _name, \
		TMP_GLOBAL->type = _type, \
		TMP_GLOBAL )


#define GET_TCL_GLOBAL( _itp, _name ) \
	Tcl_GetVar( _itp, _name, TCL_GLOBAL_ONLY )


#define SET_TCL_GLOBAL( _itp, _name, _str ) \
	Tcl_SetVar( _itp, _name, _str, TCL_GLOBAL_ONLY )


#define SET_INT_TCL_GLOBAL( _itp, _glob, _ival ) \
{ \
	sprintf( TMP_CMD, "%d", _ival ); \
\
	SET_TCL_GLOBAL( _itp, (_glob)->name, TMP_CMD ); \
\
	(_glob)->int_value = _ival; \
} \
\


#define REFRESH_GLOBAL( _glob ) \
{ \
	(_glob)->char_value = GET_TCL_GLOBAL( interp, (_glob)->name ); \
\
	if ( (_glob)->type == TCL_GLOBAL_INT ) \
		(_glob)->int_value = atoi( (_glob)->char_value ); \
} \
\


#define CHAR_GLOBVAL( _glob )    ( (_glob)->char_value )


#define INT_GLOBVAL( _glob )    ( (_glob)->int_value )


/* Timeval Manipulation Macros */

#define TIME_COMPARE( _sec1, _usec1, _sec2, _usec2 ) \
( \
	( (_sec1) > (_sec2) \
			|| ( (_sec1) == (_sec2) && (_usec1) > (_usec2) ) ) ? 1 : \
		( ( (_sec1) == (_sec2) && (_usec1) == (_usec2) ) ? 0 : -1 ) \
) \
\


#define TIME_ELAPSED( _t1, _t2 ) \
( \
	(int) ( ( ( (double) ( (_t2).tv_sec - (_t1).tv_sec ) ) \
			* ( (double) 1000000.0 ) ) \
		+ ( (double) ( (_t2).tv_usec - (_t1).tv_usec ) ) ) \
) \
\


#define TIME_OVERLAP( _t, _t1, _t2 ) \
\
	( TIME_COMPARE( (_t).tv_sec, (_t).tv_usec, \
			(_t1).tv_sec, (_t1).tv_usec ) >= 0 \
		&& TIME_COMPARE( (_t).tv_sec, (_t).tv_usec, \
			(_t2).tv_sec, (_t2).tv_usec ) <= 0 )  \
\


#define TIME_MIN( _t1, _t2, _t ) \
{ \
	if ( TIME_COMPARE( (_t1).tv_sec, (_t1).tv_usec, \
		(_t2).tv_sec, (_t2).tv_usec ) <= 0 ) \
	{ \
		(_t).tv_sec = (_t1).tv_sec; \
		(_t).tv_usec = (_t1).tv_usec; \
	} \
\
	else \
	{ \
		(_t).tv_sec = (_t2).tv_sec; \
		(_t).tv_usec = (_t2).tv_usec; \
	} \
} \
\


#define TIME_MAX( _t1, _t2, _t ) \
{ \
	if ( TIME_COMPARE( (_t1).tv_sec, (_t1).tv_usec, \
		(_t2).tv_sec, (_t2).tv_usec ) >= 0 ) \
	{ \
		(_t).tv_sec = (_t1).tv_sec; \
		(_t).tv_usec = (_t1).tv_usec; \
	} \
\
	else \
	{ \
		(_t).tv_sec = (_t2).tv_sec; \
		(_t).tv_usec = (_t2).tv_usec; \
	} \
} \
\


/* Time to Coordinate Macro */

#define X_OF_TIME( _sec, _usec, _scale ) \
	( ( (int) ( ( ((double) (_sec)) * ((double) 1000000.0) ) \
			/ ((double) (_scale)) ) ) \
		+ ( (int) ( ((double) (_usec)) / ((double) (_scale)) ) ) \
		+ ( INT_GLOBVAL( FRAME_BORDER ) \
			* ( 1 + INT_GLOBVAL( FRAME_OFFSET ) ) ) )

#define CK_X_TIME( _x, _sec, _usec ) \
	if ( (_x) < 0 && (_sec) > 0 && (_usec) > 0 ) \
		printf( "Warning: Time Coordinate Overflow.\n" );


/* Network View Link Volume Coloring Macros */

#define REFRESH_NET_VOLUME_COLORS \
\
	REFRESH_GLOBAL( NET_VOLUME_COLOR_SIZE ); \
\
	for ( TMP_I=0 ; TMP_I < INT_GLOBVAL( NET_VOLUME_COLOR_SIZE ) ; \
		TMP_I++ ) \
	{ \
		REFRESH_GLOBAL( NET_VOLUME_COLOR[ TMP_I ] ); \
	} \
\
	REFRESH_GLOBAL( NET_FG_COLOR ); \
\


#define NET_COLOR_LINK_VOLUME( _itp, _netc, _netlink ) \
{ \
	TMP_COLOR = (char *) NULL; \
\
	for ( TMP_I = ( INT_GLOBVAL( NET_VOLUME_COLOR_SIZE ) - 1 ) ; \
		TMP_I >= 0 && TMP_COLOR == NULL ; TMP_I-- ) \
	{ \
		if ( (_netlink)->msg_bytes >= NET_VOLUME_LEVEL[ TMP_I ] ) \
			TMP_COLOR = CHAR_GLOBVAL( NET_VOLUME_COLOR[ TMP_I ] ); \
	} \
\
	if ( TMP_COLOR == NULL ) \
		TMP_COLOR = CHAR_GLOBVAL( NET_FG_COLOR ); \
\
	sprintf( TMP_CMD, "%s itemconfigure %d -fill %s", \
		_netc, (_netlink)->link->id, TMP_COLOR ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
} \
\


/* Network View Link Bandwidth Macros */

#define NET_BANDWIDTH( _itp, _netc, _netlink, _msg, _bw, _draw ) \
{ \
	if ( TIME_OVERLAP( (_netlink)->bw_start, \
			(_msg)->sendtime, (_msg)->recvtime ) \
		|| TIME_OVERLAP( (_netlink)->bw_end, \
			(_msg)->sendtime, (_msg)->recvtime ) ) \
	{ \
		(_netlink)->msg_bw += (_bw); \
\
		TIME_MIN( (_netlink)->bw_start, (_msg)->sendtime, \
			(_netlink)->bw_start ); \
\
		TIME_MAX( (_netlink)->bw_end, (_msg)->recvtime, \
			(_netlink)->bw_end ); \
	} \
\
	else \
	{ \
		(_netlink)->msg_bw = (_bw); \
\
		(_netlink)->bw_start.tv_sec = (_msg)->sendtime.tv_sec; \
		(_netlink)->bw_start.tv_usec = (_msg)->sendtime.tv_usec; \
\
		(_netlink)->bw_end.tv_sec = (_msg)->recvtime.tv_sec; \
		(_netlink)->bw_end.tv_usec = (_msg)->recvtime.tv_usec; \
	} \
\
	/* Check for Numerical Overflow Problem - Slipped Thru */ \
	(_netlink)->msg_bw = (_netlink)->msg_bw <= 0 ? \
		1 : (_netlink)->msg_bw; \
\
	if ( _draw ) \
	{ \
		DRAW_NET_BANDWIDTH( _itp, _netc, _netlink ); \
	} \
} \
\


#define NET_BANDWIDTH_CORR( _itp, _netc, _netlink, _msg, _bw ) \
{ \
	if ( TIME_OVERLAP( (_netlink)->correlate_bw_start, \
			(_msg)->sendtime, (_msg)->recvtime ) \
		|| TIME_OVERLAP( (_netlink)->correlate_bw_end, \
			(_msg)->sendtime, (_msg)->recvtime ) ) \
	{ \
		(_netlink)->correlate_bw += (_bw); \
\
		TIME_MIN( (_netlink)->correlate_bw_start, (_msg)->sendtime, \
			(_netlink)->correlate_bw_start ); \
\
		TIME_MAX( (_netlink)->correlate_bw_end, (_msg)->recvtime, \
			(_netlink)->correlate_bw_end ); \
	} \
\
	else \
	{ \
		(_netlink)->correlate_bw = (_bw); \
\
		(_netlink)->correlate_bw_start.tv_sec = \
			(_msg)->sendtime.tv_sec; \
		(_netlink)->correlate_bw_start.tv_usec = \
			(_msg)->sendtime.tv_usec; \
\
		(_netlink)->correlate_bw_end.tv_sec = \
			(_msg)->recvtime.tv_sec; \
		(_netlink)->correlate_bw_end.tv_usec = \
			(_msg)->recvtime.tv_usec; \
	} \
\
	/* Check for Numerical Overflow Problem - Slipped Thru */ \
	(_netlink)->correlate_bw = (_netlink)->correlate_bw <= 0 ? \
		1 : (_netlink)->correlate_bw; \
} \
\


#define DRAW_NET_BANDWIDTH( _itp, _netc, _netlink ) \
{ \
	sprintf( TMP_CMD, "%s itemconfigure %d -width %d", \
		_netc, (_netlink)->link->id, \
		2 + (int) log10( (double) (_netlink)->msg_bw ) ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
} \
\


/* Create Canvas Text Object Macro */

#define CREATE_TEXT( _itp, _c, _tobj, _color, _font ) \
{ \
	sprintf( TMP_CMD, \
		"%s create text %d %d -text {%s} -fill {%s} -font {%s} %s", \
		_c, (_tobj)->x, (_tobj)->y, (_tobj)->text, _color, _font, \
		"-anchor nw" ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
\
	(_tobj)->id = atoi( (_itp)->result ); \
} \
\


/* Create Canvas Rectangle Object Macro */

#define CREATE_RECT( _itp, _c, _gobj, _x1, _y1, _x2, _y2, _rc, _oc ) \
{ \
	(_gobj) = create_gobj(); \
\
	X1_COORD( _gobj ) = (_x1); \
	Y1_COORD( _gobj ) = (_y1); \
\
	X2_COORD( _gobj ) = (_x2); \
	Y2_COORD( _gobj ) = (_y2); \
\
	(_gobj)->color = _rc; \
\
	sprintf( TMP_CMD, \
	"%s create rectangle %d %d %d %d -fill \"%s\" -outline \"%s\"", \
		_c, _x1, _y1, _x2, _y2, _rc, _oc ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
\
	(_gobj)->id = atoi( (_itp)->result ); \
} \
\


/* Set Host Box Outline Color & Width */

#define SET_HOST_BOX( _itp, _c, _host, _color, _width ) \
{ \
	sprintf( TMP_CMD, "%s itemconfigure %d -outline %s -width %d", \
		_c, ((HOST_EXT_DRAW) (_host)->ext)->box->id, _color, _width ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
} \
\


/* Create Network View Link Line Macro */

#define CREATE_NET_LINK( _itp, _c, _gobj, _x1, _y1, _x2, _y2, _clr ) \
{\
	(_gobj) = create_gobj(); \
\
	X1_COORD( _gobj ) = (_x1); \
	Y1_COORD( _gobj ) = (_y1); \
\
	X2_COORD( _gobj ) = (_x2); \
	Y2_COORD( _gobj ) = (_y2); \
\
	(_gobj)->color = _clr; \
\
	sprintf( TMP_CMD, \
	"%s create line %d %d %d %d -capstyle round -fill %s -width 1", \
		_c, _x1, _y1, _x2, _y2, _clr ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
\
	(_gobj)->id = atoi( (_itp)->result ); \
}\
\


/* Set Canvas Object Coordinates Macro */

#define SET_COORDS( _itp, _canvas, _gobj ) \
{ \
	sprintf( TMP_CMD, "%s coords %d %d %d %d %d", \
		(_canvas), (_gobj)->id, \
		X1_COORD( _gobj ), Y1_COORD( _gobj ),  \
		X2_COORD( _gobj ), Y2_COORD( _gobj ) ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
} \
\


/* Delete Canvas Text Object Macro */

#define UNDRAW_TEXT( _itp, _canvas, _id ) \
{ \
	sprintf( TMP_CMD, "%s delete %d", (_canvas), (_id) ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
} \
\


/* Delete Canvas Text Object Macro */

#define DELETE_TOBJ( _itp, _canvas, _tobj ) \
{ \
	sprintf( TMP_CMD, "%s delete %d", (_canvas), (_tobj)->id ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
\
	free_tobj( &(_tobj) ); \
} \
\


/* Delete Canvas Graphical Object Macro */

#define DELETE_GOBJ( _itp, _canvas, _gobj ) \
{ \
	sprintf( TMP_CMD, "%s delete %d", (_canvas), (_gobj)->id ); \
\
	Tcl_Eval( _itp, TMP_CMD ); \
\
	free_gobj( &(_gobj) ); \
} \
\

