
static char rcsid[] = 
	"$Id: twin.c,v 4.50 1998/04/09 21:12:03 kohl Exp $";

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


/* Add New Text String to Window */

void
add_twin_text( TW, str )
TWIN TW;
char *str;
{
	TOBJ TT;

	char *dummy_end;
	char *marker;

	int do_draw;
	int do_file;
	int lasty;
	int match;

	REFRESH_GLOBAL( BORDER_SPACE );

	TT = create_tobj();

	TT->x = INT_GLOBVAL( BORDER_SPACE );

	TT->text = trc_copy_str( str );

	TT->selected = FALSE;

	/* Add Text Obj to List */

	if ( TW->last_text == NULL )
		TW->last_text = TW->text = TT;

	else
	{
		TW->last_text->next = TT;

		TT->prev = TW->last_text;

		TW->last_text = TT;
	}

	/* Set Activity Flags */

	REFRESH_GLOBAL( TW->active );

	do_draw = ( !strcmp( CHAR_GLOBVAL( TW->active ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD );

	do_file = ( TW->fp != NULL );

	/* Determine Text Match, Based on Current Filter */

	if ( do_file || do_draw )
	{
		marker = TT->text;

		match = match_text( TT->text, &marker, &dummy_end,
			TW->filter, TW->regexp );
	}

	/* Dump Text to File */

	if ( do_file && match )
	{
		fprintf( TW->fp, "%s\n", TT->text );

		fflush( TW->fp );
	}

	/* Draw Text Obj */

	if ( do_draw )
	{
		if ( match )
		{
			REFRESH_GLOBAL( FIXED_ROW_HEIGHT );
			REFRESH_GLOBAL( FRAME_BORDER );
			REFRESH_GLOBAL( FIXED_FONT );
			REFRESH_GLOBAL( FG_COLOR );

			REFRESH_GLOBAL( TW->lasty );

			TT->y = INT_GLOBVAL( TW->lasty );

			CREATE_TEXT( interp, TW->canvas, TT,
				CHAR_GLOBVAL( FG_COLOR ), CHAR_GLOBVAL( FIXED_FONT ) );

			TT->selected = TRUE;

			lasty = TT->y + INT_GLOBVAL( FIXED_ROW_HEIGHT );

			SET_INT_TCL_GLOBAL( interp, TW->lasty, lasty );

			text_win_auto_scroll( TW );
		}
	}

	else
		TW->needs_redraw = TRUE;
}


/* Update Text Window View */

void
update_twin_text( TW, fileflag )
TWIN TW;
int fileflag;
{
	TOBJ TT;

	char *dummy_end;
	char *marker;

	int do_file;
	int lasty;
	int match;

	/* Update Window */

	REFRESH_GLOBAL( FIXED_ROW_HEIGHT );
	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FIXED_FONT );
	REFRESH_GLOBAL( FG_COLOR );

	do_file = ( fileflag && TW->fp != NULL );

	TT = TW->text;

	lasty = INT_GLOBVAL( FRAME_BORDER );

	while ( TT != NULL )
	{
		/* Determine Match */

		if ( TT->id == -1 || do_file )
		{
			marker = TT->text;

			match = match_text( TT->text, &marker, &dummy_end,
				TW->filter, TW->regexp );
		}

		/* Dump Text Into File */

		if ( do_file && match )
			fprintf( TW->fp, "%s\n", TT->text );

		/* Draw Text on Window */

		if ( TT->id == -1 )
		{
			if ( match )
			{
				TT->y = lasty;

				CREATE_TEXT( interp, TW->canvas, TT,
					CHAR_GLOBVAL( FG_COLOR ),
					CHAR_GLOBVAL( FIXED_FONT ) );

				TT->selected = TRUE;

				lasty += INT_GLOBVAL( FIXED_ROW_HEIGHT );
			}
		}

		else
			lasty = TT->y + INT_GLOBVAL( FIXED_ROW_HEIGHT );

		TT = TT->next;
	}

	/* Flush Text File */

	if ( do_file )
		fflush( TW->fp );

	/* Set Window Scrolling */

	REFRESH_GLOBAL( TW->lasty );

	if ( INT_GLOBVAL( TW->lasty ) != lasty )
	{
		SET_INT_TCL_GLOBAL( interp, TW->lasty, lasty );

		text_win_auto_scroll( TW );
	}

	fix_twin_search( TW );
}


/* Check for Text Window Auto-Scroll */

void
text_win_auto_scroll( TW )
TWIN TW;
{
	char tmp[4096];

	int canvas_end_y;
	int new_yview;
	int height;
	int line;

	REFRESH_GLOBAL( FIXED_ROW_HEIGHT );
	REFRESH_GLOBAL( FRAME_BORDER );

	REFRESH_GLOBAL( TW->cheight );
	REFRESH_GLOBAL( TW->lasty );
	REFRESH_GLOBAL( TW->yview );

	height = ( INT_GLOBVAL( TW->cheight )
			- ( INT_GLOBVAL( FIXED_ROW_HEIGHT ) / 3 ) )
		/ INT_GLOBVAL( FIXED_ROW_HEIGHT );

	canvas_end_y = INT_GLOBVAL( TW->yview ) + height;

	line = ( INT_GLOBVAL( TW->lasty ) - INT_GLOBVAL( FRAME_BORDER ) )
		/ INT_GLOBVAL( FIXED_ROW_HEIGHT );

	if ( line < INT_GLOBVAL( TW->yview )
		|| line >= INT_GLOBVAL( TW->yview ) + height )
	{
		sprintf( tmp, "scrollMaxValue %d %d %d",
			INT_GLOBVAL( TW->lasty ), INT_GLOBVAL( TW->cheight ),
			INT_GLOBVAL( FIXED_ROW_HEIGHT ) );
		
		Tcl_Eval( interp, tmp );

		new_yview = atoi( interp->result );

		new_yview = new_yview < 0 ? 0 : new_yview;

		sprintf( tmp, "do_yview %s %d", TW->canvas, new_yview );

		Tcl_Eval( interp, tmp );

		SET_INT_TCL_GLOBAL( interp, TW->yview, new_yview );
	}

	sprintf( tmp, "scrollSet %s %d %d %d %d", TW->sbv,
		INT_GLOBVAL( TW->lasty ), INT_GLOBVAL( TW->cheight ),
		INT_GLOBVAL( FIXED_ROW_HEIGHT ), INT_GLOBVAL( TW->yview ) );
	
	Tcl_Eval( interp, tmp );
}


/* Search Text Window */

void
search_twin( TW )
TWIN TW;
{
	SEARCH S;

	TOBJ TT;

	char tmp[1024];

	char *match_end;
	char *marker;

	int ycoord;
	int height;
	int start;
	int flag;
	int line;
	int len;

	REFRESH_GLOBAL( FIXED_ROW_HEIGHT );

	REFRESH_GLOBAL( TW->cheight );
	REFRESH_GLOBAL( TW->yview );

	/* Set Up Starting Search Coords */

	S = TW->search;

	if ( S->match_id == -1 || S->changed )
	{
		ycoord = INT_GLOBVAL( TW->yview )
			* INT_GLOBVAL( FIXED_ROW_HEIGHT );

		S->marker = (char *) NULL;
	}

	else
	{
		height = ( INT_GLOBVAL( TW->cheight )
				- ( INT_GLOBVAL( FIXED_ROW_HEIGHT ) / 3 ) )
			/ INT_GLOBVAL( FIXED_ROW_HEIGHT );

		line = S->ycoord / INT_GLOBVAL( FIXED_ROW_HEIGHT );

		if ( S->yview != INT_GLOBVAL( TW->yview )
			&& ( line < INT_GLOBVAL( TW->yview )
				|| line >= INT_GLOBVAL( TW->yview ) + height ) )
		{
			if ( S->direction == SEARCH_FORWARD )
			{
				ycoord = INT_GLOBVAL( TW->yview )
					* INT_GLOBVAL( FIXED_ROW_HEIGHT );
			}

			else
			{
				ycoord = ( INT_GLOBVAL( TW->yview ) + height )
					* INT_GLOBVAL( FIXED_ROW_HEIGHT );
			}

			S->marker = (char *) NULL;

			S->tobj = (TOBJ) NULL;
		}

		else
			ycoord = S->ycoord;
	}

	/* Remove (Any) Previous Match Highlight */

	if ( S->match_id != -1 )
	{
		UNDRAW_TEXT( interp, TW->canvas, S->match_id );

		S->match_id = -1;
	}

	/* Verify Valid Search */

	if ( !strcmp( S->search, "" ) )
	{
		S->marker = (char *) NULL;

		S->ycoord = -1;

		S->yview = INT_GLOBVAL( TW->yview );

		return;
	}

	/* Search */

	if ( S->tobj != NULL )
		TT = S->tobj;
	
	else if ( S->direction == SEARCH_FORWARD )
	{
		TT = TW->text;

		while ( TT != NULL && ( !TT->selected || TT->y < ycoord ) )
			TT = TT->next;
	}

	else
	{
		TT = TW->last_text;

		while ( TT != NULL && ( !TT->selected || TT->y > ycoord ) )
			TT = TT->prev;
	}

	flag = 0;

	while ( TT != NULL && !flag )
	{
		if ( TT->selected )
		{
			if ( S->marker != NULL )
			{
				marker = S->marker;

				S->marker = (char *) NULL;
			}

			else
				marker = TT->text;

			if ( search_text( S, TT->text, &marker, &match_end ) )
			{
				S->tobj = TT;

				flag++;
			}
		}

		TT = S->direction == SEARCH_FORWARD ? TT->next : TT->prev;
	}

	/* Continue Search From Top */

	if ( !flag )
	{
		if ( S->direction == SEARCH_FORWARD )
			TT = TW->text;

		else
			TT = TW->last_text;

		while ( TT != NULL && !flag && ( !TT->selected
			|| ( S->direction == SEARCH_FORWARD && TT->y < ycoord )
			|| ( S->direction == SEARCH_BACKWARD && TT->y > ycoord ) ) )
		{
			if ( TT->selected )
			{
				marker = TT->text;

				if ( search_text( S, TT->text, &marker, &match_end ) )
				{
					S->tobj = TT;

					flag++;
				}
			}

			TT = S->direction == SEARCH_FORWARD ? TT->next : TT->prev;
		}
	}

	if ( flag )
	{
		/* Adjust Task Output Y View */

		set_twin_yview_to_search( TW );

		/* Highlight Matched Pattern */

		REFRESH_GLOBAL( FIXED_COL_WIDTH );
		REFRESH_GLOBAL( BORDER_SPACE );

		start = (int) ( marker - S->tobj->text );

		len = (int) ( match_end - marker );

		S->x1 = INT_GLOBVAL( BORDER_SPACE )
			+ ( start * INT_GLOBVAL( FIXED_COL_WIDTH ) );

		S->x2 = INT_GLOBVAL( BORDER_SPACE )
			+ ( ( start + len ) * INT_GLOBVAL( FIXED_COL_WIDTH ) );

		line = ( S->tobj->y - INT_GLOBVAL( FRAME_BORDER ) )
			/ INT_GLOBVAL( FIXED_ROW_HEIGHT );

		sprintf( tmp, "%s create rectangle %d %d %d %d %s %s %s",
			TW->canvas,
			S->x1, line * INT_GLOBVAL( FIXED_ROW_HEIGHT ),
			S->x2, ( line + 1 ) * INT_GLOBVAL( FIXED_ROW_HEIGHT ),
			"-outline purple", "-fill yellow", "-width 2" );
		
		Tcl_Eval( interp, tmp );

		S->match_id = atoi( interp->result );

		sprintf( tmp, "%s lower %d", TW->canvas, S->match_id );

		Tcl_Eval( interp, tmp );

		/* Save Search State */

		S->ycoord = S->tobj->y;

		marker++;

		if ( *marker != '\0' )
			S->marker = marker;

		else
		{
			if ( S->direction == SEARCH_FORWARD )
			{
				S->marker = (char *) NULL;

				S->tobj = (TOBJ) NULL;

				S->ycoord += INT_GLOBVAL( FIXED_ROW_HEIGHT );
			}

			else
				S->marker = marker;
		}
	}

	else
	{
		Tcl_Eval( interp, "bell" );

		S->marker = (char *) NULL;

		S->ycoord = -1;
	}

	S->yview = INT_GLOBVAL( TW->yview );
}


void
set_twin_yview_to_search( TW )
TWIN TW;
{
	SEARCH S;

	char tmp[1024];

	int difflo, diffhi;
	int new_yview;
	int height;
	int line;

	REFRESH_GLOBAL( FIXED_ROW_HEIGHT );
	REFRESH_GLOBAL( FRAME_BORDER );

	REFRESH_GLOBAL( TW->cheight );
	REFRESH_GLOBAL( TW->yview );

	S = TW->search;

	line = ( S->tobj->y - INT_GLOBVAL( FRAME_BORDER ) )
		/ INT_GLOBVAL( FIXED_ROW_HEIGHT );

	height = ( INT_GLOBVAL( TW->cheight )
			- ( INT_GLOBVAL( FIXED_ROW_HEIGHT ) / 3 ) )
		/ INT_GLOBVAL( FIXED_ROW_HEIGHT );

	if ( line < INT_GLOBVAL( TW->yview )
		|| line >= INT_GLOBVAL( TW->yview ) + height )
	{
		difflo = INT_GLOBVAL( TW->yview ) - line;
		diffhi = line - ( INT_GLOBVAL( TW->yview ) + height ) + 1;

		if ( difflo > 0 && difflo < height / 2 )
			new_yview = line;

		else if ( diffhi > 0 && diffhi < height / 2 )
			new_yview = INT_GLOBVAL( TW->yview ) + diffhi;

		else
			new_yview = line - ( height / 2 );

		new_yview = new_yview < 0 ? 0 : new_yview;

		sprintf( tmp, "do_yview %s %d", TW->canvas, new_yview );

		Tcl_Eval( interp, tmp );

		SET_INT_TCL_GLOBAL( interp, TW->yview, new_yview );

		REFRESH_GLOBAL( TW->lasty );

		sprintf( tmp, "scrollSet %s %d %d %d %d",
			TW->sbv, INT_GLOBVAL( TW->lasty ),
			INT_GLOBVAL( TW->cheight ), INT_GLOBVAL( FIXED_ROW_HEIGHT ),
			new_yview );
	
		Tcl_Eval( interp, tmp );
	}
}


void
fix_twin_search( TW )
TWIN TW;
{
	SEARCH S;

	TOBJ TT;

	char tmp[1024];

	int line;

	REFRESH_GLOBAL( FIXED_ROW_HEIGHT );
	REFRESH_GLOBAL( FRAME_BORDER );

	S = TW->search;

	TT = S->tobj;

	if ( TT != NULL )
	{
		/* Adjust Search Highlight Coords */

		if ( TT->selected )
		{
			line = TT->y - INT_GLOBVAL( FRAME_BORDER );

			sprintf( tmp, "%s coords %d %d %d %d %d",
				TW->canvas, S->match_id,
				S->x1, line,
				S->x2, line + INT_GLOBVAL( FIXED_ROW_HEIGHT ) );
		
			Tcl_Eval( interp, tmp );

			S->ycoord = TT->y;

			set_twin_yview_to_search( TW );
		}

		else
			reset_search( TW );
	}
}


/* Clear Text Window */

void
twin_clear( TW )
TWIN TW;
{
	TOBJ TT;

	TT = TW->text;

	while ( TT != NULL )
	{
		if ( TT->id != -1 )
		{
			UNDRAW_TEXT( interp, TW->canvas, TT->id );

			TT->id = -1;

			TT->selected = FALSE;
		}

		TT = TT->next;
	}
}


int
search_text( S, str, marker, marker_end )
SEARCH S;
char *str;
char **marker;
char **marker_end;
{
	char *last_mark_end;
	char *last_mark;

	char *mark_end;
	char *mark;

	char *target;

	int done;
	int flag;
	int cc;

	if ( S->direction == SEARCH_FORWARD )
	{
		return( match_text( str, marker, marker_end,
			S->search, S->regexp ) );
	}

	else
	{
		last_mark_end = (char *) NULL;
		last_mark = (char *) NULL;

		target = *marker - 1;

		mark = str;

		flag = 0;

		done = 0;

		do
		{
			cc = match_text( str, &mark, &mark_end,
				S->search, S->regexp );

			if ( cc )
			{
				if ( mark != target )
				{
					last_mark_end = mark_end;
					last_mark = mark;

					flag++;
				}

				else
					done++;

				mark++;
			}
		}
		while ( cc && !done );

		*marker_end = last_mark_end;
		*marker = last_mark;

		return( flag );
	}
}


int
match_text( str, marker, match_end, filter, regexp )
char *str;
char **marker;
char **match_end;
char *filter;
Tcl_RegExp regexp;
{
#if ( TCL_MAJOR_VERSION == 7 ) && ( TCL_MINOR_VERSION == 3 )
	char tmp[4096];
#endif

	char *start, *end;

	int cc;

	if ( filter == NULL || *filter == '\0' )
		return( TRUE );

#if ( TCL_MAJOR_VERSION > 7 ) || ( TCL_MINOR_VERSION >= 4 )

	cc = Tcl_RegExpExec( interp, regexp, *marker, str );

	cc = cc < 0 ? 0 : cc;

	if ( cc )
		Tcl_RegExpRange( regexp, 0, &start, &end );

	else
		start = end = (char *) NULL;

#else

	sprintf( tmp, "regexp {%s} {%s}", filter, str );

	Tcl_Eval( interp, tmp );

	cc = atoi( interp->result );

	start = end = (char *) NULL;


#endif

	*marker = start;

	*match_end = end;

	return( cc );
}

