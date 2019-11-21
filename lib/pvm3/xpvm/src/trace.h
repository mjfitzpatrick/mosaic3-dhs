
/* $Id: trace.h,v 4.50 1998/04/09 21:12:23 kohl Exp $ */

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

/* Trace Handling Constants */

#define HANDLE_FIRST		1
#define HANDLE_NEWTASK		1
#define HANDLE_SPNTASK		2
#define HANDLE_ENDTASK		3
#define HANDLE_HOST_ADD		4
#define HANDLE_HOST_DEL		5
#define HANDLE_HOST_SYNC	6
#define HANDLE_OUTPUT		7
#define HANDLE_EXIT			8
#define HANDLE_SEND			9
#define HANDLE_MCAST		10
#define HANDLE_PSEND		11
#define HANDLE_RECV			12
#define HANDLE_NRECV		13
#define HANDLE_PRECV		14
#define HANDLE_TRECV		15
#define HANDLE_USER_DEFINED	16
#define HANDLE_MAX			16

/* Trace Mask Compatibility Macros */

#define INIT_TRACE_MASK( _mask, _fmt ) \
{ \
	if ( (_fmt) == TEV_FMT_33 ) \
	{ \
		TEV33_MASK_INIT( _mask ); \
	} \
\
	else if ( (_fmt) == TEV_FMT_34 ) \
	{ \
		TEV_MASK_INIT( _mask ); \
	} \
} \
\

#define SET_TRACE_MASK( _mask, _fmt, _index ) \
{ \
	if ( (_fmt) == TEV_FMT_33 ) \
	{ \
		TEV33_MASK_SET( _mask, _index ); \
	} \
\
	else if ( (_fmt) == TEV_FMT_34 ) \
	{ \
		TEV_MASK_SET( _mask, _index ); \
	} \
} \
\

#define UNSET_TRACE_MASK( _mask, _fmt, _index ) \
{ \
	if ( (_fmt) == TEV_FMT_33 ) \
	{ \
		TEV33_MASK_UNSET( _mask, _index ); \
	} \
\
	else if ( (_fmt) == TEV_FMT_34 ) \
	{ \
		TEV_MASK_UNSET( _mask, _index ); \
	} \
} \
\

#define CHECK_TRACE_MASK( _mask, _fmt, _index ) \
( \
	(_fmt) == TEV_FMT_33 ? \
		TEV33_MASK_CHECK( _mask, _index ) : \
		TEV_MASK_CHECK( _mask, _index ) \
) \
\

#define FIRST_EVENT( _fmt ) \
	( (_fmt) == TEV_FMT_33 ? TRC_OLD_TEV_FIRST : TEV_FIRST )

#define LAST_EVENT( _fmt ) \
	( (_fmt) == TEV_FMT_33 ? TRC_OLD_TEV_MAX : TEV_MAX )

#define EVENT_INCR( _fmt )    ( (_fmt) == TEV_FMT_33 ? 2 : 1 )

#define TRACE_MASK_LENGTH( _fmt ) \
	( (_fmt) == TEV_FMT_33 ? TEV33_MASK_LENGTH : TEV_MASK_LENGTH )

