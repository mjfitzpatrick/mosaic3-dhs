
static char rcsid[] = 
	"$Id: network.c,v 4.50 1998/04/09 21:11:44 kohl Exp $";

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


/*
char *BUTTHEAD[] =
{
	"HOST_OFF",
	"HOST_ON",
	"HOST_NOTIFY_DEL",
	"HOST_NOTIFY_ADD",
	"HOST_DELETED",
	"HOST_ADDED"
};
*/


void
check_host_status()
{
	static int cnt = 0;

	HOST_EXT_DRAW HED;

	TRC_HOST H;

	if ( ++cnt < 5 )
		return;
	
	cnt = 0;

	REFRESH_GLOBAL( NET_ALIVE_COLOR );
	REFRESH_GLOBAL( NET_DEAD_COLOR );

	/* printf( "check_host_status:\n" ); */

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		/*
		if ( HED->status != HOST_OFF )
		{
			printf( "\t%s: status = %s\n", H->name,
				BUTTHEAD[ HE->status ] );
		}
		*/

		switch ( HED->status )
		{
			case HOST_OFF: break;

			case HOST_ON: break;

			case HOST_NOTIFY_ADD:
			case HOST_ADDED:
			{
				flash_host( H, CHAR_GLOBVAL( NET_ALIVE_COLOR ),
					FALSE );

				break;
			}

			case HOST_NOTIFY_DEL:
			case HOST_DELETED:
			{
				flash_host( H, CHAR_GLOBVAL( NET_DEAD_COLOR ),
					FALSE );

				break;
			}
		}

		H = H->next;
	}
}


int
netHandle( H, old, new )
TRC_HOST H;
int old;
int new;
{
	HOST_EXT_DRAW HED;

	char *color;

	REFRESH_GLOBAL( NET_ACTIVE );

	if ( H == NULL )
	{
		printf( "\nError: Null Host to netHandle()\n\n" );

		return( FALSE );
	}

	HED = (HOST_EXT_DRAW) H->ext;

	if ( HED->status == HOST_OFF )
		return( FALSE );

	/* Calculate New Counts */

	(HED->counts[old])--;

	(HED->counts[new])++;

	/* Calculate New Icon Color */

	if ( !strcmp( CHAR_GLOBVAL( NET_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD
		&& CORRELATE_LOCK != LOCKED )
	{
		if ( HED->counts[STATE_RUNNING] > 0 )
		{
			REFRESH_GLOBAL( NET_RUNNING_COLOR );

			color = CHAR_GLOBVAL( NET_RUNNING_COLOR );
		}

		else if ( HED->counts[STATE_SYSTEM] > 0 )
		{
			REFRESH_GLOBAL( NET_SYSTEM_COLOR );

			color = CHAR_GLOBVAL( NET_SYSTEM_COLOR );
		}

		else if ( HED->counts[STATE_IDLE] > 0 )
		{
			REFRESH_GLOBAL( NET_IDLE_COLOR );

			color = CHAR_GLOBVAL( NET_IDLE_COLOR );
		}

		else
		{
			REFRESH_GLOBAL( NET_EMPTY_COLOR );

			color = CHAR_GLOBVAL( NET_EMPTY_COLOR );
		}

		if ( HED->color == NULL || strcmp( HED->color, color ) )
			color_network_host( H, color );

		HED->color = color;
	}

	else
		NEEDS_REDRAW_NT = TRUE;

	return( TRUE );
}


void
redraw_network()
{
	REFRESH_GLOBAL( NET_ACTIVE );

	if ( !strcmp( CHAR_GLOBVAL( NET_ACTIVE ), "TRUE" ) )
		Tcl_Eval( interp, "netUpdate" );
}


/* ARGSUSED */
int
network_update_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	HOST_EXT_DRAW HED;

	NETLINK L;

	TRC_HOST H;

	char *color;

	int save;
	int i;

	if ( NEEDS_REDRAW_NT )
	{
		LOCK_INTERFACE( save );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Network... }" );

		if ( CORRELATE_LOCK == LOCKED )
		{
			UNLOCK_INTERFACE( save );

			return( TCL_OK );
		}

		REFRESH_GLOBAL( NET_RUNNING_COLOR );
		REFRESH_GLOBAL( NET_SYSTEM_COLOR );
		REFRESH_GLOBAL( NET_IDLE_COLOR );
		REFRESH_GLOBAL( NET_EMPTY_COLOR );
		REFRESH_NET_VOLUME_COLORS;

		/* Do Hosts */

		H = MAIN_NET->host_list;

		while ( H != NULL )
		{
			HED = (HOST_EXT_DRAW) H->ext;

			if ( HED->status != HOST_OFF )
			{
				/* Color Host */

				if ( HED->counts[STATE_RUNNING] > 0 )
					color = CHAR_GLOBVAL( NET_RUNNING_COLOR );

				else if ( HED->counts[STATE_SYSTEM] > 0 )
					color = CHAR_GLOBVAL( NET_SYSTEM_COLOR );

				else if ( HED->counts[STATE_IDLE] > 0 )
					color = CHAR_GLOBVAL( NET_IDLE_COLOR );

				else
					color = CHAR_GLOBVAL( NET_EMPTY_COLOR );

				if ( HED->color == NULL || strcmp( HED->color, color ) )
					color_network_host( H, color );

				HED->color = color;

				/* Draw Host Link */

				L = HED->links[MAIN_LINK];

				if ( L != NULL )
				{
					NET_COLOR_LINK_VOLUME( itp, MAIN_NET->NET_C, L );

					DRAW_NET_BANDWIDTH( itp, MAIN_NET->NET_C, L );
				}
			}

			H = H->next;
		}

		/* Draw Network Links */

		for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
		{
			L = MAIN_NET->links[i];

			NET_COLOR_LINK_VOLUME( interp, MAIN_NET->NET_C, L );

			DRAW_NET_BANDWIDTH( itp, MAIN_NET->NET_C, L );
		}

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Network... Done. }" );

		NEEDS_REDRAW_NT = FALSE;

		UNLOCK_INTERFACE( save );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
network_reset_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	HOST_EXT_DRAW HED;

	NETLINK L;

	TRC_HOST H;

	char cmd[1024];

	char *color;

	int i;

	REFRESH_GLOBAL( NET_EMPTY_COLOR );
	REFRESH_GLOBAL( NET_FG_COLOR );

	color = CHAR_GLOBVAL( NET_EMPTY_COLOR );

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		if ( HED->status != HOST_OFF )
			color_network_host( H, color );

		for ( i=0 ; i < MAX_STATE ; i++ )
			HED->counts[i] = 0;

		HED->color = (char *) NULL;

		for ( i=0 ; i < MAX_LINK ; i++ )
		{
			if ( (L = HED->links[i]) != NULL )
			{
				if ( L->link != NULL )
				{
					sprintf( cmd,
						"%s itemconfigure %d -fill %s -width 1",
						MAIN_NET->NET_C, L->link->id,
						CHAR_GLOBVAL( NET_FG_COLOR ) );

					Tcl_Eval( itp, cmd );
				}

				L->msg_bytes = 0;
				L->correlate_bytes = -1;

				L->msg_bw = 0;
				L->correlate_bw = -1;

				L->bw_start.tv_usec = 0;
				L->bw_start.tv_sec = 0;

				L->bw_end.tv_usec = 0;
				L->bw_end.tv_sec = 0;

				L->correlate_bw_start.tv_usec = 0;
				L->correlate_bw_start.tv_sec = 0;

				L->correlate_bw_end.tv_usec = 0;
				L->correlate_bw_end.tv_sec = 0;
			}
		}

		H = H->next;
	}

	for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
	{
		if ( (L = MAIN_NET->links[i]) != NULL )
		{
			if ( L->link != NULL )
			{
				sprintf( cmd, "%s itemconfigure %d -fill %s -width 1",
					MAIN_NET->NET_C, L->link->id,
					CHAR_GLOBVAL( NET_FG_COLOR ) );

				Tcl_Eval( itp, cmd );
			}

			L->msg_bytes = 0;
			L->correlate_bytes = -1;

			L->msg_bw = 0;
			L->correlate_bw = -1;

			L->bw_start.tv_usec = 0;
			L->bw_start.tv_sec = 0;

			L->bw_end.tv_usec = 0;
			L->bw_end.tv_sec = 0;

			L->correlate_bw_start.tv_usec = 0;
			L->correlate_bw_start.tv_sec = 0;

			L->correlate_bw_end.tv_usec = 0;
			L->correlate_bw_end.tv_sec = 0;
		}
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
network_host_find( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	HOST_EXT_DRAW HED;

	NETLINK L;

	TRC_HOST H;

	char value[1024];

	char *susec_pad;
	char *eusec_pad;

	int canvx, canvy;
	int id;
	int i;

	id = atoi( argv[1] );

	canvx = atoi( argv[2] );
	canvy = atoi( argv[3] );

	/* Check for Host Click */

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		if ( HED->status != HOST_OFF )
		{
			if ( canvx >= X1_COORD( HED->box )
				&& canvx <= X2_COORD( HED->box )
				&& canvy >= Y1_COORD( HED->box )
				&& canvy <= Y2_COORD( HED->box ) )
			{
				if ( CORRELATE_LOCK == LOCKED )
				{
					sprintf( value,
						"Host %s (0x%x):  run=%d sys=%d idl=%d",
						H->refname, H->pvmd_tid,
						HED->correlate_counts[STATE_RUNNING],
						HED->correlate_counts[STATE_SYSTEM],
						HED->correlate_counts[STATE_IDLE] );
				}

				else
				{
					sprintf( value,
						"Host %s (0x%x):  run=%d sys=%d idl=%d",
						H->refname, H->pvmd_tid,
						HED->counts[STATE_RUNNING],
						HED->counts[STATE_SYSTEM],
						HED->counts[STATE_IDLE] );
				}

				Tcl_SetResult( itp, trc_copy_str( value ),
					TCL_DYNAMIC );

				return( TCL_OK );
			}
		}

		H = H->next;
	}

	/* Check for Host Link Click */

	H = MAIN_NET->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		if ( HED->status != HOST_OFF )
		{
			L = HED->links[MAIN_LINK];

			if ( L != NULL && L->link->id == id )
			{
				susec_pad = trc_pad_num( L->bw_start.tv_usec, 6 );
				eusec_pad = trc_pad_num( L->bw_end.tv_usec, 6 );

				sprintf( value,
					"Host %s (0x%x) Link:  V=%d, BW=%d (%d.%s - %d.%s)",
					H->alias, H->pvmd_tid,
					L->msg_bytes, L->msg_bw,
					L->bw_start.tv_sec, susec_pad,
					L->bw_end.tv_sec, eusec_pad );

				free( susec_pad );
				free( eusec_pad );

				Tcl_SetResult( itp, trc_copy_str( value ),
					TCL_DYNAMIC );

				return( TCL_OK );
			}
		}

		H = H->next;
	}

	/* Check for Network Link Click */

	for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
	{
		if ( (L = MAIN_NET->links[i]) != NULL && L->link != NULL )
		{
			if ( L->link->id == id )
			{
				susec_pad = trc_pad_num( L->bw_start.tv_usec, 6 );
				eusec_pad = trc_pad_num( L->bw_end.tv_usec, 6 );

				sprintf( value,
					"Network Link:  V=%d, BW=%d (%d.%s - %d.%s)",
					L->msg_bytes, L->msg_bw,
					L->bw_start.tv_sec, susec_pad,
					L->bw_end.tv_sec, eusec_pad );

				free( susec_pad );
				free( eusec_pad );

				Tcl_SetResult( itp, trc_copy_str( value ),
					TCL_DYNAMIC );

				return( TCL_OK );
			}
		}
	}

	/* Nothing Found */

	Tcl_SetResult( itp, "", TCL_STATIC );

	return( TCL_OK );
}


/* ARGSUSED */
int
network_set_menuvar( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	HOST_EXT HE;

	TRC_HOST H;

	char *menuvar;

	int index;

	index = atoi( argv[1] );

	index -= MAX_HID;

	menuvar = trc_copy_str( argv[2] );

	H = HOST_LIST;

	while ( H != NULL )
	{
		HE = (HOST_EXT) H->ext;

		if ( HE->index == index )
		{
			HE->menuvar = menuvar;

			return( TCL_OK );
		}

		H = H->next;
	}

	free( menuvar );

	return( TCL_OK );
}


TRC_HOST
create_network_host( refname, name, alias, arch, tid, speed, status )
char *refname;
char *name;
char *alias;
char *arch;
int tid;
int speed;
int status;
{
	HOST_EXT_DRAW HED;

	TRC_HOST H;

	int i;

	if ( MAIN_NET->host_list != NULL )
		H = MAIN_NET->last_host->next = trc_create_host();

	else
		H = MAIN_NET->host_list = trc_create_host();

	MAIN_NET->last_host = H;

	H->refname = trc_copy_str( refname );

	H->name = trc_copy_str( name );

	H->alias = trc_copy_str( alias );

	H->arch = trc_copy_str( arch );

	H->pvmd_tid = tid;

	H->speed = speed;

	H->in_pvm = TRC_IN_PVM;

	H->ext = (void *) (HED = create_host_ext_draw());

	HED->arch = get_arch_code( arch );

	/* Update Interface */

	for ( i=0 ; i < MAX_STATE ; i++ )
		HED->counts[i] = 0;

	HED->status = status;

	return( H );
}


void
extract_network_host( H )
TRC_HOST H;
{
	HOST_EXT HE;

	HE = (HOST_EXT) H->ext;

	HE->nethost = create_network_host( H->refname, H->name, H->alias,
		H->arch, H->pvmd_tid, H->speed, HE->status );

	/* Set Host Delta, Stupid */

	(HE->nethost)->delta.tv_sec = H->delta.tv_sec;
	(HE->nethost)->delta.tv_usec = H->delta.tv_usec;
}


void
destroy_network_host( Hptr, flash, perm )
TRC_HOST *Hptr;
int flash;
int perm;
{
	HOST_EXT HE;

	TRC_HOST Hdestroy;
	TRC_HOST Hlast;
	TRC_HOST H;

	int found;

	/* Save Host to be Destroyed, Null Out Pointer */

	Hdestroy = *Hptr;

	if ( perm )
		*Hptr = (TRC_HOST) NULL;

	/* Undraw Host */

	undraw_network_host( Hdestroy, flash );

	/* Clear Host File Net Host Pointer... */

	H = HOST_LIST;

	found = 0;

	while ( H != NULL && !found )
	{
		HE = (HOST_EXT) H->ext;

		if ( HE->nethost == Hdestroy )
		{
			HE->nethost = (TRC_HOST) NULL;

			found++;
		}

		H = H->next;
	}

	/* Remove Host From Network List */

	if ( perm )
	{
		H = MAIN_NET->host_list;

		Hlast = (TRC_HOST) NULL;

		while ( H != NULL && H != Hdestroy )
		{
			Hlast = H;

			H = H->next;
		}

		if ( H == Hdestroy )
		{
			if ( Hlast != NULL )
				Hlast->next = H->next;

			else
				MAIN_NET->host_list = H->next;

			if ( H->next == NULL )
				MAIN_NET->last_host = Hlast;

			free_host_ext_draw( (HOST_EXT_DRAW *) &(H->ext) );

			trc_free_host( &Hdestroy );
		}
	}
}


void
draw_network_host( H )
TRC_HOST H;
{
	HOST_EXT_DRAW HED;

	GOBJ G;

	char *net_empty_color;
	char *net_fg_color;

	char cmdopts[1024];
	char cmd[1024];

	int net_size;
	int xy;

	REFRESH_GLOBAL( NET_EMPTY_COLOR );
	REFRESH_GLOBAL( NET_FG_COLOR );
	REFRESH_GLOBAL( MAIN_FONT );
	REFRESH_GLOBAL( NET_SIZE );

	HED = (HOST_EXT_DRAW) H->ext;

	xy = -2 * INT_GLOBVAL( NET_SIZE );

	/* Create Box */

	CREATE_RECT( interp, NET_C, HED->box, xy, xy, xy, xy,
		CHAR_GLOBVAL( NET_EMPTY_COLOR ),
		CHAR_GLOBVAL( NET_FG_COLOR ) );

	/* Create Icon */

	if ( HED->arch == NULL )
		HED->arch = get_arch_code( "DUMMY" );

	HED->icon = create_gobj();

	sprintf( cmdopts,
		"-bitmap %s -anchor center -foreground %s -background %s",
		HED->arch->name, CHAR_GLOBVAL( NET_FG_COLOR ),
		CHAR_GLOBVAL( NET_EMPTY_COLOR ) );

	sprintf( cmd, "%s create bitmap %d %d %s", NET_C, xy, xy, cmdopts );

	Tcl_Eval( interp, cmd );

	HED->icon->id = atoi( interp->result );

	X_COORD( HED->icon ) = Y_COORD( HED->icon ) = xy;

	HED->icon->color = CHAR_GLOBVAL( NET_EMPTY_COLOR );

	/* Create Name */

	HED->nme = create_gobj();

	sprintf( cmd,
		"%s create text %d %d -text {%s} -fill {%s} -font {%s} %s",
		NET_C, xy, xy, trunc_str( H->alias, 8 ),
		CHAR_GLOBVAL( NET_FG_COLOR ), CHAR_GLOBVAL( MAIN_FONT ),
		"-anchor center" );

	Tcl_Eval( interp, cmd );

	HED->nme->id = atoi( interp->result );

	X_COORD( HED->nme ) = Y_COORD( HED->nme ) = xy;

	HED->nme->color = CHAR_GLOBVAL( NET_FG_COLOR );

	/* Create Link */

	HED->links[MAIN_LINK] = create_netlink();
	
	CREATE_NET_LINK( interp, MAIN_NET->NET_C,
		HED->links[MAIN_LINK]->link,
		xy, xy, xy, xy, CHAR_GLOBVAL( NET_FG_COLOR ) );
}


void
undraw_network_host( H, flash )
TRC_HOST H;
int flash;
{
	HOST_EXT_DRAW HED;

	char cmd[1024];

	HED = (HOST_EXT_DRAW) H->ext;

	HED->status = HOST_OFF;

	if ( HED->box != NULL )
	{
		/* Flash Dead Host */

		if ( flash )
		{
			REFRESH_GLOBAL( NET_DEAD_COLOR );

			flash_host( H, CHAR_GLOBVAL( NET_DEAD_COLOR ), TRUE );

			color_network_host( H, CHAR_GLOBVAL( NET_DEAD_COLOR ) );
		}

		/* Remove Dead Host */

		if ( HED->links[MAIN_LINK] != NULL )
		{
			sprintf( cmd, "%s delete %d %d %d %d",
				NET_C, HED->box->id, HED->icon->id, HED->nme->id,
				HED->links[MAIN_LINK]->link->id );
		}

		else
		{
			sprintf( cmd, "%s delete %d %d %d",
				NET_C, HED->box->id, HED->icon->id, HED->nme->id );
		}

		Tcl_Eval( interp, cmd );

		free_gobj( &(HED->box) );
		free_gobj( &(HED->icon) );
		free_gobj( &(HED->nme) );

		if ( HED->links[MAIN_LINK] != NULL )
		{
			free_gobj( &(HED->links[MAIN_LINK]->link) );

			free_netlink( &(HED->links[MAIN_LINK]) );
		}
	}
}


void
flash_host( H, color, bigflash )
TRC_HOST H;
char *color;
int bigflash;
{
	HOST_EXT_DRAW HED;

	char *orig_color;

	int orig_status;
	int num;
	int i;

	HED = (HOST_EXT_DRAW) H->ext;

	if ( bigflash )
	{
		REFRESH_GLOBAL( NET_FLASH );

		num = INT_GLOBVAL( NET_FLASH );
	}

	else
		num = 1;

	if ( HED->color != NULL )
		orig_color = HED->color;
	
	else
	{
		REFRESH_GLOBAL( NET_EMPTY_COLOR );

		orig_color = CHAR_GLOBVAL( NET_EMPTY_COLOR );
	}

	orig_status = HED->status;

	for ( i=0 ; i < num && HED->status == orig_status ; i++ )
	{
		if ( HED->status == orig_status )
		{
			color_network_host( H, color );

			Tcl_Eval( interp, "update" );
		}

		if ( HED->status == orig_status )
		{
			color_network_host( H, orig_color );

			Tcl_Eval( interp, "update" );
		}
	}
}


void
arrange_network_hosts( N )
NETWORK N;
{
	HOST_EXT_DRAW HED;

	NETLINK L;

	TRC_HOST H;

	GOBJ G;

	MSG M;

	char value[1024];
	char cmd[1024];

	int net_hheight, net_hwidth;
	int startwt, endwt;
	int Xtop, Xbot;
	int hi_y, lo_y;
	int iwt, iht;
	int numlinks;
	int totalwt;
	int nactive;
	int x1, x2;
	int y1, y2;
	int ix, iy;
	int first;
	int lastx;
	int nlink;
	int vert;
	int ly;
	int ny;
	int cy;
	int i;

	REFRESH_GLOBAL( NET_FG_COLOR );
	REFRESH_GLOBAL( BORDER_SPACE );
	REFRESH_GLOBAL( ICON_BORDER );
	REFRESH_GLOBAL( ICON_SPACE );
	REFRESH_GLOBAL( ROW_HEIGHT );
	REFRESH_GLOBAL( NET_SPACE );

	/* Get Network Info */

	info_network_hosts( N, &totalwt, &startwt, &endwt, &nactive,
		&hi_y, &lo_y, &numlinks );

	/* Set Network Size Globals */

	hi_y -= INT_GLOBVAL( BORDER_SPACE );
	lo_y += INT_GLOBVAL( BORDER_SPACE );

	net_hheight = lo_y - hi_y;

	sprintf( value, "%d", net_hheight );

	SET_TCL_GLOBAL( interp, "net_hheight", value );

	if ( totalwt > 0 )
		net_hwidth = totalwt + (2 * INT_GLOBVAL( BORDER_SPACE ) );
	
	else
		net_hwidth = 0;

	sprintf( value, "%d", net_hwidth );

	SET_TCL_GLOBAL( interp, "net_hwidth", value );

	/* Initialize Working Vars */

	Xtop = -1 * ( totalwt / 2 );

	Xbot = -1 * ( totalwt / 2 );

	cy = ( (lo_y - hi_y) / 2 ) - lo_y;

	/* (Re)Allocate Network Links Storage */

	if ( N->links != NULL )
	{
		for ( i=0 ; i < N->nlinks ; i++ )
		{
			DELETE_GOBJ( interp, N->NET_C, N->links[i]->link );

			free_netlink( &(N->links[i]) );
		}

		free( N->links );

		N->links = (NETLINK *) NULL;
	}

	if ( numlinks > 0 )
	{
		N->links = (NETLINK *) malloc( (unsigned) numlinks
			* sizeof(NETLINK) );
		trc_memcheck( N->links, "Network Links Array" );
	}

	N->nlinks = numlinks;

	/* Place Individual Hosts */

	vert = ARRANGE_TOP;

	first = TRUE;

	nlink = 0;

	H = N->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		/* If ON, Update Host Image */

		if ( HED->status != HOST_OFF )
		{
			iht = HED->arch->height;
			iwt = HED->arch->width;

			/* Place Icon */

			if ( vert == ARRANGE_TOP )
			{
				ix = Xtop + ( iwt / 2 );

				iy = cy - ( ( iht / 2 ) + INT_GLOBVAL( ROW_HEIGHT )
					+ INT_GLOBVAL( ICON_BORDER )
					+ INT_GLOBVAL( NET_SPACE ) );
			}

			else
			{
				ix = Xbot + ( iwt / 2 );

				iy = cy + ( iht / 2 ) + INT_GLOBVAL( ICON_BORDER )
					+ INT_GLOBVAL( NET_SPACE );
			}

			sprintf( cmd, "%s coords %d %d %d",
				NET_C, HED->icon->id, ix, iy );
			
			Tcl_Eval( interp, cmd );

			/* Place Host Name */

			ny = iy + ( iht / 2 ) + ( INT_GLOBVAL( ROW_HEIGHT ) / 2 );

			sprintf( cmd, "%s coords %d %d %d",
				NET_C, HED->nme->id, ix, ny );
			
			Tcl_Eval( interp, cmd );

			/* Place Bounding Box */

			if ( vert == ARRANGE_TOP )
			{
				x1 = Xtop - INT_GLOBVAL( ICON_BORDER );

				x2 = Xtop + iwt + INT_GLOBVAL( ICON_BORDER );
			}

			else
			{
				x1 = Xbot - INT_GLOBVAL( ICON_BORDER );

				x2 = Xbot + iwt + INT_GLOBVAL( ICON_BORDER );
			}

			y1 = iy - ( iht / 2 ) - INT_GLOBVAL( ICON_BORDER );

			y2 = y1 + iht + INT_GLOBVAL( ROW_HEIGHT )
				+ INT_GLOBVAL( ICON_BORDER );

			X1_COORD( HED->box ) = x1;
			Y1_COORD( HED->box ) = y1;
			X2_COORD( HED->box ) = x2;
			Y2_COORD( HED->box ) = y2;

			SET_COORDS( interp, NET_C, HED->box );

			/* Place Host Link */

			if ( nactive > 1 )
			{
				if ( vert == ARRANGE_TOP )
					ly = y2;

				else
					ly = y1;

				L = HED->links[MAIN_LINK];

				/* Create New Link */

				if ( L == NULL )
				{
					L = HED->links[MAIN_LINK] = create_netlink();
	
					CREATE_NET_LINK( interp, N->NET_C, L->link,
						ix, cy, ix, ly, CHAR_GLOBVAL( NET_FG_COLOR ) );
				}

				/* Reset & Place Old Link */

				else
				{
					sprintf( cmd,
						"%s itemconfigure %d -fill %s -width 1",
						N->NET_C, L->link->id,
						CHAR_GLOBVAL( NET_FG_COLOR ) );

					Tcl_Eval( interp, cmd );

					L->msg_bytes = 0;
					L->correlate_bytes = -1;

					L->msg_bw = 0;
					L->correlate_bw = -1;

					L->bw_start.tv_usec = 0;
					L->bw_start.tv_sec = 0;

					L->bw_end.tv_usec = 0;
					L->bw_end.tv_sec = 0;

					L->correlate_bw_start.tv_usec = 0;
					L->correlate_bw_start.tv_sec = 0;

					L->correlate_bw_end.tv_usec = 0;
					L->correlate_bw_end.tv_sec = 0;

					X1_COORD( L->link ) = ix;
					Y1_COORD( L->link ) = cy;
	
					X2_COORD( L->link ) = ix;
					Y2_COORD( L->link ) = ly;
	
					SET_COORDS( interp, N->NET_C, L->link );
				}
			}

			else if ( HED->links[MAIN_LINK] != NULL )
			{
				DELETE_GOBJ( interp, N->NET_C,
					HED->links[MAIN_LINK]->link );

				free_netlink( &(HED->links[MAIN_LINK]) );
			}

			/* Place Any Network Link */

			if ( first )
			{
				lastx = ix;

				first = FALSE;
			}

			else if ( ix > lastx )
			{
				/* Add New Link */

				N->links[nlink] = create_netlink();
				
				CREATE_NET_LINK( interp, N->NET_C,
					N->links[nlink]->link,
					lastx, cy, ix, cy, CHAR_GLOBVAL( NET_FG_COLOR ) );

				nlink++;

				lastx = ix;
			}

			else if ( ix < lastx )
			{
				/* Split Old Link */

				G = N->links[ nlink - 1 ]->link;

				X2_COORD( G ) = ix;

				SET_COORDS( interp, N->NET_C, G );

				/* Add New Link */

				N->links[nlink] = create_netlink();
				
				CREATE_NET_LINK( interp, N->NET_C,
					N->links[nlink]->link,
					ix, cy, lastx, cy, CHAR_GLOBVAL( NET_FG_COLOR ) );

				nlink++;

				lastx = ix;
			}

			/* Toggle TOP / BOT vert flag */

			if ( vert == ARRANGE_TOP )
			{
				Xtop = Xtop + iwt + INT_GLOBVAL( ICON_SPACE )
					+ ( 2 * INT_GLOBVAL( ICON_BORDER ) );

				vert = ARRANGE_BOT;
			}

			else
			{
				Xbot = Xbot + iwt + INT_GLOBVAL( ICON_SPACE )
					+ ( 2 * INT_GLOBVAL( ICON_BORDER ) );

				vert = ARRANGE_TOP;
			}
		}

		H = H->next;
	}

	/* Add Pending Message Volumes */

	M = PENDING_MSG_LIST;

	while ( M != NULL )
	{
		if ( M->recvtime.tv_sec == -1 && M->recvtime.tv_usec == -1 )
			net_add_message( M );
		
		M = M->next;
	}

	Tcl_Eval( interp, "setNetworkView" );
}


void
info_network_hosts( N, totalwt, startwt, endwt, nactive,
	high_y, low_y, num_links )
NETWORK N;
int *totalwt;
int *startwt;
int *endwt;
int *nactive;
int *high_y;
int *low_y;
int *num_links;
{
	HOST_EXT_DRAW HED;

	TRC_HOST H;

	int startwt_top, startwt_bot;
	int endwt_top, endwt_bot;
	int topwt, botwt;
	int topx, botx;
	int hi_y, lo_y;
	int iwt, iht;
	int numlinks;
	int y1, y2;
	int first;
	int lastx;
	int vert;
	int ix;

	/* Note:  ICON_BORDER, ICON_SPACE, ROW_HEIGHT, NET_SPACE
		are REFRESH_GLOBAL()-ed in arrange_network_hosts()
		before calling us. */

	/* Initialize Working Vars */

	startwt_top = -1;
	startwt_bot = -1;

	endwt_top = -1;
	endwt_bot = -1;

	vert = ARRANGE_TOP;

	*nactive = 0;

	numlinks = 0;

	first = TRUE;

	topwt = 0;
	botwt = 0;

	topx = 0;
	botx = 0;

	hi_y = 0;
	lo_y = 0;

	/* Process Host List */

	H = N->host_list;

	while ( H != NULL )
	{
		HED = (HOST_EXT_DRAW) H->ext;

		if ( HED->status != HOST_OFF )
		{
			iht = HED->arch->height;
			iwt = HED->arch->width;

			if ( vert == ARRANGE_TOP )
			{
				topwt += iwt + INT_GLOBVAL( ICON_SPACE )
					+ ( 2 * INT_GLOBVAL( ICON_BORDER ) );

				if ( startwt_top == -1 )
					startwt_top = iwt;

				endwt_top = iwt;

				topx += iwt / 2;

				ix = topx;

				y1 = -1 * ( iht + INT_GLOBVAL( NET_SPACE )
					+ INT_GLOBVAL( ROW_HEIGHT )
					+ ( 2 * INT_GLOBVAL( ICON_BORDER ) ) );

				if ( y1 < hi_y )
					hi_y = y1;

				vert = ARRANGE_BOT;
			}

			else
			{
				botwt += iwt + INT_GLOBVAL( ICON_SPACE )
					+ ( 2 * INT_GLOBVAL( ICON_BORDER ) );

				if ( startwt_bot == -1 )
					startwt_bot = iwt;

				endwt_bot = iwt;

				botx += iwt / 2;

				ix = botx;

				y2 = iht + INT_GLOBVAL( NET_SPACE )
					+ INT_GLOBVAL( ROW_HEIGHT )
					+ ( 2 * INT_GLOBVAL( ICON_BORDER ) );

				if ( y2 > lo_y )
					lo_y = y2;

				vert = ARRANGE_TOP;
			}

			(*nactive)++;

			/* Count Network Links */

			if ( first )
			{
				lastx = ix;

				first = FALSE;
			}

			else if ( lastx != ix )
			{
				numlinks++;

				lastx = ix;
			}
		}

		H = H->next;
	}

	topwt -= INT_GLOBVAL( ICON_SPACE )
		+ ( 2 * INT_GLOBVAL( ICON_BORDER ) );

	botwt -= INT_GLOBVAL( ICON_SPACE )
		+ ( 2 * INT_GLOBVAL( ICON_BORDER ) );

	/* Determine Largest Total Width */

	if ( topwt > botwt )
		*totalwt = topwt;

	else
		*totalwt = botwt;

	/* Determine Smallest Starting Host Width */

	if ( startwt_bot != -1 )
	{
		if ( startwt_top < startwt_bot )
			*startwt = startwt_top;

		else
			*startwt = startwt_bot;
	}

	else
		*startwt = startwt_top;

	/* Determine Ending Host Width */

	topwt -= ( endwt_top / 2 );

	if ( startwt_bot != -1 )
	{
		botwt -= ( endwt_bot / 2 );

		if ( topwt > botwt )
			*endwt = endwt_top;

		else
			*endwt = endwt_bot;
	}

	else
		*endwt = endwt_top;

	/* Save High & Low Y */

	*high_y = hi_y;
	*low_y = lo_y;

	/* Save Number of Network Links */

	*num_links = numlinks;
}


void
color_network_host( H, color )
TRC_HOST H;
char *color;
{
	HOST_EXT_DRAW HED;

	char cmd[1024];

	REFRESH_GLOBAL( DEPTH );

	HED = (HOST_EXT_DRAW) H->ext;

	sprintf( cmd, "%s itemconfigure %d -background %s",
		NET_C, HED->icon->id, color );
	Tcl_Eval( interp, cmd );

	if ( INT_GLOBVAL( DEPTH ) > 1 )
	{
		sprintf( cmd, "%s itemconfigure %d -fill %s",
			NET_C, HED->box->id, color );
		Tcl_Eval( interp, cmd );
	}

	else
	{
		if ( !strcmp( color, "black" ) )
		{
			sprintf( cmd, "%s itemconfigure %d -foreground white",
				NET_C, HED->icon->id );
			Tcl_Eval( interp, cmd );
		}

		else
		{
			sprintf( cmd, "%s itemconfigure %d -foreground black",
				NET_C, HED->icon->id );
			Tcl_Eval( interp, cmd );
		}
	}
}


/* Network Message Routines */

void
net_add_message( M )
MSG M;
{
	TRC_HOST SH, DH;

	NETLINK LS, LD;
	NETLINK L;

	char cmd[1024];

	int nbytes;
	int xs, xd;
	int x1, x2;
	int draw;
	int i;

	REFRESH_NET_VOLUME_COLORS;

	if ( M->ST == NULL || M->DT == NULL )
		return;

	/* Get Source & Dest Host */

	SH = M->ST->host;
	DH = M->DT->host;

	if ( SH == NULL || SH->ext == NULL
		|| DH == NULL || DH->ext == NULL )
	{
		return;
	}

	/* Add Message to Network Volume */

	draw = ( !strcmp( CHAR_GLOBVAL( NET_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD && CORRELATE_LOCK != LOCKED );

	if ( SH != DH )
	{
		LS = ((HOST_EXT_DRAW) SH->ext)->links[MAIN_LINK];
		LD = ((HOST_EXT_DRAW) DH->ext)->links[MAIN_LINK];

		if ( LS == NULL || LD == NULL )
			return;

		xs = X2_COORD( LS->link );
		xd = X2_COORD( LD->link );

		x1 = xs < xd ? xs : xd;
		x2 = xs > xd ? xs : xd;

		nbytes = M->assbytes;

		/* Src Host Link */

		if ( CORRELATE_LOCK == LOCKED )
			LS->correlate_bytes += nbytes;
		
		else
			LS->msg_bytes += nbytes;

		if ( draw )
			NET_COLOR_LINK_VOLUME( interp, MAIN_NET->NET_C, LS );

		/* Dest Host Link */

		if ( CORRELATE_LOCK == LOCKED )
			LD->correlate_bytes += nbytes;
		
		else
			LD->msg_bytes += nbytes;

		if ( draw )
			NET_COLOR_LINK_VOLUME( interp, MAIN_NET->NET_C, LD );

		/* Network Links */

		for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
		{
			L = MAIN_NET->links[i];

			if ( x1 <= X1_COORD( L->link )
				&& X2_COORD( L->link ) <= x2 )
			{
				if ( CORRELATE_LOCK == LOCKED )
					L->correlate_bytes += nbytes;
		
				else
					L->msg_bytes += nbytes;

				if ( draw )
					NET_COLOR_LINK_VOLUME( interp, MAIN_NET->NET_C, L );
			}
		}
	}
}


void
net_del_message( M, queued )
MSG M;
int queued;
{
	TRC_HOST SH, DH;

	NETLINK LS, LD;
	NETLINK L;

	char cmd[1024];

	int elapsed;
	int nbytes;
	int xs, xd;
	int x1, x2;
	int draw;
	int bw;
	int i;

	REFRESH_NET_VOLUME_COLORS;

	if ( M->ST == NULL || M->DT == NULL )
		return;

	/* Get Source & Dest Host */

	SH = M->ST->host;
	DH = M->DT->host;

	if ( SH == NULL || SH->ext == NULL
		|| DH == NULL || DH->ext == NULL )
	{
		return;
	}

	/* Delete Message from Network Volume & Do Bandwidth */

	draw = ( !strcmp( CHAR_GLOBVAL( NET_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD && CORRELATE_LOCK != LOCKED );

	if ( SH != DH )
	{
		LS = ((HOST_EXT_DRAW) SH->ext)->links[MAIN_LINK];
		LD = ((HOST_EXT_DRAW) DH->ext)->links[MAIN_LINK];

		if ( LS == NULL || LD == NULL )
			return;

		xs = X2_COORD( LS->link );
		xd = X2_COORD( LD->link );

		x1 = xs < xd ? xs : xd;
		x2 = xs > xd ? xs : xd;

		/* Src & Dest Link Volumes */

		if ( queued )
		{
			nbytes = M->assbytes;

			if ( CORRELATE_LOCK == LOCKED )
				LS->correlate_bytes -= nbytes;
		
			else
				LS->msg_bytes -= nbytes;

			if ( draw )
				NET_COLOR_LINK_VOLUME( interp, MAIN_NET->NET_C, LS );

			if ( CORRELATE_LOCK == LOCKED )
				LD->correlate_bytes -= nbytes;
		
			else
				LD->msg_bytes -= nbytes;

			if ( draw )
				NET_COLOR_LINK_VOLUME( interp, MAIN_NET->NET_C, LD );
		}

		/* Src & Dest Link Bandwidth */

		elapsed = TIME_ELAPSED( M->sendtime, M->recvtime );

		if ( elapsed > 0 )
		{
			bw = (int) ( ( 1000000.0 / (double) elapsed )
				* (double) M->nbytes );

			if ( CORRELATE_LOCK == LOCKED )
			{
				NET_BANDWIDTH_CORR( interp,
					MAIN_NET->NET_C, LS, M, bw );

				NET_BANDWIDTH_CORR( interp,
					MAIN_NET->NET_C, LD, M, bw );
			}

			else
			{
				NET_BANDWIDTH( interp,
					MAIN_NET->NET_C, LS, M, bw, draw );

				NET_BANDWIDTH( interp,
					MAIN_NET->NET_C, LD, M, bw, draw );
			}
		}

		/* Network Links */

		for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
		{
			L = MAIN_NET->links[i];

			if ( x1 <= X1_COORD( L->link )
				&& X2_COORD( L->link ) <= x2 )
			{
				if ( queued )
				{
					if ( CORRELATE_LOCK == LOCKED )
						L->correlate_bytes -= nbytes;
		
					else
						L->msg_bytes -= nbytes;

					if ( draw )
					{
						NET_COLOR_LINK_VOLUME( interp,
							MAIN_NET->NET_C, L );
					}
				}

				if ( elapsed > 0 )
				{
					if ( CORRELATE_LOCK == LOCKED )
					{
						NET_BANDWIDTH_CORR( interp,
							MAIN_NET->NET_C, L, M, bw );
					}

					else
					{
						NET_BANDWIDTH( interp,
							MAIN_NET->NET_C, L, M, bw, draw );
					}
				}
			}
		}
	}
}


void
net_add_volume( M )
MSG M;
{
	TRC_HOST SH, DH;

	NETLINK LS, LD;
	NETLINK L;

	char cmd[1024];

	int nbytes;
	int xs, xd;
	int x1, x2;
	int draw;
	int i;

	if ( M->ST == NULL || M->DT == NULL )
		return;

	/* Get Source & Dest Host */

	SH = M->ST->host;
	DH = M->DT->host;

	if ( SH == NULL || SH->ext == NULL
		|| DH == NULL || DH->ext == NULL )
	{
		return;
	}

	/* Add Message to Network Volume */

	if ( SH != DH )
	{
		LS = ((HOST_EXT_DRAW) SH->ext)->links[MAIN_LINK];
		LD = ((HOST_EXT_DRAW) DH->ext)->links[MAIN_LINK];

		if ( LS == NULL || LD == NULL )
			return;

		xs = X2_COORD( LS->link );
		xd = X2_COORD( LD->link );

		x1 = xs < xd ? xs : xd;
		x2 = xs > xd ? xs : xd;

		nbytes = M->assbytes;

		LS->msg_bytes += nbytes;

		LD->msg_bytes += nbytes;

		/* Network Links */

		for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
		{
			L = MAIN_NET->links[i];

			if ( x1 <= X1_COORD( L->link )
				&& X2_COORD( L->link ) <= x2 )
			{
				L->msg_bytes += nbytes;
			}
		}
	}
}


void
net_add_bw( M )
MSG M;
{
	TRC_HOST SH, DH;

	NETLINK LS, LD;
	NETLINK L;

	char cmd[1024];

	int elapsed;
	int xs, xd;
	int x1, x2;
	int bw;
	int i;

	if ( M->ST == NULL || M->DT == NULL )
		return;

	/* Get Source & Dest Host */

	SH = M->ST->host;
	DH = M->DT->host;

	if ( SH == NULL || SH->ext == NULL
		|| DH == NULL || DH->ext == NULL )
	{
		return;
	}

	/* Delete Message from Network Volume & Do Bandwidth */

	if ( SH != DH )
	{
		LS = ((HOST_EXT_DRAW) SH->ext)->links[MAIN_LINK];
		LD = ((HOST_EXT_DRAW) DH->ext)->links[MAIN_LINK];

		if ( LS == NULL || LD == NULL )
			return;

		xs = X2_COORD( LS->link );
		xd = X2_COORD( LD->link );

		x1 = xs < xd ? xs : xd;
		x2 = xs > xd ? xs : xd;

		/* Src & Dest Link Bandwidth */

		elapsed = TIME_ELAPSED( M->sendtime, M->recvtime );

		if ( elapsed > 0 )
		{
			bw = (int) ( ( 1000000.0 / (double) elapsed )
				* (double) M->nbytes );

			NET_BANDWIDTH( interp, MAIN_NET->NET_C, LS, M, bw, TRUE );

			NET_BANDWIDTH( interp, MAIN_NET->NET_C, LD, M, bw, TRUE );

			/* Network Links */

			for ( i=0 ; i < MAIN_NET->nlinks ; i++ )
			{
				L = MAIN_NET->links[i];

				if ( x1 <= X1_COORD( L->link )
					&& X2_COORD( L->link ) <= x2 )
				{
					NET_BANDWIDTH( interp, MAIN_NET->NET_C, L, M, bw,
						TRUE );
				}
			}
		}
	}
}

