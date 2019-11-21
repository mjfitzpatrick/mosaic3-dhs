
static char rcsid[] = 
	"$Id: ut.c,v 4.50 1998/04/09 21:12:01 kohl Exp $";

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
utHandle( new, ssec, susec, esec, eusec )
int new;
int ssec;
int susec;
int esec;
int eusec;
{
	UT_STATE U;
	UT_STATE Utmp;
	UT_STATE Utmp2;

	UT_STATE last;

	char cmd[1024];

	int HT;

	int cmps, cmpe;
	int ts, te;
	int scale;
	int draw;
	int flag;
	int tmp;
	int tot;
	int i;

	REFRESH_GLOBAL( UT_RUNNING_COLOR );
	REFRESH_GLOBAL( UT_SYSTEM_COLOR );
	REFRESH_GLOBAL( UT_IDLE_COLOR );
	REFRESH_GLOBAL( UT_CHEIGHT );
	REFRESH_GLOBAL( UT_ACTIVE );
	REFRESH_GLOBAL( UT_TIMEID );
	REFRESH_GLOBAL( SCALE );

	/* Verify Correct Timestamping */

	if ( TIME_COMPARE( esec, eusec, ssec, susec ) < 0 )
	{
		printf( "\nError: Timestamps Decreasing... Ignoring.\n\n" );
		fflush( stdout );

		return;
	}

	draw = ( !strcmp( CHAR_GLOBVAL( UT_ACTIVE ), "TRUE" )
		&& TRACE_STATUS != TRACE_FASTFWD );

	if ( !draw )
		NEEDS_REDRAW_UT = TRUE;

	ts = X_OF_TIME( ssec, susec, INT_GLOBVAL( SCALE ) );
	CK_X_TIME( ts, ssec, susec );

	te = X_OF_TIME( esec, eusec, INT_GLOBVAL( SCALE ) );
	CK_X_TIME( te, esec, eusec );

	HT = INT_GLOBVAL( UT_CHEIGHT );

	scale = INT_GLOBVAL( SCALE );

	/* Create First State */

	if ( UT_LIST == NULL )
	{
		U = UT_LIST = create_ut_state();

		for ( i=0 ; i < MAX_STATE ; i++ )
			U->counts[i] = 0;
		
		(U->counts[new])++;

		U->starttime.tv_sec = ssec;
		U->starttime.tv_usec = susec;

		U->endtime.tv_sec = esec;
		U->endtime.tv_usec = eusec;

		if ( draw )
			update_state_rects( U, scale, HT );
	}

	/* Insert State Change */

	else
	{
		/* Check Latest Existing State */

		cmpe = TIME_COMPARE( esec, eusec,
			UT_LIST->endtime.tv_sec, UT_LIST->endtime.tv_usec );

		/* Add Newer State */

		if ( cmpe > 0 )
		{
			if ( draw )
				update_state_rects( UT_LIST, scale, HT );

			Utmp = create_ut_state();

			for ( i=0 ; i < MAX_STATE ; i++ )
				Utmp->counts[i] = 0;

			(Utmp->counts[new])++;

			Utmp->endtime.tv_sec = esec;
			Utmp->endtime.tv_usec = eusec;

			/* Merge with Existing States */

			cmps = TIME_COMPARE( ssec, susec,
				UT_LIST->endtime.tv_sec, UT_LIST->endtime.tv_usec );

			/* Separate State, Add Intermediate Empty State */

			if ( cmps > 0 )
			{
				Utmp->starttime.tv_sec = ssec;
				Utmp->starttime.tv_usec = susec;

				if ( draw )
					update_state_rects( Utmp, scale, HT );

				/* Add Intermediate State */

				Utmp2 = Utmp->next = create_ut_state();

				for ( i=0 ; i < MAX_STATE ; i++ )
					Utmp2->counts[i] = 0;

				Utmp2->starttime.tv_sec = UT_LIST->endtime.tv_sec;
				Utmp2->starttime.tv_usec = UT_LIST->endtime.tv_usec;

				Utmp2->endtime.tv_sec = ssec;
				Utmp2->endtime.tv_usec = susec;

				/* Link Together */

				Utmp2->next = UT_LIST;

				UT_LIST = Utmp;

				return;
			}

			/* Adjacent State */

			else if ( cmps == 0 )
			{
				Utmp->starttime.tv_sec = ssec;
				Utmp->starttime.tv_usec = susec;

				if ( draw )
					update_state_rects( Utmp, scale, HT );

				Utmp->next = UT_LIST;

				UT_LIST = Utmp;

				return;
			}

			/* Overlaps Existing States */

			else
			{
				Utmp->starttime.tv_sec = UT_LIST->endtime.tv_sec;
				Utmp->starttime.tv_usec = UT_LIST->endtime.tv_usec;

				if ( draw )
					update_state_rects( Utmp, scale, HT );

				Utmp->next = U = UT_LIST;

				UT_LIST = Utmp;

				last = Utmp;
			}
		}

		/* Find Nearest Older State */

		else
		{
			U = UT_LIST;

			last = (UT_STATE) NULL;

			flag = 0;

			while ( U != NULL && !flag )
			{
				cmpe = TIME_COMPARE( esec, eusec,
					U->endtime.tv_sec, U->endtime.tv_usec );

				if ( cmpe >= 0 )
					flag++;

				else
				{
					last = U;

					U = U->next;
				}
			}

			/* Ran Off End, New State Earlier than Existing */

			if ( U == NULL )
			{
				/* Check for Overlap with Last State */

				cmpe = TIME_COMPARE( esec, eusec,
					last->starttime.tv_sec,
					last->starttime.tv_usec );

				/* Append New States Off End */

				if ( cmpe <= 0 )
				{
					/* Intermediate Empty State */

					if ( cmpe < 0 )
					{
						Utmp2 = last->next = create_ut_state();

						for ( i=0 ; i < MAX_STATE ; i++ )
							Utmp2->counts[i] = 0;

						Utmp2->starttime.tv_sec = esec;
						Utmp2->starttime.tv_usec = eusec;

						Utmp2->endtime.tv_sec =
							last->starttime.tv_sec;
						Utmp2->endtime.tv_usec =
							last->starttime.tv_usec;

						U = Utmp2;
					}

					else
						U = last;

					/* New State */

					Utmp = U->next = create_ut_state();

					for ( i=0 ; i < MAX_STATE ; i++ )
						Utmp->counts[i] = 0;

					(Utmp->counts[new])++;

					Utmp->starttime.tv_sec = ssec;
					Utmp->starttime.tv_usec = susec;

					Utmp->endtime.tv_sec = esec;
					Utmp->endtime.tv_usec = eusec;

					if ( draw )
						update_state_rects( Utmp, scale, HT );

					return;
				}

				/* Overlap with Last State - Split, But Wait for New */

				else
				{
					/* Adjust End of Old State */

					if ( draw && last->running != NULL )
					{
						X1_COORD( last->running ) = te;

						SET_COORDS( interp, UT_C, last->running );
					}

					if ( draw && last->system != NULL )
					{
						X1_COORD( last->system ) = te;

						SET_COORDS( interp, UT_C, last->system );
					}

					if ( draw && last->idle != NULL )
					{
						X1_COORD( last->idle ) = te;

						SET_COORDS( interp, UT_C, last->idle );
					}

					/* Create New State */

					Utmp = create_ut_state();

					for ( i=0 ; i < MAX_STATE ; i++ )
						Utmp->counts[i] = last->counts[i];

					/* Don't Increment New Yet - Defer */

					Utmp->starttime.tv_sec = last->starttime.tv_sec;
					Utmp->starttime.tv_usec = last->starttime.tv_usec;

					Utmp->endtime.tv_sec = esec;
					Utmp->endtime.tv_usec = eusec;

					if ( draw )
						update_state_rects( Utmp, scale, HT );

					/* Set Original State Start Time */

					last->starttime.tv_sec = esec;
					last->starttime.tv_usec = eusec;

					/* Add New State to List */

					U = last->next = Utmp;
				}
			}

			/* Defer Aligned State for Start Handling Below */
			/* cmpe == 0 */

			/* Split State */

			else if ( cmpe > 0 )
			{
				/* Adjust End of Old State */

				if ( draw && last->running != NULL )
				{
					X1_COORD( last->running ) = te;

					SET_COORDS( interp, UT_C, last->running );
				}

				if ( draw && last->system != NULL )
				{
					X1_COORD( last->system ) = te;

					SET_COORDS( interp, UT_C, last->system );
				}

				if ( draw && last->idle != NULL )
				{
					X1_COORD( last->idle ) = te;

					SET_COORDS( interp, UT_C, last->idle );
				}

				/* Create New State */

				Utmp = create_ut_state();

				for ( i=0 ; i < MAX_STATE ; i++ )
					Utmp->counts[i] = last->counts[i];

				/* Don't Increment New Yet - Defer */

				Utmp->starttime.tv_sec = last->starttime.tv_sec;
				Utmp->starttime.tv_usec = last->starttime.tv_usec;

				Utmp->endtime.tv_sec = esec;
				Utmp->endtime.tv_usec = eusec;

				if ( draw )
					update_state_rects( Utmp, scale, HT );

				/* Set Original State Start Time */

				last->starttime.tv_sec = esec;
				last->starttime.tv_usec = eusec;

				/* Add New State to List */

				Utmp->next = U;

				last->next = Utmp;

				U = Utmp;
			}
		}

		/* Adjust Task Counts Backwards */

		flag = 0;

		while ( U != NULL && !flag )
		{
			cmps = TIME_COMPARE( ssec, susec,
				U->starttime.tv_sec, U->starttime.tv_usec );

			if ( cmps >= 0 )
				flag++;
			
			else
			{
				/* Update Counts */

				(U->counts[new])++;

				if ( draw )
					update_state_rects( U, scale, HT );

				last = U;

				U = U->next;
			}
		}

		if ( U == NULL )
		{
			/* Create New Earliest State */

			Utmp = create_ut_state();

			for ( i=0 ; i < MAX_STATE ; i++ )
				Utmp->counts[i] = 0;

			(Utmp->counts[new])++;

			Utmp->starttime.tv_sec = ssec;
			Utmp->starttime.tv_usec = susec;

			Utmp->endtime.tv_sec = last->starttime.tv_sec;
			Utmp->endtime.tv_usec = last->starttime.tv_usec;

			if ( draw )
				update_state_rects( Utmp, scale, HT );

			last->next = Utmp;

			return;
		}

		/* Fix Beginning of New State */

		/* Aligned State, Simply Update Counts */

		if ( cmps == 0 )
		{
			(U->counts[new])++;

			if ( draw )
				update_state_rects( U, scale, HT );
		}

		/* Check for Start Split (Non-Aligned with State End) */

		else
		{
			cmps = TIME_COMPARE( ssec, susec,
				U->endtime.tv_sec, U->endtime.tv_usec );

			/* Split Into New States */

			if ( cmps < 0 )
			{
				/* Adjust End of Old State */

				if ( draw && U->running != NULL )
				{
					X2_COORD( U->running ) = ts;

					SET_COORDS( interp, UT_C, U->running );
				}

				if ( draw && U->system != NULL )
				{
					X2_COORD( U->system ) = ts;

					SET_COORDS( interp, UT_C, U->system );
				}

				if ( draw && U->idle != NULL )
				{
					X2_COORD( U->idle ) = ts;

					SET_COORDS( interp, UT_C, U->idle );
				}

				/* Create New State */

				Utmp = create_ut_state();

				for ( i=0 ; i < MAX_STATE ; i++ )
					Utmp->counts[i] = U->counts[i];

				(Utmp->counts[new])++;

				Utmp->starttime.tv_sec = ssec;
				Utmp->starttime.tv_usec = susec;

				Utmp->endtime.tv_sec = U->endtime.tv_sec;
				Utmp->endtime.tv_usec = U->endtime.tv_usec;

				if ( draw )
					update_state_rects( Utmp, scale, HT );

				/* Set Original State End Time */

				U->endtime.tv_sec = ssec;
				U->endtime.tv_usec = susec;

				/* Add New State to List */

				Utmp->next = U;

				if ( last != NULL )
					last->next = Utmp;

				else
					UT_LIST = Utmp;
			}
		}
	}

	sprintf( cmd, "%s raise %d", UT_C, INT_GLOBVAL( UT_TIMEID ) );

	Tcl_Eval( interp, cmd );
}


void
update_state_rects( U, scale, ht )
UT_STATE U;
int scale;
int ht;
{
	int ry, sy, iy;
	int x1, x2;

	x1 = X_OF_TIME( U->starttime.tv_sec, U->starttime.tv_usec, scale );

	CK_X_TIME( x1, U->starttime.tv_sec, U->starttime.tv_usec );

	x2 = X_OF_TIME( U->endtime.tv_sec, U->endtime.tv_usec, scale );

	CK_X_TIME( x2, U->endtime.tv_sec, U->endtime.tv_usec );

	/* Verify Rectangles Worth Drawing */

	if ( x1 == x2 )
	{
		if ( U->running != NULL )
		{
			DELETE_GOBJ( interp, UT_C, U->running );
		}

		if ( U->system != NULL )
		{
			DELETE_GOBJ( interp, UT_C, U->system );
		}

		if ( U->idle != NULL )
		{
			DELETE_GOBJ( interp, UT_C, U->idle );
		}

		return;
	}

	/* Get Vertical Coords */

	calcUtilCoords( U->counts, ht, &ry, &sy, &iy );

	/* Check Running Rectangle */

	if ( U->running != NULL )
	{
		if ( ry != ht )
		{
			X1_COORD( U->running ) = x1;
			X2_COORD( U->running ) = x2;

			Y1_COORD( U->running ) = ry;
			Y2_COORD( U->running ) = ht;

			SET_COORDS( interp, UT_C, U->running );
		}

		else
		{
			DELETE_GOBJ( interp, UT_C, U->running );
		}
	}

	else if ( ry != ht )
	{
		CREATE_RECT( interp, UT_C, U->running,
			x1, ry, x2, ht,
			CHAR_GLOBVAL( UT_RUNNING_COLOR ), "" );
	}

	/* Check System Rectangle */

	if ( U->system != NULL )
	{
		if ( sy != ry )
		{
			X1_COORD( U->system ) = x1;
			X2_COORD( U->system ) = x2;

			Y1_COORD( U->system ) = sy;
			Y2_COORD( U->system ) = ry;

			SET_COORDS( interp, UT_C, U->system );
		}

		else
		{
			DELETE_GOBJ( interp, UT_C, U->system );
		}
	}

	else if ( sy != ry )
	{
		CREATE_RECT( interp, UT_C, U->system,
			x1, sy, x2, ry,
			CHAR_GLOBVAL( UT_SYSTEM_COLOR ), "" );
	}

	/* Check Idle Rectangle */

	if ( U->idle != NULL )
	{
		if ( iy != sy )
		{
			X1_COORD( U->idle ) = x1;
			X2_COORD( U->idle ) = x2;

			Y1_COORD( U->idle ) = iy;
			Y2_COORD( U->idle ) = sy;

			SET_COORDS( interp, UT_C, U->idle );
		}

		else
		{
			DELETE_GOBJ( interp, UT_C, U->idle );
		}
	}

	else if ( iy != sy )
	{
		CREATE_RECT( interp, UT_C, U->idle,
			x1, iy, x2, sy,
			CHAR_GLOBVAL( UT_IDLE_COLOR ), "" );
	}
}


void
calcUtilCoords( counts, ht, ry, sy, iy )
int *counts;
int ht;
int *ry;
int *sy;
int *iy;
{
	char cmd[1024];

	int rcnt, scnt, icnt;
	int tot;

	rcnt = counts[STATE_RUNNING];
	scnt = counts[STATE_SYSTEM];
	icnt = counts[STATE_IDLE];

	/* Check Max Ntasks */

	tot = rcnt + scnt + icnt;

	if ( tot > UT_MAX_NTASKS )
	{
		UT_MAX_NTASKS = tot;

		sprintf( cmd, "%s.ntasks configure -text {%d}", UT_F, tot );

		Tcl_Eval( interp, cmd );

		Tcl_Eval( interp, "utAdjustHeight" );
	}

	/* Calculate Coords */

	*iy = ht - ( ht * tot / UT_MAX_NTASKS );

	*sy = ht - ( ht * ( rcnt + scnt ) / UT_MAX_NTASKS );

	*ry = ht - ( ht * rcnt / UT_MAX_NTASKS );
}


int
utIncr( sec, usec )
int sec;
int usec;
{
	UT_STATE U;

	int tx;

	REFRESH_GLOBAL( TIMEX );

	U = UT_LIST;

	if ( U == NULL )
		return( TCL_OK );

	tx = INT_GLOBVAL( TIMEX );

	if ( U->running != NULL )
	{
		X2_COORD( U->running ) = tx;

		SET_COORDS( interp, UT_C, U->running );
	}

	if ( U->system != NULL )
	{
		X2_COORD( U->system ) = tx;

		SET_COORDS( interp, UT_C, U->system );
	}

	if ( U->idle != NULL )
	{
		X2_COORD( U->idle ) = tx;

		SET_COORDS( interp, UT_C, U->idle );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
ut_adjust_height_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	UT_STATE U;

	int ry, sy, iy;
	int ht;

	REFRESH_GLOBAL( UT_CHEIGHT );

	ht = INT_GLOBVAL( UT_CHEIGHT );

	U = UT_LIST;

	while ( U != NULL )
	{
		calcUtilCoords( U->counts, ht, &ry, &sy, &iy );

		if ( U->running != NULL )
		{
			Y1_COORD( U->running ) = ry;
			Y2_COORD( U->running ) = ht;

			SET_COORDS( itp, UT_C, U->running );
		}

		if ( U->system != NULL )
		{
			Y1_COORD( U->system ) = sy;
			Y2_COORD( U->system ) = ry;

			SET_COORDS( itp, UT_C, U->system );
		}

		if ( U->idle != NULL )
		{
			Y1_COORD( U->idle ) = iy;
			Y2_COORD( U->idle ) = sy;

			SET_COORDS( itp, UT_C, U->idle );
		}

		U = U->next;
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
ut_zoom_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	UT_STATE U;

	int x1, x2;
	int scale;
	int ht;

	REFRESH_GLOBAL( UT_CHEIGHT );

	ht = INT_GLOBVAL( UT_CHEIGHT );

	scale = atoi( argv[1] );

	U = UT_LIST;

	while ( U != NULL )
	{
		update_state_rects( U, scale, ht );

		U = U->next;
	}

	return( TCL_OK );
}


void
redraw_utilization()
{
	REFRESH_GLOBAL( UT_ACTIVE );

	if ( !strcmp( CHAR_GLOBVAL( UT_ACTIVE ), "TRUE" ) )
		Tcl_Eval( interp, "utUpdate" );
}


/* ARGSUSED */
int
ut_update_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	UT_STATE U;

	int HT;

	int scale;
	int save;

	if ( NEEDS_REDRAW_UT )
	{
		LOCK_INTERFACE( save );

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Utilization... }" );

		REFRESH_GLOBAL( UT_CHEIGHT );
		REFRESH_GLOBAL( SCALE );

		HT = INT_GLOBVAL( UT_CHEIGHT );

		scale = INT_GLOBVAL( SCALE );

		U = UT_LIST;

		while ( U != NULL )
		{
			update_state_rects( U, scale, HT );

			U = U->next;
		}

		Tcl_Eval( itp,
			"setMsg { Drawing Views... Utilization... Done. }" );

		NEEDS_REDRAW_UT = FALSE;

		UNLOCK_INTERFACE( save );
	}

	return( TCL_OK );
}


/* ARGSUSED */
int
ut_reset_proc( clientData, itp, argc, argv )
ClientData clientData;
Tcl_Interp *itp;
int argc;
char **argv;
{
	UT_STATE U;

	char cmd[1024];

	REFRESH_GLOBAL( FRAME_BORDER );
	REFRESH_GLOBAL( FRAME_OFFSET );

	U = UT_LIST;

	while ( U != NULL )
	{
		if ( U->running != NULL )
			DELETE_GOBJ( itp, UT_C, U->running );

		if ( U->system != NULL )
			DELETE_GOBJ( itp, UT_C, U->system );

		if ( U->idle != NULL )
			DELETE_GOBJ( itp, UT_C, U->idle );

		U = U->next;
	}

	UT_SCROLL_MARK = 0;

	UT_MAX_NTASKS = 1;

	sprintf( cmd, "%s.ntasks configure -text {%d}",
		UT_F, UT_MAX_NTASKS );

	Tcl_Eval( itp, cmd );

	return( TCL_OK );
}

