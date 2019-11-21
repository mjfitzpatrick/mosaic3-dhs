#
# $Id: procs.tcl,v 4.50 1998/04/09 21:12:41 kohl Exp $
#

#
#         XPVM version 1.1:  A Console and Monitor for PVM
#           Oak Ridge National Laboratory, Oak Ridge TN.
#           Authors:  James Arthur Kohl and G. A. Geist
#                   (C) 1994 All Rights Reserved
#
#                              NOTICE
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby granted
# provided that the above copyright notice appear in all copies and
# that both the copyright notice and this permission notice appear
# in supporting documentation.
#
# Neither the Institution, Oak Ridge National Laboratory, nor the
# Authors make any representations about the suitability of this
# software for any purpose.  This software is provided ``as is''
# without express or implied warranty.
#
# XPVM was funded by the U.S. Department of Energy.
#


puts -nonewline "\[procs.tcl\]"
flush stdout


#
# XPVM TCL Procs
#

proc canvas_handle { view action x y } \
{
	#debug_proc canvas_handle entry

	global st_help_label_query
	global st_canvas_height
	global ut_canvas_height
	global st_highlight_id
	global st_labels_width
	global correlate_help
	global correlate_keep
	global correlate_lock
	global st_query_keep
	global st_help_enter
	global ut_help_enter
	global st_help_query
	global st_help_label
	global qc_info_mode
	global st_help_zoom
	global ut_help_zoom
	global border_space
	global frame_border
	global st_label_ht
	global scale_stack
	global timeid_prim
	global main_width
	global st_cheight
	global st_cwidth
	global stl_yview
	global ut_cwidth
	global zoom_lock
	global fg_color
	global zoom_x1
	global zoom_x2
	global zoomid
	global scale

	global FRAME_OFFSET

	global ST
	global ST_L
	global ST_C

	global UT_C

	set STQ .xpvm.st_label_query

	if { $view == "ST" } \
	{
		global st_xview
		global stc_yview

		set canvx [ expr $x + $st_xview ]
		set canvy [ expr $y + $stc_yview ]
		set lcanvy [ expr $y + ( $stl_yview * $st_label_ht ) ]

		set PRIM_C $ST_C

		set ALT_C $UT_C

		set zoom_height $st_canvas_height

		set timeid_prim "ST"

		set prim_cwidth $st_cwidth

		set prim_help_enter $st_help_enter
		set prim_help_zoom $st_help_zoom
	} \
\
	elseif { $view == "UT" } \
	{
		global ut_xview

		set canvx [ expr $x + $ut_xview ]
		set canvy $y

		set PRIM_C $UT_C

		set ALT_C $ST_C

		set zoom_height $ut_canvas_height

		set timeid_prim "UT"

		set prim_cwidth $ut_cwidth

		set prim_help_enter $ut_help_enter
		set prim_help_zoom $ut_help_zoom
	}

	#
	# Calculate Canvas Time
	#

	set canvt [ expr $canvx \
		- ( $frame_border * ( 1 + $FRAME_OFFSET ) ) ]

	#
	# Process Action
	#

	if { $action == "query_press" } \
	{
		if { $qc_info_mode == "query" } \
		{
			if { $view == "UT" } \
				{ return }

			timeid_proc destroy -1 -1 -1

			if { $st_query_keep == "FALSE" } \
				{ set_query_time -1 }

			set id [ $ST_C find closest $canvx $canvy ]

			set tindex1 [ expr ( $canvy - ($st_label_ht / 2) \
					- $border_space ) \
				/ $st_label_ht ]

			set tindex2 [ expr ( $canvy + ($st_label_ht / 2) \
					- $border_space ) \
				/ $st_label_ht ]

			st_query_info $id $tindex1 $tindex2 1

			setTmpMsg "Help" $st_help_query
		} \
\
		elseif { $qc_info_mode == "correlate" } \
		{
			if { $correlate_keep == "TRUE" } \
				{ timeid_proc move $canvx 2 orange }

			set correlate_lock TRUE

			set dblclk [ double_click ]

			if { $dblclk == "1" } \
				{ set correlate_keep TRUE } \
\
			else \
				{ set correlate_keep FALSE }

			timeid_proc modify -1 2 orange

			correlate_views $canvt

			setTmpMsg "Help" $correlate_help
		}
	} \
\
	elseif { $action == "query_motion" } \
	{
		if { $qc_info_mode == "query" } \
		{
			if { $view == "UT" } \
			{
				timeid_proc move $canvx 1 blue

				if { $st_query_keep == "FALSE" } \
					{ set_query_time $canvt }

				return
			}

			if { $x >= 0 && $x < $st_cwidth \
				&& $y >= 0 && $y < $st_cheight } \
			{
				set id [ $ST_C find closest $canvx $canvy ]

				set tindex1 [ expr ( $canvy - ($st_label_ht / 2) \
					- $border_space ) / $st_label_ht ]

				set tindex2 [ expr ( $canvy + ($st_label_ht / 2) \
					- $border_space ) / $st_label_ht ]

				st_query_info $id $tindex1 $tindex2 0
			} \
\
			else \
			{
				st_query_info -1 -1 -1 0
			}
		} \
\
		elseif { $qc_info_mode == "correlate" } \
		{
			timeid_proc move $canvx 2 orange

			correlate_views $canvt
		}
	} \
\
	elseif { $action == "query_release" } \
	{
		if { $qc_info_mode == "query" } \
		{
			if { $view == "UT" } \
				{ return }

			st_query_info -1 -1 -1 0

			if { $st_highlight_id == -1 } \
				{ set st_query_keep FALSE }

			timeid_proc create $canvx 1 blue

			if { $st_query_keep == "FALSE" } \
				{ set_query_time $canvt }

			setTmpMsg "Help" $prim_help_enter
		} \
\
		elseif { $qc_info_mode == "correlate" } \
		{
			if { $correlate_keep == "FALSE" } \
			{
				timeid_proc modify -1 1 blue

				correlate_views -1

				set_query_time $canvt

				set correlate_lock FALSE

				setTmpMsg "Help" $prim_help_enter
			}
		}
	} \
\
	elseif { $action == "enter" } \
	{
		if { $correlate_lock == "TRUE" || $correlate_keep == "TRUE" } \
			{ return }

		timeid_proc create $canvx 1 blue

		if { $st_query_keep == "FALSE" } \
			{ set_query_time $canvt }

		setTmpMsg "Help" $prim_help_enter
	} \
\
	elseif { $action == "time" } \
	{
		if { $correlate_keep == "TRUE" } \
			{ return }

		timeid_proc move $canvx 1 blue

		if { $st_query_keep == "FALSE" } \
			{ set_query_time $canvt }

		if { $prim_help_enter != [ tmpMsg ] } \
			{ setTmpMsg "Help" $prim_help_enter }
	} \
\
	elseif { $action == "leave" } \
	{
		if { $correlate_lock == "TRUE" || $correlate_keep == "TRUE" } \
			{ return }

		timeid_proc destroy -1 -1 -1

		if { $st_query_keep == "FALSE" } \
			{ set_query_time -1 }

		popMsg
	} \
\
	elseif { $action == "zoom_press" } \
	{
		set zoom_x1 $canvx
		set zoom_x2 $canvx

		if { $view == "ST" } \
		{
			set zoomid [ $PRIM_C create rectangle \
				$canvx 0 $canvx $zoom_height \
				-outline blue -fill blue ]

			$PRIM_C lower $zoomid
		} \
\
		else \
		{
			set zoomid [ $PRIM_C create rectangle \
				$canvx 0 $canvx $zoom_height -outline blue ]
		}

		setTmpMsg "Help" $prim_help_zoom
	} \
\
	elseif { $action == "zoom_motion" } \
	{
		set zoom_x2 $canvx

		if { $zoom_x1 < $zoom_x2 } \
		{
			set x1 $zoom_x1
			set x2 $zoom_x2
		} \
\
		else \
		{
			set x1 $zoom_x2
			set x2 $zoom_x1
		}

		$PRIM_C coords $zoomid $x1 0 $x2 $zoom_height
	} \
\
	elseif { $action == "zoom_release" } \
	{
		$PRIM_C delete $zoomid

		if { $zoom_lock == 1 } \
		{
			bell

			return
		}

		set zoom_lock 1

		set lq [ interface_lock query ]

		if { $lq == "locked" } \
		{
			set zoom_lock 0

			bell

			return
		}

		interface_lock lock

		setMsg { Zooming... }

		set zoom_x2 $canvx

		if { $zoom_x1 != $zoom_x2 } \
		{
			if { $zoom_x1 < $zoom_x2 } \
			{
				set x1 $zoom_x1
				set x2 $zoom_x2
			} \
\
			else \
			{
				set x1 $zoom_x2
				set x2 $zoom_x1
			}

			set t1 [ expr $x1 * $scale ]
			set t2 [ expr $x2 * $scale ]

			set new_scale [ expr ( $t2 - $t1 ) / $prim_cwidth ]

			if { $new_scale == 0 } \
			{
				if { $scale == 1 } \
				{
					setMsg { Error: Maximum Zoom Already Achieved! }

					interface_lock unlock

					set zoom_lock 0

					return
				} \
\
				else \
				{
					set new_scale 1

					setMsg { Warning: Zoom Factor Limited... }
				}
			}

			zoomScrollingViews $new_scale $t1 $view

			set scale_stack [ linsert $scale_stack 0 $scale ]

			set scale $new_scale

			if { $view == "ST" } \
				{ set newx [ expr $x + $st_xview ] } \
\
			elseif { $view == "UT" } \
				{ set newx [ expr $x + $ut_xview ] }

			set newt [ expr $newx \
				- ( $frame_border * ( 1 + $FRAME_OFFSET ) ) ]

			timeid_proc move $newx 1 blue

			if { $qc_info_mode == "query" } \
			{
				set st_query_keep FALSE

				st_query_info -1 -1 -1 1
			} \
\
			elseif { $qc_info_mode == "correlate" } \
			{
				set correlate_keep FALSE

				timeid_proc modify -1 1 blue

				correlate_views -1
			}

			set_query_time $newt

			setMsg { Zooming Done. }
		} \
\
		else \
		{
			setMsg { Error - Infinite Zoom }
		}

		popMsg

		interface_lock unlock

		set zoom_lock 0
	} \
\
	elseif { $action == "unzoom" } \
	{
		if { $zoom_lock == 1 } \
		{
			bell
			
			return
		}

		set zoom_lock 1

		set lq [ interface_lock query ]

		if { $lq == "locked" } \
		{
			set zoom_lock 0

			bell

			return
		}

		interface_lock lock

		setMsg { Unzooming... }

		set new_scale [ lindex $scale_stack 0 ]

		if { $new_scale != "empty" } \
		{
			set scale_stack [ lreplace $scale_stack 0 0 ]
		} \
\
		else \
		{
			set new_scale [ expr 2 * $scale ]
		}

		set ckmax [ check_max_scale $new_scale ]

		if { $ckmax == 0 } \
		{
			setMsg { Error: Maximum Unzoom Achieved. }

			interface_lock unlock

			set zoom_lock 0

			return
		}

		set start_time [ expr ( $canvt * $scale ) \
			- ( $prim_cwidth * $new_scale / 2 ) ]

		if { $start_time < 0 } \
			{ set start_time 0 }

		zoomScrollingViews $new_scale $start_time $view

		set scale $new_scale

		if { $view == "ST" } \
			{ set newx [ expr $x + $st_xview ] } \
\
		elseif { $view == "UT" } \
			{ set newx [ expr $x + $ut_xview ] }

		set newt [ expr $newx \
			- ( $frame_border * ( 1 + $FRAME_OFFSET ) ) ]

		timeid_proc move $newx 1 blue

		if { $qc_info_mode == "query" } \
		{
			set st_query_keep FALSE

			st_query_info -1 -1 -1 1
		} \
\
		elseif { $qc_info_mode == "correlate" } \
		{
			set correlate_keep FALSE

			timeid_proc modify -1 1 blue

			correlate_views -1
		}

		set_query_time $newt

		setMsg { Unzooming Done. }

		interface_lock unlock

		set zoom_lock 0
	} \
\
	elseif { $action == "label_enter" } \
	{
		if { $st_help_label_query != [ tmpMsg ] } \
			{ setTmpMsg "Help" $st_help_label }
	} \
\
	elseif { $action == "label_press" } \
	{
		global st_label_yoff

		set tindex [ expr 10 \
			* ( $lcanvy - $border_space - $frame_border ) \
			/ $st_label_ht ]

		set tlstr [ taskLabel press $tindex ]

		set tindex [ expr $tindex - ( $tindex % 10 ) ]

		set custom [ lindex $tlstr 0 ]
		set str [ lindex $tlstr 1 ]

		set st_label_yoff 0

		if { $str != "" } \
		{
			set lck [ winfo exists $STQ ]

			if { $lck == 1 } \
				{ destroy $STQ }

			label $STQ -text $str \
				-bd 2 -relief raised -foreground $fg_color

			restrict_bindings $STQ "Label"

			set lx [ expr [winfo x $ST_L] + [winfo x $ST] \
				+ $border_space - $frame_border ]

			set ly [ expr [winfo y $ST_L] + [winfo y $ST] \
				+ ( $tindex * $st_label_ht / 10 ) \
				+ ( $st_label_ht / 2 ) + $border_space \
				+ ( 2 * $frame_border ) \
				- ( $stl_yview * $st_label_ht ) ]

			place $STQ -x $lx -y $ly -anchor w

			if { $custom == "TRUE" } \
				{ set st_label_yoff [ expr $ly - $lcanvy ] }
		}
	} \
\
	elseif { $action == "label_motion" } \
	{
		if { $y >= 0 && $y < $st_cheight \
			&& $x >= 0 && $x < $st_labels_width } \
		{
			global st_label_yoff

			set tindex \
				[ expr 10 \
					* ( $lcanvy - $border_space - $frame_border ) \
					/ $st_label_ht ]

			set tlstr [ taskLabel move $tindex ]

			set tindex [ expr $tindex - ( $tindex % 10 ) ]

			set custom [ lindex $tlstr 0 ]
			set str [ lindex $tlstr 1 ]

			if { $str != "" } \
			{
				set upck [ winfo exists $STQ ]

				if { $upck == 1 } \
					{ $STQ configure -text $str } \
\
				else \
				{
					label $STQ -text $str \
						-bd 2 -relief raised -foreground $fg_color

					restrict_bindings $STQ "Label"
				}

				set lx [ expr [winfo x $ST_L] + [winfo x $ST] \
					+ $border_space - $frame_border ]

				if { $st_label_yoff == 0 } \
				{
					set ly [ expr [winfo y $ST_L] + [winfo y $ST] \
						+ ( $tindex * $st_label_ht / 10 ) \
						+ ( $st_label_ht / 2 ) + $border_space \
						+ ( 2 * $frame_border ) \
						- ( $stl_yview * $st_label_ht ) ]
				} \
\
				else \
					{ set ly [ expr $lcanvy + $st_label_yoff ] }

				place $STQ -x $lx -y $ly -anchor w
			} \
\
			else \
			{
				set upck [ winfo exists $STQ ]

				if { $upck == 1 } \
					{ destroy $STQ }
			}
		} \
\
		else \
		{
			set upck [ winfo exists $STQ ]

			if { $upck == 1 } \
				{ destroy $STQ }
		}

		setTmpMsg Help $st_help_label_query
	} \
\
	elseif { $action == "label_release" } \
	{
		if { $y >= 0 && $y < $st_cheight \
			&& $x >= 0 && $x < $st_labels_width } \
		{
			set tindex \
				[ expr 10 \
					* ( $lcanvy - $border_space - $frame_border ) \
					/ $st_label_ht ]

			taskLabel release $tindex
		}

		set upck [ winfo exists $STQ ]

		if { $upck == 1 } \
			{ destroy $STQ }

		setTmpMsg "Help" $st_help_label
	} \
\
	elseif { $action == "label_leave" } \
	{
		set upck [ winfo exists $STQ ]

		if { $upck == 0 } \
			{ popMsg } \
\
		else \
			{ setTmpMsg "Help" $st_help_label_query }
	}

	#debug_proc canvas_handle exit
}


proc timeid_proc { op x wt color } \
{
	#debug_proc timeid_proc entry

	global st_canvas_height
	global ut_canvas_height
	global st_timeid
	global ut_timeid

	global ST_C
	global UT_C

	if { $op == "create" } \
	{
		if { $st_timeid != -1 } \
			{ $ST_C delete $st_timeid }

		if { $ut_timeid != -1 } \
			{ $UT_C delete $ut_timeid }

		set st_timeid [ $ST_C create line \
			$x 0 $x $st_canvas_height -width $wt -fill $color ]

		set ut_timeid [ $UT_C create line \
			$x 0 $x $ut_canvas_height -width $wt -fill $color ]
	} \
\
	elseif { $op == "modify" } \
	{
		if { $st_timeid != -1 } \
			{ $ST_C itemconfigure $st_timeid -width $wt -fill $color }

		if { $ut_timeid != -1 } \
			{ $UT_C itemconfigure $ut_timeid -width $wt -fill $color }
	} \
\
	elseif { $op == "move" } \
	{
		if { $st_timeid != -1 } \
		{
			$ST_C coords $st_timeid $x 0 $x $st_canvas_height
			$ST_C raise $st_timeid
		} \
\
		else \
		{
			set st_timeid [ $ST_C create line \
				$x 0 $x $st_canvas_height -width $wt -fill $color ]
		}

		if { $ut_timeid != -1 } \
		{
			$UT_C coords $ut_timeid $x 0 $x $ut_canvas_height
			$UT_C raise $ut_timeid
		} \
\
		else \
		{
			set ut_timeid [ $UT_C create line \
				$x 0 $x $ut_canvas_height -width $wt -fill $color ]
		}
	} \
\
	elseif { $op == "destroy" } \
	{
		if { $st_timeid != -1 } \
		{
			$ST_C delete $st_timeid

			set st_timeid -1
		}

		if { $ut_timeid != -1 } \
		{
			$UT_C delete $ut_timeid

			set ut_timeid -1
		}
	} \
\
	else \
		{ puts "Error in timeid_proc{}:  Unknown Op = <$op>." }

	#debug_proc timeid_proc exit
}


proc st_query_info { id tindex1 tindex2 press } \
{
	#debug_proc st_query_info entry

	global st_highlight_id
	global st_query_keep
	global st_query_text

	global ST

	#
	# Button Release
	#

	if { $id == -1 } \
	{
		if { $st_query_keep == "FALSE" } \
		{
			st_unhighlight_query noreset

			$ST.query_frame.query configure -text {View Info:}

			set st_query_text ""
		}

		return
	}

	#
	# Check for Double Click
	#

	if { $press == 1 } \
	{
		set dblclk [ double_click ]

		if { $id == $st_highlight_id && $dblclk == "1" } \
			{ set st_query_keep TRUE } \
\
		else \
			{ set st_query_keep FALSE }
	} \
\
	else \
	{
		if { $id != $st_highlight_id } \
			{ st_unhighlight_query noreset }
	}

	#
	# Un-Highlight Query
	#

	if { $st_query_keep == "FALSE" } \
		{ st_unhighlight_query noreset }

	#
	# Search for Query Item & Dump Info String
	#

	set found [ taskQuery $id $tindex1 $tindex2 ]

	if { $found == 0 } \
	{
		#
		# Search Communication Lines
		#

		set found [ commQuery $id ]

		if { $found == 0 } \
		{
			st_unhighlight_query reset

			$ST.query_frame.query configure -text {View Info:}

			set st_query_text ""
		}
	}

	#debug_proc st_query_info exit
}


proc st_unhighlight_query { reset } \
{
	#debug_proc st_unhighlight_query entry

	global st_highlight_type
	global st_highlight_id

	global ST

	if { $st_highlight_id == -1 } \
		{ return }

	if { $st_highlight_type == "state" } \
	{
		$ST.canvas itemconfigure $st_highlight_id -width 1
	} \
\
	elseif { $st_highlight_type == "comm" } \
	{
		$ST.canvas itemconfigure $st_highlight_id -arrow none -width 1
	}

	set st_highlight_type ""

	if { $reset == "reset" } \
		{ set st_highlight_id -1 }

	#debug_proc st_unhighlight_query exit
}


proc st_info_scroll { event x } \
{
	#debug_proc st_info_scroll entry

	global st_query_text_scroll
	global st_query_text_index
	global st_query_text

	if { $st_query_text == "" } \
		{ return }

	global ST

	if { $event == "enter" } \
		{ set st_query_text_index $x } \
\
	elseif { $event == "motion" } \
	{
		if { $st_query_text_index == -1 } \
			{ return }

		set delta [ expr $x - $st_query_text_index ]

		set st_query_text_index $x

		if { $delta < 0 } \
		{
			#
			# Scroll Right
			#

			set st_query_text_scroll \
				[ expr $st_query_text_scroll + $delta ]

			if { $st_query_text_scroll < 0 } \
				{ set st_query_text_scroll 0 }

			set str [ string range $st_query_text \
				$st_query_text_scroll end ]

			$ST.query_frame.query configure -text "View Info:   $str"
		} \
\
		elseif { $delta > 0 } \
		{
			#
			# Scroll Left
			#

			set fwt [ winfo width $ST.query_frame ]

			set qwt [ winfo width $ST.query_frame.query ]

			if { $qwt > $fwt } \
			{
				#
				# Limit Delta
				#

				set limit [ expr $fwt / 100 ]

				if { $delta > $limit } \
					{ set delta $limit }

				#
				# Scroll Text
				#

				set st_query_text_scroll \
					[ expr $st_query_text_scroll + $delta ]

				set str [ string range $st_query_text \
					$st_query_text_scroll end ]

				$ST.query_frame.query configure \
					-text "View Info:   $str"
			}
		}
	} \
\
	elseif { $event == "leave" } \
		{ set st_query_text_index -1 } \
\
	else \
		{ puts "Error in st_info_scroll{}:  unknown event \"$event\"" }

	#debug_proc st_info_scroll exit
}


proc zoomScrollingViews { new_scale start_time view } \
{
	#debug_proc zoomScrollingViews entry

	global frame_border
	global st_cwidth
	global ut_cwidth
	global st_xview
	global ut_xview
	global timex

	global FRAME_OFFSET

	global ST_C
	global ST_SBH

	global UT_C
	global UT_SBH

	set fo [ expr $FRAME_OFFSET * $frame_border ]

	#
	# Re-Position Space-Time State Rectangles
	#

	taskZoom $new_scale

	#
	# Re-Position Space-Time Comm Lines
	#

	commZoom $new_scale

	#
	# Re-Position Utilization Rectangles
	#

	utZoom $new_scale

	#
	# Adjust Time Parameters
	#

	timeZoom $new_scale

	#
	# Align Canvases to Proper Starting Time
	#

	set x1 [ expr $start_time / $new_scale ]

	if { $view == "ST" } \
	{
		set maxvalue [ expr $timex - $st_cwidth + (2 * $frame_border) \
			+ $fo ]

		if { $maxvalue > 0 } \
		{
			if { $x1 > $maxvalue } \
				{ set x1 $maxvalue } \
\
			elseif { $x1 < 0 } \
				{ set x1 0 }
		} \
\
		else \
			{ set x1 0 }

		do_xview $ST_C $x1

		set st_xview $x1

		scrollSet $ST_SBH $timex $st_cwidth 1 $x1

		#
		# Align Other View's Canvas
		#

		if { $timex != $st_cwidth } \
		{
			set utx1 [ expr $x1 \
				- ( $x1 * ( $ut_cwidth - $st_cwidth ) \
					/ ( $timex - $st_cwidth ) ) ]
		} \
\
		else \
		{
			set utx1 [ expr $x1 \
				- ( $x1 * ( $ut_cwidth - $st_cwidth ) ) ]
		}

		do_xview $UT_C $utx1

		set ut_xview $utx1

		scrollSet $UT_SBH $timex $ut_cwidth 1 $utx1
	} \
\
	elseif { $view == "UT" } \
	{
		set maxvalue [ expr $timex - $ut_cwidth + (2 * $frame_border) \
			+ $fo ]

		if { $maxvalue > 0 } \
		{
			if { $x1 > $maxvalue } \
				{ set x1 $maxvalue } \
\
			elseif { $x1 < 0 } \
				{ set x1 0 }
		} \
\
		else \
			{ set x1 0 }

		do_xview $UT_C $x1

		set ut_xview $x1

		scrollSet $UT_SBH $timex $ut_cwidth 1 $x1

		#
		# Align Other View's Canvas
		#

		if { $timex != $ut_cwidth } \
		{
			set stx1 [ expr $x1 \
				+ ( $x1 * ( $ut_cwidth - $st_cwidth) \
					/ ( $timex - $ut_cwidth ) ) ]
		} \
\
		else \
		{
			set stx1 [ expr $x1 \
				+ ( $x1 * ( $ut_cwidth - $st_cwidth ) ) ]
		}

		do_xview $ST_C $stx1

		set st_xview $stx1

		scrollSet $ST_SBH $timex $st_cwidth 1 $stx1
	}

	update_scroll_marks

	#debug_proc zoomScrollingViews exit
}


proc network_handle { action x y } \
{
	#debug_proc network entry

	global net_help_enter
	global net_cheight
	global net_cwidth
	global net_scroll
	global net_xview
	global net_yview
	global net_size

	global NET_C

	set ns2 [ expr $net_size / 2 ]

	set canvx [ expr $x + ( $net_xview * $net_scroll ) - $ns2 ]
	set canvy [ expr $y + ( $net_yview * $net_scroll ) - $ns2 ]

	if { $action == "query_press" } \
	{
		set id [ $NET_C find closest $canvx $canvy ]

		set str [ netHostFind $id $canvx $canvy ]

		if { $str != "" } \
		{
			if { $str != [ tmpMsg ] } \
				{ setTmpMsg "Network View Info" $str }
		}
	} \
\
	elseif { $action == "query_motion" } \
	{
		if { $y >= 0 && $y < $net_cheight \
			&& $x >= 0 && $x < $net_cwidth } \
		{
			set id [ $NET_C find closest $canvx $canvy ]

			set str [ netHostFind $id $canvx $canvy ]

			if { $str != "" } \
			{
				if { $str != [ tmpMsg ] } \
					{ setTmpMsg "Network View Info" $str }
			} \
\
			else \
				{ setTmpMsg "Help" $net_help_enter }
		} \
\
		else \
			{ setTmpMsg "Help" $net_help_enter }
	} \
\
	elseif { $action == "query_release" } \
	{
		if { $y >= 0 && $y < $net_cheight \
			&& $x >= 0 && $x < $net_cwidth } \
		{
			setTmpMsg "Help" $net_help_enter
		} \
\
		else \
			{ popMsg }
	} \
\
	elseif { $action == "enter" } \
		{ setTmpMsg "Help" $net_help_enter } \
\
	elseif { $action == "leave" } \
		{ popMsg }

	#debug_proc network exit
}


proc toggle_netkey { } \
{
	#debug_proc toggle_netkey entry

	global NK

	set cknk [ winfo ismapped $NK ]

	if { $cknk == 1 } \
		{ wm withdraw $NK } \
\
	else \
		{ wm deiconify $NK }

	#debug_proc toggle_netkey exit
}


proc mq_canvas_handle { action x y } \
{
	#debug_proc mq_canvas_handle entry

	global mq_query_index_type
	global mq_query_index
	global mq_query_keep
	global mq_query_text
	global mq_help_enter
	global mq_help_query
	global border_space
	global frame_border
	global mq_task_wt
	global mq_cheight
	global mq_cwidth
	global mq_xview
	global fg_color

	global MQ_F

	set canvx [ expr $x + ( $mq_xview * $mq_task_wt ) ]

	set str ""

	if { $action == "query_press" } \
	{
		set dblclk [ double_click ]

		set ret [ mqQuery query $canvx $y ]

		set id		[ lindex $ret 0 ]
		set type	[ lindex $ret 1 ]
		set str		[ lindex $ret 2 ]

		if { $id == $mq_query_index && $dblclk == "1" } \
			{ set mq_query_keep TRUE } \
\
		else \
			{ set mq_query_keep FALSE }

		if { $id != $mq_query_index } \
			{ mq_unhighlight noreset }

		set mq_query_index $id
		set mq_query_index_type $type

		mq_highlight

		setTmpMsg "Help" $mq_help_query
	} \
\
	elseif { $action == "query_motion" } \
	{
		set ret [ mqQuery query $canvx $y ]

		set id		[ lindex $ret 0 ]
		set type	[ lindex $ret 1 ]
		set str		[ lindex $ret 2 ]

		if { $id != $mq_query_index } \
			{ mq_unhighlight noreset }

		set mq_query_index $id
		set mq_query_index_type $type

		mq_highlight
	} \
\
	elseif { $action == "query_release" } \
	{
		if { $mq_query_keep == "TRUE" } \
		{
			if { $mq_query_index != 0 } \
				{ return } \
\
			else \
				{ set mq_query_keep FALSE }
		}

		set ret [ mqQuery entry $canvx $y ]

		set id		[ lindex $ret 0 ]
		set type	[ lindex $ret 1 ]
		set str		[ lindex $ret 2 ]

		mq_unhighlight noreset

		if { $y >= 0 && $y < $mq_cheight \
			&& $x >= 0 && $x < $mq_cwidth } \
		{
			setTmpMsg "Help" $mq_help_enter
		} \
\
		else \
			{ popMsg }
	} \
\
	elseif { $action == "enter" } \
	{
		if { $mq_query_keep == "TRUE" } \
			{ return }

		set ret [ mqQuery entry $canvx $y ]

		set id		[ lindex $ret 0 ]
		set type	[ lindex $ret 1 ]
		set str		[ lindex $ret 2 ]

		setTmpMsg "Help" $mq_help_enter
	} \
\
	elseif { $action == "move" } \
	{
		if { $mq_query_keep == "TRUE" } \
			{ return }

		set ret [ mqQuery entry $canvx $y ]

		set id		[ lindex $ret 0 ]
		set type	[ lindex $ret 1 ]
		set str		[ lindex $ret 2 ]
	} \
\
	elseif { $action == "leave" } \
	{
		if { $mq_query_keep == "TRUE" } \
			{ return }

		popMsg
	}

	#
	# Set New Query Text
	#

	$MQ_F.query_frame.query configure -text "View Info:   $str"

	set mq_query_text $str

	#debug_proc mq_canvas_handle exit
}


proc mq_highlight { } \
{
	#debug_proc mq_highlight entry

	global mq_query_index_type
	global mq_query_index

	global MQ_C

	if { $mq_query_index != 0 } \
	{
		if { $mq_query_index_type == "rect" } \
			{ $MQ_C itemconfigure $mq_query_index -width 3 } \
\
		elseif { $mq_query_index_type == "text" } \
			{ $MQ_C itemconfigure $mq_query_index -fill red }
	}

	#debug_proc mq_highlight exit
}


proc mq_unhighlight { reset } \
{
	#debug_proc mq_unhighlight entry

	global mq_query_index_type
	global mq_query_index

	global fg_color

	global MQ_C

	if { $mq_query_index != 0 } \
	{
		if { $mq_query_index_type == "rect" } \
			{ $MQ_C itemconfigure $mq_query_index -width 1 } \
\
		elseif { $mq_query_index_type == "text" } \
			{ $MQ_C itemconfigure $mq_query_index -fill $fg_color }

		if { $reset == "reset" } \
		{
			set mq_query_index 0
			set mq_query_index_type none
		}
	}

	#debug_proc mq_unhighlight exit
}


proc mq_info_scroll { event x } \
{
	#debug_proc mq_info_scroll entry

	global mq_query_text_scroll
	global mq_query_text_index
	global mq_query_text

	if { $mq_query_text == "" } \
		{ return }

	global MQ_F

	if { $event == "enter" } \
		{ set mq_query_text_index $x } \
\
	elseif { $event == "motion" } \
	{
		if { $mq_query_text_index == -1 } \
			{ return }

		set delta [ expr $x - $mq_query_text_index ]

		set mq_query_text_index $x

		if { $delta < 0 } \
		{
			#
			# Scroll Right
			#

			set mq_query_text_scroll \
				[ expr $mq_query_text_scroll + $delta ]

			if { $mq_query_text_scroll < 0 } \
				{ set mq_query_text_scroll 0 }

			set str [ string range $mq_query_text \
				$mq_query_text_scroll end ]

			$MQ_F.query_frame.query configure -text "View Info:   $str"
		} \
\
		elseif { $delta > 0 } \
		{
			#
			# Scroll Left
			#

			set fwt [ winfo width $MQ_F.query_frame ]

			set qwt [ winfo width $MQ_F.query_frame.query ]

			if { $qwt > $fwt } \
			{
				#
				# Limit Delta
				#

				set limit [ expr $fwt / 100 ]

				if { $delta > $limit } \
					{ set delta $limit }

				#
				# Scroll Text
				#

				set mq_query_text_scroll \
					[ expr $mq_query_text_scroll + $delta ]

				set str [ string range $mq_query_text \
					$mq_query_text_scroll end ]

				$MQ_F.query_frame.query configure \
					-text "View Info:   $str"
			}
		}
	} \
\
	elseif { $event == "leave" } \
		{ set mq_query_text_index -1 } \
\
	else \
		{ puts "Error in mq_info_scroll{}:  unknown event \"$event\"" }

	#debug_proc mq_info_scroll exit
}


proc ct_canvas_handle { action x y } \
{
	#debug_proc ct_canvas_handle entry

	global ct_help_label_query
	global ct_labels_width
	global ct_query_index
	global ct_help_enter
	global ct_help_label
	global border_space
	global frame_border
	global st_label_ht
	global ct_cheight
	global ct_cwidth
	global ct_yview
	global fg_color

	global CT_F
	global CT_L
	global CT_C

	set CTQ $CT_F.label_query

	set canvy [ expr $y + ( $ct_yview * $st_label_ht ) ]

	if { $action == "query_press" } \
	{
		set tindex [ expr ( $canvy - $border_space - $frame_border ) \
			/ $st_label_ht ]

		set str [ taskCTQuery $tindex ]

		if { $str != "" } \
		{
			$CT_F.query configure -text $str

			set ct_query_index $tindex
		}
	} \
\
	elseif { $action == "query_motion" } \
	{
		set tindex [ expr ( $canvy - $border_space - $frame_border ) \
			/ $st_label_ht ]

		if { $y >= 0 && $y < $ct_cheight \
			&& $x >= 0 && $x < $ct_cwidth } \
		{
			set str [ taskCTQuery $tindex ]

			if { $str != "" } \
			{
				$CT_F.query configure -text $str

				set ct_query_index $tindex
			} \
\
			else \
			{
				$CT_F.query configure -text ""

				set ct_query_index -1
			}
		} \
\
		else \
		{
			$CT_F.query configure -text ""

			set ct_query_index -1
		}
	} \
\
	elseif { $action == "query_release" } \
	{
		$CT_F.query configure -text ""

		set ct_query_index -1
	} \
\
	elseif { $action == "enter" } \
		{ setTmpMsg "Help" $ct_help_enter } \
\
	elseif { $action == "leave" } \
		{ popMsg } \
\
	elseif { $action == "label_press" } \
	{
		global ct_label_yoff

		set tindex [ expr 10 \
			* ( $canvy - $border_space - $frame_border ) \
			/ $st_label_ht ]

		set tlstr [ taskLabel press $tindex ]

		set tindex [ expr $tindex - ( $tindex % 10 ) ]

		set custom [ lindex $tlstr 0 ]
		set str [ lindex $tlstr 1 ]

		set ct_label_yoff 0

		if { $str != "" } \
		{
			set lck [ winfo exists $CTQ ]

			if { $lck == 1 } \
				{ destroy $CTQ }

			label $CTQ -text $str \
				-bd 2 -relief raised -foreground $fg_color

			restrict_bindings $CTQ "Label"

			set lx [ expr [ winfo x $CT_L ] \
				+ $border_space - $frame_border ]

			set ly [ expr [ winfo y $CT_L ] \
				+ ( $tindex * $st_label_ht / 10 ) \
				+ ( $st_label_ht / 2 ) + $border_space \
				+ ( 2 * $frame_border ) \
				- ( $ct_yview * $st_label_ht ) ]

			place $CTQ -x $lx -y $ly -anchor w

			if { $custom == "TRUE" } \
				{ set ct_label_yoff [ expr $ly - $canvy ] }
		}

		setTmpMsg "Help" $ct_help_label_query
	} \
\
	elseif { $action == "label_motion" } \
	{
		if { $y >= 0 && $y < $ct_cheight \
			&& $x >= 0 && $x < $ct_labels_width } \
		{
			global ct_label_yoff

			set tindex \
				[ expr 10 \
					* ( $canvy - $border_space - $frame_border ) \
					/ $st_label_ht ]

			set tlstr [ taskLabel move $tindex ]

			set tindex [ expr $tindex - ( $tindex % 10 ) ]

			set custom [ lindex $tlstr 0 ]
			set str [ lindex $tlstr 1 ]

			if { $str != "" } \
			{
				set upck [ winfo exists $CTQ ]

				if { $upck == 1 } \
					{ $CTQ configure -text $str } \
\
				else \
				{
					label $CTQ -text $str \
						-bd 2 -relief raised -foreground $fg_color

					restrict_bindings $CTQ "Label"
				}

				set lx [ expr [ winfo x $CT_L ] \
					+ $border_space - $frame_border ]

				if { $ct_label_yoff == 0 } \
				{
					set ly [ expr [ winfo y $CT_L ] \
						+ ( $tindex * $st_label_ht / 10 ) \
						+ ( $st_label_ht / 2 ) + $border_space \
						+ ( 2 * $frame_border ) \
						- ( $ct_yview * $st_label_ht ) ]
				} \
\
				else \
					{ set ly [ expr $canvy + $ct_label_yoff ] }

				place $CTQ -x $lx -y $ly -anchor w
			} \
\
			else \
			{
				set upck [ winfo exists $CTQ ]

				if { $upck == 1 } \
					{ destroy $CTQ }
			}
		} \
\
		else \
		{
			set upck [ winfo exists $CTQ ]

			if { $upck == 1 } \
				{ destroy $CTQ }
		}
	} \
\
	elseif { $action == "label_release" } \
	{
		if { $y >= 0 && $y < $ct_cheight \
			&& $x >= 0 && $x < $ct_labels_width } \
		{
			set tindex \
				[ expr 10 \
					* ( $canvy - $border_space - $frame_border ) \
					/ $st_label_ht ]

			taskLabel release $tindex
		}

		set upck [ winfo exists $CTQ ]

		if { $upck == 1 } \
			{ destroy $CTQ }

		setTmpMsg "Help" $ct_help_label
	} \
\
	elseif { $action == "label_enter" } \
	{
		if { $ct_help_label_query != [ tmpMsg ] } \
			{ setTmpMsg "Help" $ct_help_label }
	} \
\
	elseif { $action == "label_leave" } \
	{
		set upck [ winfo exists $CTQ ]

		if { $upck == 0 } \
			{ popMsg } \
\
		else \
			{ setTmpMsg "Help" $ct_help_label_query }
	}

	#debug_proc ct_canvas_handle exit
}


proc active { tf } \
{
	#debug_proc active entry

	if { $tf == "TRUE" } \
		{ .xpvm.active configure -text "*" } \
\
	elseif { $tf == "CLEANUP" } \
		{ .xpvm.active configure -text "(*)" } \
\
	else \
		{ .xpvm.active configure -text " " }

	#debug_proc active exit
}


proc set_output_file { } \
{
	#debug_proc set_output_file entry

	global output_file

	global TO_F

	global OF

	set ckof [ winfo ismapped $OF ]

	if { $ckof == 1 } \
		{ return }

	set file [ $TO_F.file_entry get ]

	if { $file != $output_file } \
	{
		set output_file $file

		output_file_handle $output_file new
	}

	#debug_proc set_output_file exit
}


proc set_output_filter { } \
{
	#debug_proc set_output_filter entry

	global output_filter

	global TO_F

	set filter [ $TO_F.filter_entry get ]

	if { "x$filter" != "x$output_filter" } \
	{
		set output_filter $filter

		output_filter_handle $output_filter new
	}

	#debug_proc set_output_filter exit
}


proc set_output_search { } \
{
	#debug_proc set_output_search entry

	global TO_F

	output_search_handle [ $TO_F.search_entry get ]

	#debug_proc set_output_search exit
}


proc set_tevhist_filter { } \
{
	#debug_proc set_tevhist_filter entry

	global tevhist_filter

	global TV_F

	set filter [ $TV_F.filter_entry get ]

	if { "x$filter" != "x$tevhist_filter" } \
	{
		set tevhist_filter $filter

		tevhist_filter_handle $tevhist_filter
	}

	#debug_proc set_tevhist_filter exit
}


proc set_tevhist_search { } \
{
	#debug_proc set_tevhist_search entry

	global TV_F

	tevhist_search_handle [ $TV_F.search_entry get ]

	#debug_proc set_tevhist_search exit
}


proc prompt_file_overwrite { file cmd } \
{
	#debug_proc prompt_file_overwrite entry

	global file_overwrite_cmd
	global border_space

	global OF

	$OF.file configure -text $file

	set file_overwrite_cmd $cmd

	update

	set lwt [ winfo width $OF.label ]
	set fwt [ winfo width $OF.file ]

	if { $lwt > $fwt } \
		{ set wt $lwt } \
\
	else \
		{ set wt $fwt }

	set wt [ expr $wt + (2 * $border_space) ]

	set ht [ expr [ winfo y $OF.yes ] + [ winfo height $OF.yes ] \
		+ $border_space ]

	place $OF -x 25 -y 25 -width $wt -height $ht

	raise $OF

	#debug_proc prompt_file_overwrite exit
}


proc do_file_overwrite_result { result } \
{
	#debug_proc do_file_overwrite_result entry

	global file_overwrite_cmd

	file_overwrite_result $result $file_overwrite_cmd

	#debug_proc do_file_overwrite_result exit
}


proc do_spawn { init } \
{
	#debug_proc do_spawn entry

	global spawn_close_on_start
	global spawn_flags_debug
	global spawn_flags_trace
	global spawn_flags_front
	global spawn_flags_compl
	global spawn_init_save
	global spawn_where
	global tasks_subs

	global SP

	focus .xpvm

	if { $spawn_close_on_start == "ON" } \
	{
		set cksd [ winfo ismapped $SP ]

		if { $cksd == 1 } \
		{
			raiseMenu .xpvm.tasks_menu .xpvm.tasks .xpvm \
				$tasks_subs
		}
	}

	if { $init == "again" } \
		{ set init $spawn_init_save } \
\
	else \
		{ set spawn_init_save $init }

	set cmd [ $SP.cmd_entry get ]

	if { $cmd == "" } \
		{ return }

	set name [ lindex $cmd 0 ]

	set args [ lreplace $cmd 0 0 ]

	set buf [ $SP.buf_entry get ]

	if { $buf == "" } \
		{ set buf 1 }

	set ntasks [ $SP.ntasks_entry get ]

	if { $ntasks == "" } \
		{ set ntasks 1 }

	set cmd \
		[ list pvm_tasks spawn $init $name $args $buf $ntasks ]

	set loc [ $SP.loc_entry get ]

	if { $loc != "" } \
	{
		if { $spawn_where == "Host" } \
			{ set cmd [ concat $cmd [ list host $loc ] ] } \
\
		elseif { $spawn_where == "Arch" } \
			{ set cmd [ concat $cmd [ list arch $loc ] ] }
	}

	if { $spawn_flags_debug == "ON" } \
		{ lappend cmd "debug" }

	if { $spawn_flags_trace == "ON" } \
		{ lappend cmd "trace" }

	if { $spawn_flags_front == "ON" } \
		{ lappend cmd "front" }

	if { $spawn_flags_compl == "ON" } \
		{ lappend cmd "compl" }

	eval $cmd

	#debug_proc do_spawn exit
}


proc checkSpawnWhere { } \
{
	#debug_proc checkSpawnWhere entry

	global spawn_host_string
	global spawn_arch_string
	global last_spawn_where
	global spawn_where

	global SP

	set str [ $SP.loc_entry get ]

	if { $last_spawn_where == "Host" } \
		{ set spawn_host_string $str } \
\
	elseif { $last_spawn_where == "Arch" } \
		{ set spawn_arch_string $str }

	$SP.loc_entry delete 0 end

	if { $spawn_where == "Host" } \
		{ $SP.loc_entry insert 0 $spawn_host_string } \
\
	elseif { $spawn_where == "Arch" } \
		{ $SP.loc_entry insert 0 $spawn_arch_string }

	set last_spawn_where $spawn_where

	layout_spawn_dialog

	#debug_proc checkSpawnWhere exit
}


proc set_trace_format { tfmt } \
{
	#debug_proc set_trace_format entry

	global trace_format
	global tm33_subs
	global tm_subs

	global OTF

	global SP

	#
	# Lower Any Trace Mask Menus
	#

	place forget .xpvm.trace_mask_menu

	foreach i $tm_subs \
		{ place forget $i }

	place forget .xpvm.trace33_mask_menu

	foreach i $tm33_subs \
		{ place forget $i }

	#
	# Set Up Correct Trace Mask Menu
	#

	set trace_format $tfmt

	if { $tfmt == "3.3" } \
	{
		set menu .xpvm.trace33_mask_menu

		set cmd [ list raiseMenuIndirect $menu trace_mask_button \
			$SP.trace_mask .xpvm $tm33_subs ]

		$SP.trace_mask configure -command $cmd

		set cmd [ list raiseMenuIndirect $menu trace_mask_button \
			$OTF.trace_mask .xpvm $tm33_subs ]

		$OTF.trace_mask configure -command $cmd
	} \
\
	elseif { $tfmt == "3.4" } \
	{
		set menu .xpvm.trace_mask_menu

		set cmd [ list raiseMenuIndirect $menu trace_mask_button \
			$SP.trace_mask .xpvm $tm_subs ]

		$SP.trace_mask configure -command $cmd

		set cmd [ list raiseMenuIndirect $menu trace_mask_button \
			$OTF.trace_mask .xpvm $tm33_subs ]

		$OTF.trace_mask configure -command $cmd
	} \
\
	else \
		{ puts "Error: Unknown Tracing Format" }

	layout_spawn_dialog

	#debug_proc set_trace_format exit
}


proc trace_mask { index label menuvar setflag } \
{
	#debug_proc trace_mask entry

	global $menuvar

	set state [ set $menuvar ]

	pvm_trace_mask $index $state

	#debug_proc trace_mask exit
}


proc trace_mask_result { result } \
{
	#debug_proc trace_mask_result entry

	global trace_format

	if { $trace_format == "3.3" } \
		{ set prefix "trace33_mask" } \
\
	elseif { $trace_format == "3.4" } \
		{ set prefix "trace_mask" } \
\
	else \
	{
		puts "Error: Unknown Tracing Format"

		return
	}

	foreach ev $result \
	{
		set index [ lindex $ev 0 ]
		set onoff [ lindex $ev 1 ]

		set menuvar "${prefix}_state_$index"

		global $menuvar

		set $menuvar $onoff
	}

	#debug_proc trace_mask_result exit
}


proc do_trace_buffer { } \
{
	#debug_proc do_trace_buffer entry

	global SP

	set tbuf [ $SP.buf_entry get ]

	set_trace_buffer $tbuf

	#debug_proc do_trace_buffer exit
}


proc do_otf { index } \
{
	#debug_proc do_otf entry

	global tracing_status
	global border_space
	global otf_tasklist
	global otf_lasty
	global row_height

	if { $index == "refresh" } \
		{ refresh_tasks otf } \
\
	elseif { $index == "all" } \
	{
		pvm_tasks otf 0 $tracing_status

		refresh_tasks otf
	} \
\
	else \
	{
		if { $index > 0 && $index < $otf_lasty } \
		{
			set i [ expr ( $index - $border_space ) / $row_height ]

			set max [ llength $otf_tasklist ]

			set t [ lindex $otf_tasklist [ expr $max - $i - 2 ] ]

			set name [ lindex $t 1 ]
			set tid [ lindex $t 2 ]

			pvm_tasks otf $tid $tracing_status

			refresh_tasks otf
		}
	}

	#debug_proc do_otf exit
}


proc do_kill { index } \
{
	#debug_proc do_kill entry

	global kill_tasklist
	global border_space
	global kill_lasty
	global row_height

	if { $index == "refresh" } \
		{ refresh_tasks kill } \
\
	elseif { $index == "all" } \
	{
		pvm_tasks kill 0 all

		refresh_tasks kill
	} \
\
	else \
	{
		if { $index > 0 && $index < $kill_lasty } \
		{
			set i [ expr ( $index - $border_space ) / $row_height ]

			set max [ llength $kill_tasklist ]

			set t [ lindex $kill_tasklist [ expr $max - $i - 2 ] ]

			set name [ lindex $t 1 ]
			set tid [ lindex $t 2 ]

			pvm_tasks kill $tid $name

			refresh_tasks kill
		}
	}

	#debug_proc do_kill exit
}


proc do_signal { index } \
{
	#debug_proc do_signal entry

	global signal_selected
	global signal_tasklist
	global signal_lasty
	global border_space
	global row_height

	if { $index == "refresh" } \
		{ refresh_tasks signal } \
\
	elseif { $index == "all" } \
	{
		pvm_tasks signal 0 $signal_selected

		refresh_tasks signal
	} \
\
	else \
	{
		if { $index > 0 && $index < $signal_lasty } \
		{
			set i [ expr ( $index - $border_space ) / $row_height ]

			set max [ llength $signal_tasklist ]

			set t [ lindex $signal_tasklist [ expr $max - $i - 2 ] ]

			set tid [ lindex $t 2 ]

			pvm_tasks signal $tid $signal_selected

			refresh_tasks signal
		}
	}

	#debug_proc do_signal exit
}


proc systasks { index label menuvar setflag } \
{
	#debug_proc systasks entry

	global $menuvar

	set state [ set $menuvar ]

	eval "pvm_tasks systasks $index $state"

	#debug_proc systasks exit
}


proc reset { index label menuvar setflag } \
{
	#debug_proc reset entry

	if { $index == "pvm" } \
		{ reset_pvm } \
\
	elseif { $index == "views" } \
		{ reset_views } \
\
	elseif { $index == "trace_file" } \
		{ reset_trace_file } \
\
	elseif { $index == "all" } \
	{
		#
		# Reset PVM after interface & trace file
		#     - to get group servers, etc, if desired.
		#
		# Reset Views after Trace File to insure that
		#     TRACE_PENDING is cleared immediately.
		#

		reset_trace_file
		reset_views
		reset_pvm
	}

	#debug_proc reset exit
}


proc handleTraceFileStatus { } \
{
	#debug_proc handleTraceFileStatus entry

	global trace_file_status

	global OV

	trace_file_status_handle $trace_file_status

	if { $trace_file_status == "PlayBack" } \
	{
		set ckov [ winfo ismapped $OV ]

		if { $ckov == 1 } \
		{
			place forget $OV
		}
	}

	#debug_proc handleTraceFileStatus exit
}


proc set_trace_file { } \
{
	#debug_proc set_trace_file entry

	global trace_file

	global CTRL

	set file [ $CTRL.file_entry get ]

	if { $file != $trace_file } \
	{
		set trace_file $file

		trace_file_handle $trace_file
	}

	#debug_proc set_trace_file exit
}


proc prompt_trace_overwrite { file } \
{
	#debug_proc prompt_trace_overwrite entry

	global border_space

	global OV

	$OV.file configure -text $file

	update

	set lwt [ winfo width $OV.label ]
	set fwt [ winfo width $OV.file ]

	if { $lwt > $fwt } \
		{ set wt $lwt } \
\
	else \
		{ set wt $fwt }

	set wt [ expr $wt + (2 * $border_space) ]

	set ht [ expr [ winfo y $OV.yes ] + [ winfo height $OV.yes ] \
		+ $border_space ]

	place $OV -x 175 -y 200 -width $wt -height $ht

	raise $OV

	#debug_proc prompt_trace_overwrite exit
}


proc reset_interface { } \
{
	#debug_proc reset_interface entry

	global st_canvas_height
	global frame_border
	global st_timeline
	global to_lasty
	global tv_lasty
	global timex

	global FRAME_OFFSET

	global ST_C

	global CTRL

	#
	# Reset Network Host States
	#

	netReset

	#
	# Clear All Tasks
	#

	taskReset

	#
	# Task Output Scroll Size
	#

	set to_lasty $frame_border

	#
	# Trace Event History Scroll Size
	#

	set tv_lasty $frame_border

	#
	# Re-Initialize Time / Space-Time
	#

	$CTRL.time configure -text {Time: 0.000000}

	set timex [ expr $frame_border * ( 1 + $FRAME_OFFSET ) ]

	$ST_C coords $st_timeline $timex 0 $timex $st_canvas_height

	#
	# Clear Utilization Rectangles
	#

	utReset

	#
	# Clear Message Queue Rectangles
	#

	mqReset

	#
	# Reset Scrollbars / Canvases
	#

	scrollReset

	#debug_proc reset_interface exit
}


proc scrollReset {} \
{
	#debug_proc scrollReset entry

	global ct_canvas_width
	global to_canvas_width
	global tv_canvas_width
	global tasks_height
	global st_label_ht
	global net_cheight
	global net_hheight
	global net_cwidth
	global net_hwidth
	global row_height
	global net_scroll
	global st_cheight
	global ct_cheight
	global to_cheight
	global tv_cheight
	global st_cwidth
	global ut_cwidth
	global ct_cwidth
	global to_cwidth
	global tv_cwidth
	global to_lasty
	global tv_lasty
	global net_size

	global NET_C
	global NET_SBH
	global NET_SBV

	global ST_L
	global ST_C
	global ST_SBH
	global ST_SBV

	global UT_C
	global UT_SBH

	global CT_L
	global CT_C
	global CT_SBH
	global CT_SBV

	global TO_C
	global TO_SBH
	global TO_SBV

	global TV_C
	global TV_SBH
	global TV_SBV

	scrollInit $NET_C $NET_SBH $net_size $net_hwidth $net_cwidth \
		net_xview HORIZ center $net_scroll

	scrollInit $NET_C $NET_SBV $net_size $net_hheight $net_cheight \
		net_yview VERT center $net_scroll

	scrollInit $ST_C $ST_SBH -1 $st_cwidth $st_cwidth st_xview \
		HORIZ low 1

	scrollInit $ST_C $ST_SBV -1 $tasks_height $st_cheight stc_yview \
		VERT low 1

	scrollInit $ST_L $ST_SBV -1 $tasks_height $st_cheight stl_yview \
		VERT low $st_label_ht

	scrollInit $UT_C $UT_SBH -1 $ut_cwidth $ut_cwidth ut_xview \
		HORIZ low 1

	scrollInit $CT_C $CT_SBH -1 $ct_canvas_width $ct_cwidth ct_xview \
		HORIZ low 1

	scrollInit $CT_L $CT_SBV -1 $tasks_height $ct_cheight ct_yview \
		VERT low $st_label_ht

	scrollInit $CT_C $CT_SBV -1 $tasks_height $ct_cheight ct_yview \
		VERT low $st_label_ht

	scrollInit $TO_C $TO_SBH -1 $to_canvas_width $to_cwidth to_xview \
		HORIZ low $row_height

	scrollInit $TO_C $TO_SBV -1 $to_lasty $to_cheight to_yview \
		VERT low $row_height

	scrollInit $TV_C $TV_SBH -1 $tv_canvas_width $tv_cwidth tv_xview \
		HORIZ low $row_height

	scrollInit $TV_C $TV_SBV -1 $tv_lasty $tv_cheight tv_yview \
		VERT low $row_height

	#debug_proc scrollReset exit
}


proc do_pvm_hosts { index label menuvar setflag } \
{
	#debug_proc do_pvm_hosts entry

	global border_space

	if { $menuvar == "none" } \
	{
		if { $label == "Add All Hosts" } \
		{
			#
			# Add All Hosts
			#

			pvm_hosts $index
		} \
\
		elseif { $label == "Other Hosts..." } \
		{
			#
			# Add Other Host
			#

			global OH

			set ckmpd [ winfo ismapped $OH ]

			if { $ckmpd == 0 } \
			{
				place $OH -x 150 -y 175 \
					-width [ expr [ winfo width $OH.label ] \
						+ (2 * $border_space) ] \
					-height [ expr [ winfo height $OH.label ] \
						+ [ winfo height $OH.host_entry ] \
						+ [ winfo height $OH.accept ] \
						+ (3 * $border_space) ]

				raise $OH

				focus $OH.host_entry
			} \
\
			else \
				{ place forget $OH }
		}
	} \
\
	else \
	{
		#
		# Add / Delete Specific Host
		#

		global $menuvar

		set state [ set $menuvar ]

		if { $setflag == "TRUE" } \
		{
			if { $state == "ON" } \
			{
				set $menuvar OFF

				set state OFF
			} \
\
			else \
			{
				set $menuvar ON

				set state ON
			}
		}

		pvm_hosts $index
	}

	#debug_proc do_pvm_hosts exit
}


proc do_pvm_other_host { } \
{
	#debug_proc do_pvm_other_host entry

	global OH

	set new_host [ $OH.host_entry get ]

	place forget $OH

	$OH.host_entry delete 0 end

	set strlen [ string length $new_host ]

	if { $strlen > 0 } \
		{ pvm_hosts 1 $new_host }

	#debug_proc do_pvm_other_host exit
}


proc refreshHostsMenu { } \
{
	#debug_proc refreshHostsMenu entry

	#
	# Destroy Existing Menu Hierarchy
	#

	set name .xpvm.hosts_menu

	set wck [ winfo exists $name ]

	if { $wck == 0 } \
		{ return }

	set wup [ winfo ismapped $name ]

	destroy $name

	#
	# Clear Globals
	#

	set root [ rootName $name ]

	set cmil "${root}_cmd_list"

	global $cmil

	set $cmil ""

	set hmil "${root}_hosts_list"

	global $hmil

	set $hmil ""

	#
	# Re-Generate Hosts Menu
	#

	global hosts_subs

	set cmd [ list raiseMenu .xpvm.hosts_menu .xpvm.hosts .xpvm \
		$hosts_subs ]

	makeHostsMenu .xpvm.hosts_menu do_pvm_hosts \
		[ get_hosts_menu_list ] $cmd $hosts_subs none

	#
	# Raise Menu
	#

	if { $wup == 1 } \
	{
		eval $cmd

		update
	}

	#debug_proc refreshHostsMenu exit
}


proc setNetworkView { } \
{
	#debug_proc setNetworkView entry

	global net_cheight
	global net_hheight
	global net_cwidth
	global net_hwidth
	global net_scroll
	global net_xview
	global net_yview
	global net_size

	global NET_C
	global NET_SBH
	global NET_SBV

	#
	# Set Network View Location
	#

	set scroll_yview [ expr $net_yview \
		- [ scrollCenterValue $net_size $net_hheight $net_scroll ] ]

	set max_yview \
		[ scrollMaxValue $net_hheight $net_cheight $net_scroll ]

	if { $scroll_yview < 0 || $max_yview < 0 || \
		( $max_yview >= 0 && $scroll_yview > $max_yview ) } \
	{
		set net_yview \
			[ scrollCenterValue $net_size $net_cheight $net_scroll ]

		do_yview $NET_C $net_yview
	}

	set scroll_xview [ expr $net_xview \
		- [ scrollCenterValue $net_size $net_hwidth $net_scroll ] ]

	set max_xview \
		[ scrollMaxValue $net_hwidth $net_cwidth $net_scroll ]

	if { $scroll_xview < 0 || $max_xview < 0 || \
		( $max_xview >= 0 && $scroll_xview > $max_xview ) } \
	{
		set net_xview \
			[ scrollCenterValue $net_size $net_cwidth $net_scroll ]

		do_xview $NET_C $net_xview
	}

	#
	# Set Network Scrollbars
	#

	if { $net_hheight < $net_cheight } \
		{ set yview 0 } \
\
	else \
	{
		set yview [ expr $net_yview \
			- ( ( ($net_size / 2) - ($net_hheight / 2) ) \
				/ $net_scroll ) ]

		if { $yview < 0 } { set yview 0 }
	}

	if { $net_hwidth < $net_cwidth } \
		{ set xview 0 } \
\
	else \
	{
		set xview [ expr $net_xview \
			- ( ( ($net_size / 2) - ($net_hwidth / 2) ) \
				/ $net_scroll ) ]

		if { $xview < 0 } { set xview 0 }
	}

	scrollSet $NET_SBV $net_hheight $net_cheight $net_scroll $yview

	scrollSet $NET_SBH $net_hwidth $net_cwidth $net_scroll $xview

	#debug_proc setNetworkView exit
}


proc fileHandle { cmd label menuvar setflag } \
{
	#debug_proc fileHandle entry

	if { $cmd == "quit" } \
	{
		pvm_quit
	} \
\
	elseif { $cmd == "halt" } \
	{
		pvm_halt
	}

	#debug_proc fileHandle exit
}


proc viewSelect { view label menuvar setflag } \
{
	#debug_proc viewSelect entry

	global $menuvar

	global min_main_height
	global min_main_width
	global min_net_height
	global min_st_height
	global height_left
	global net_active
	global net_height
	global net_width
	global mq_active
	global ct_active
	global to_active
	global tv_active
	global st_active
	global ut_active
	global st_height
	global st_width
	global scale

	global UT
	global MQ
	global CT
	global TO
	global TV

	set state [ set $menuvar ]

	if { $setflag == "TRUE" } \
	{
		if { $state == "ON" } \
		{
			set $menuvar OFF

			set state OFF
		} \
\
		else \
		{
			set $menuvar ON

			set state ON
		}
	}

	if { $view == "network" } \
	{
		if { $state == "ON" } \
		{
			set min_main_height \
				[ expr $min_main_height + $min_net_height ]

			checkMainHeight

			set net_active TRUE

			if { $st_active == "TRUE" } \
			{
				set net_height [ expr $height_left / 2 ]

				if { $net_height < $min_net_height } \
					{ set net_height $min_net_height }

				set st_height [ expr $height_left - $net_height ]

				if { $st_height < $min_st_height } \
				{
					set net_height [ expr $net_height - \
						( $min_st_height - $st_height ) ]

					set st_height $min_st_height
				}
			} \
\
			else \
				{ set net_height $height_left }

			netUpdate
		} \
\
		else \
		{
			set net_active FALSE

			set net_height 2

			if { $st_active == "TRUE" } \
				{ set st_height $height_left }

			set min_main_height \
				[ expr $min_main_height - $min_net_height ]
		}

		layout_main_panel

		wm minsize .xpvm $min_main_width $min_main_height
	} \
\
	elseif { $view == "space_time" } \
	{
		if { $state == "ON" } \
		{
			set min_main_height \
				[ expr $min_main_height + $min_st_height ]

			checkMainHeight

			set st_active TRUE

			if { $net_active == "TRUE" } \
			{
				set net_height [ expr $height_left / 2 ]

				if { $net_height < $min_net_height } \
					{ set net_height $min_net_height }

				set st_height [ expr $height_left - $net_height ]

				if { $st_height < $min_st_height } \
				{
					set net_height [ expr $net_height - \
						( $min_st_height - $st_height ) ]

					set st_height $min_st_height
				}
			} \
\
			else \
				{ set st_height $height_left }

			taskSTUpdate

			timeZoom $scale

			if { $ut_active == "TRUE" } \
				{ rescrollViews UT }
		} \
\
		else \
		{
			set st_active FALSE

			set st_height 0

			if { $net_active == "TRUE" } \
				{ set net_height $height_left }

			set min_main_height \
				[ expr $min_main_height - $min_st_height ]
		}

		layout_main_panel

		wm minsize .xpvm $min_main_width $min_main_height
	} \
\
	elseif { $view == "utilization" } \
	{
		if { $state == "ON" } \
		{
			wm deiconify $UT

			set ut_active TRUE

			utUpdate

			timeZoom $scale

			if { $st_active == "TRUE" } \
				{ rescrollViews ST }
		} \
\
		else \
		{
			wm withdraw $UT

			set ut_active FALSE
		}
	} \
\
	elseif { $view == "msgq" } \
	{
		if { $state == "ON" } \
		{
			wm deiconify $MQ

			set mq_active TRUE

			mqUpdate
		} \
\
		else \
		{
			wm withdraw $MQ

			set mq_active FALSE
		}
	} \
\
	elseif { $view == "call_trace" } \
	{
		if { $state == "ON" } \
		{
			wm deiconify $CT

			set ct_active TRUE

			taskCTUpdate
		} \
\
		else \
		{
			wm withdraw $CT

			set ct_active FALSE
		}
	} \
\
	elseif { $view == "task_output" } \
	{
		if { $state == "ON" } \
		{
			wm deiconify $TO

			set to_active TRUE

			toUpdate
		} \
\
		else \
		{
			wm withdraw $TO

			set to_active FALSE
		}
	} \
\
	elseif { $view == "task_tevhist" } \
	{
		if { $state == "ON" } \
		{
			wm deiconify $TV

			set tv_active TRUE

			tvUpdate
		} \
\
		else \
		{
			wm withdraw $TV

			set tv_active FALSE
		}
	}

	#debug_proc viewSelect exit
}


proc optionsHandle { cmd label menuvar setflag } \
{
	#debug_proc fileHandle entry

	global correlate_keep
	global correlate_lock
	global st_query_keep

	if { $cmd == "query" } \
	{
		#
		# Clean Up Any Correlate Stuff
		#

		timeid_proc modify -1 1 blue

		set correlate_lock FALSE

		set correlate_keep FALSE

		correlate_views -1
	} \
\
	elseif { $cmd == "correlate" } \
	{
		#
		# Clean Up Any Query
		#

		set st_query_keep FALSE

		st_query_info -1 -1 -1 0

		set_query_time -1
	} \
\
	elseif { $cmd == "alpha" } \
	{
		set_task_sort alpha
	} \
\
	elseif { $cmd == "tid" } \
	{
		set_task_sort tid
	} \
\
	elseif { $cmd == "custom" } \
	{
		set_task_sort custom
	}

	#debug_proc fileHandle exit
}


proc raiseFastForwardFrames { } \
{
	#debug_proc raiseFastForwardFrames entry

	global FF_LIST

	global border_space

	set bs2 [ expr 2 * $border_space ]

	foreach ff $FF_LIST \
	{
		set name [ lindex $ff 0 ]

		set ht [ expr [ winfo height $name.label ] + $bs2 ]
		set wt [ expr [ winfo width $name.label ] + $bs2 ]

		set canvas [ lindex $ff 1 ]

		set cht [ winfo height $canvas ]
		set cwt [ winfo width $canvas ]

		set x [ expr [ winfo x $canvas ] + ( $cwt / 2 ) - ( $wt / 2 ) ]
		set y [ expr [ winfo y $canvas ] + ( $cht / 2 ) - ( $ht / 2 ) ]

		place $name -x $x -y $y -width $wt -height $ht
	}

	#debug_proc raiseFastForwardFrames exit
}


proc lowerFastForwardFrames { } \
{
	#debug_proc lowerFastForwardFrames entry

	global FF_LIST

	foreach ff $FF_LIST \
	{
		set name [ lindex $ff 0 ]

		place forget $name
	}

	#debug_proc lowerFastForwardFrames exit
}


proc checkMainHeight { } \
{
	#debug_proc checkMainHeight entry

	global min_main_height
	global start_main_y
	global height_left
	global main_height
	global ctrl_height

	set ht [ winfo height .xpvm ]

	if { $ht < $min_main_height } \
	{
		set wt [ winfo width .xpvm ]

		set geom "[ expr $wt ]x[ expr $min_main_height ]"

		wm geometry .xpvm $geom

		update

		set height_left \
			[ expr $main_height - $start_main_y - $ctrl_height ]
	}

	#debug_proc checkMainHeight exit
}


proc adjust_main_panel { action y } \
{
	#debug_proc adjust_main_panel entry

	global min_net_height
	global min_st_height
	global hold_motion
	global net_active
	global net_height
	global st_active
	global st_height
	global adjust_y
	global nest_y

	if { $net_active != "TRUE" || $st_active != "TRUE" } \
		{ return }

	global ADJ

	if { $action == "press" } \
	{
		$ADJ configure -state active -relief sunken

		set hold_motion 0

		set nest_y NONE

		set adjust_y $y
	} \
\
	elseif { $action == "release" } \
	{
		$ADJ configure -state normal -relief raised
	} \
\
	elseif { $action == "motion" } \
	{
		if { $hold_motion == 0 } \
		{
			set hold_motion 1

			set first 1

			while { $first == 1 || $nest_y != "NONE" } \
			{
				if { $first == 1 } \
					{ set first 0 } \
\
				else \
				{
					set ry $nest_y

					set nest_y NONE

					set rooty [ winfo rooty $ADJ ]

					set y [ expr $ry - $rooty ]
				}

				set delta_y [ expr $y - $adjust_y ]

				if { $delta_y != 0 } \
				{
					set rooty [ winfo rooty $ADJ ]

					set ry [ expr $y + $rooty ]

					set net_height [ expr $net_height + $delta_y ]

					if { $net_height < $min_net_height } \
					{
						set delta_y [ expr $delta_y + \
							( $min_net_height - $net_height ) ]

						set net_height $min_net_height
					}

					set st_height [ expr $st_height - $delta_y ]

					if { $st_height < $min_st_height } \
					{
						set delta_y [ expr $delta_y - \
							( $min_st_height - $st_height ) ]

						set net_height [ expr $net_height - \
							( $min_st_height - $st_height ) ]

						set st_height $min_st_height
					}

					layout_main_panel

					set rooty [ winfo rooty $ADJ ]

					set adjust_y [ expr $ry - [ winfo rooty $ADJ ] ]
				}
			}

			set hold_motion 0
		} \
\
		else \
		{
			set rooty [ winfo rooty $ADJ ]

			set nest_y [ expr $rooty + $y ]
		}
	}

	#debug_proc adjust_main_panel exit
}


proc change_st_height { way } \
{
	#debug_proc change_st_height entry

	global change_st_height_lock
	global st_label_ht
	global st_rect_ht

	global ST

	if { $change_st_height_lock == 0 } \
	{
		set change_st_height_lock 1

		set lq [ interface_lock query ]

		if { $lq == "locked" } \
		{
			set change_st_height_lock 0

			bell

			return
		}

		interface_lock lock

		if { $way == "shrink" } \
		{
			setMsg { Shrinking Task Height... }

			set st_label_ht [ expr $st_label_ht - 1 ]
		} \
\
		elseif { $way == "grow" } \
		{
			setMsg { Growing Task Height... }

			set st_label_ht [ expr $st_label_ht + 1 ]
		} \
\
		elseif { $way == "set" } \
		{
			setMsg { Setting Task Height... }

			set new_ht [ $ST.task_height_entry get ]

			set ck [ scan $new_ht "%d" new_ht_val ]

			if { $ck == 0 || $new_ht_val <= 0 } \
			{
				$ST.task_height_entry delete 0 end

				$ST.task_height_entry insert 0 $st_label_ht

				set change_st_height_lock 0

				bell

				return
			}

			set st_label_ht $new_ht_val
		}

		set st_rect_ht [ expr ( 8 * $st_label_ht ) / 20 ]

		if { $st_label_ht < 4 } \
			{ set st_label_ht 4 }

		if { $st_rect_ht < 3 } \
			{ set st_rect_ht 3 }

		#
		# Update Entry Text
		#

		$ST.task_height_entry delete 0 end

		$ST.task_height_entry insert 0 $st_label_ht

		#
		# Fix Scroll Stuff
		#

		global stl_yview

		global XSCROLLINCREMENT
		global YSCROLLINCREMENT

		global ST_L
		global ST_C
		global ST_SBV

		global CT_L
		global CT_C
		global CT_SBH
		global CT_SBV

		$ST_L configure $YSCROLLINCREMENT $st_label_ht

		$CT_L configure $YSCROLLINCREMENT $st_label_ht
		$CT_C configure $XSCROLLINCREMENT $st_label_ht \
			$YSCROLLINCREMENT $st_label_ht

		set cmd_sv [ list scrollTwoCanvases $ST_L $ST_C $ST_SBV \
			-1 tasks_height st_cheight stl_yview stc_yview VERT low \
			$st_label_ht 1 ]

		$ST_SBV configure -command $cmd_sv

		set cmd_ch [ list scrollCanvas $CT_C $CT_SBH \
			-1 ct_canvas_width ct_cwidth ct_xview HORIZ low \
			$st_label_ht ]

		$CT_SBH configure -command $cmd_ch

		set cmd_cv [ list scrollTwoCanvases $CT_L $CT_C $CT_SBV \
			-1 tasks_height ct_cheight ct_yview ct_yview VERT low \
			$st_label_ht $st_label_ht ]

		$CT_SBV configure -command $cmd_cv

		taskArrange

		eval "$cmd_sv $stl_yview"

		setMsg { Task Height Adjusted. }

		interface_lock unlock

		set change_st_height_lock 0
	} \
\
	else \
		{ bell }

	#debug_proc change_st_height exit
}


proc resize_main_panel { } \
{
	#debug_proc resize_main_panel entry

	global min_net_height
	global min_st_height
	global start_main_y
	global height_left
	global ctrl_height
	global main_height
	global ctrl_width
	global main_width
	global net_active
	global net_height
	global net_width
	global st_active
	global st_height
	global st_width

	set ht [ winfo height .xpvm ]
	set wt [ winfo width .xpvm ]

	if { $main_width != $wt || $main_height != $ht } \
	{
		set main_height $ht
		set main_width $wt

		set height_left \
			[ expr $main_height - $start_main_y - $ctrl_height ]

		if { $net_active == "TRUE" } \
		{
			set net_height [ expr ( $net_height * $height_left ) \
				/ ( $net_height + $st_height ) ]

			if { $net_height < $min_net_height } \
				{ set net_height $min_net_height }
		} \
\
		else \
			{ set net_height 2 }

		if { $st_active == "TRUE" } \
		{
			set st_height [ expr $height_left - $net_height ]

			if { $st_height < $min_st_height } \
			{
				set net_height [ expr $net_height + \
					( $min_st_height - $st_height ) ]

				set st_height $min_st_height
			}
		} \
\
		else \
			{ set st_height 0 }

		set ctrl_width $wt
		set net_width $wt
		set st_width $wt

		layout_main_panel

		fix_menus_resize
	}

	#debug_proc resize_main_panel exit
}


proc layout_main_panel { } \
{
	#debug_proc layout_main_panel entry

	global st_labels_width
	global border_space
	global scroll_width
	global main_height
	global ctrl_height
	global main_width
	global ctrl_width
	global net_active
	global net_height
	global net_width
	global st_active
	global st_height
	global st_width
	global ctrl_x
	global ctrl_y
	global net_y
	global st_y

	global CTRL

	global NET

	global ST

	#
	# Status Message Label
	#

	set msg_wt [ expr $main_width - (2 * $border_space) ]

	place .xpvm.message -width $msg_wt

	place .xpvm.tmp_msg -width $msg_wt

	#
	# Set View Positions
	#

	set ctrl_y [ expr $net_y + $net_height ]

	set st_y [ expr $ctrl_y + $ctrl_height ]

	#
	# Layout Network View
	#

	if { $net_active == "TRUE" } \
	{
		layout_net_panel

		setNetworkView
	} \
\
	else \
		{ place forget $NET }

	#
	# Layout View Select & Playback Controls
	#

	set x [ expr $ctrl_width - $border_space ]

	place $CTRL.overwrite -x $x

	update

	set x [ expr [ left $CTRL.overwrite ] - $border_space ]

	place $CTRL.playback -x $x

	update

	set fewt [ expr [ left $CTRL.playback ] \
		- [ winfo x $CTRL.file ] - [ winfo width $CTRL.file ] \
		- $border_space ]

	place $CTRL.file_entry -width $fewt

	place $CTRL -x $ctrl_x -y $ctrl_y \
		-width $ctrl_width -height $ctrl_height

	#
	# Layout Space-Time View
	#

	if { $st_active == "TRUE" } \
	{
		layout_st_panel
	} \
\
	else \
		{ place forget $ST }

	update

	#debug_proc layout_main_panel exit
}


proc layout_net_panel { } \
{
	#debug_proc layout_net_panel entry

	global frame_border
	global border_space
	global scroll_width
	global net_cheight
	global net_hheight
	global net_cwidth
	global net_hwidth
	global net_scroll
	global net_active
	global net_height
	global net_width
	global net_xview
	global net_yview
	global net_size
	global depth
	global net_x
	global net_y

	global NET
	global NET_C
	global NET_SBH
	global NET_SBV

	#
	# Top Titles
	#

	place $NET -x $net_x -y $net_y \
		-width $net_width -height $net_height

	place $NET.title -relx 0.5 -y 0 -anchor n

	#
	# Bottom Buttons
	#

	set kly [ expr $net_height - ($border_space / 2) ]

	place $NET.close -x $border_space -y $kly -anchor sw

	set kky [ expr $kly - $frame_border ]

	if { $depth != 1 } \
	{
		place $NET.active_label -relx 0.23 -y $kky -anchor s
		place $NET.system_label -relx 0.43 -y $kky -anchor s
		place $NET.empty_label -relx 0.65 -y $kky -anchor s

		place $NET.netkey \
			-x [ expr $net_width - $border_space ] -y $kly -anchor se
	} \
\
	else \
	{
		place $NET.active_label -relx 0.21 -y $kky -anchor sw
		place $NET.system_label -relx 0.44 -y $kky -anchor sw
		place $NET.empty_label -relx 0.70 -y $kky -anchor sw
	}

	update

	set sz [ expr [ winfo height $NET.active_label ] - 2 ]

	set kky [ expr $kky - 2 ]

	place $NET.active_box \
		-x [ right $NET.active_label ] -y $kky -anchor sw \
		-width $sz -height $sz

	place $NET.system_box \
		-x [ right $NET.system_label ] -y $kky -anchor sw \
		-width $sz -height $sz

	place $NET.empty_box \
		-x [ right $NET.empty_label ] -y $kky -anchor sw \
		-width $sz -height $sz

	update

	#
	# Canvas & Scrollbars
	#

	set net_cy [ below $NET.title ]

	set net_cheight [ expr [ above $NET.close ] - $net_cy \
		- $scroll_width - ($border_space / 2) ]

	set net_cwidth [ expr $net_width - $scroll_width \
		- (2 * $border_space)]

	set net_sbx $border_space

	place $NET_SBV -x $net_sbx -y $net_cy \
		-width $scroll_width -height $net_cheight

	if { $net_hheight < $net_cheight } \
		{ set yview 0 } \
\
	else \
	{
		set yview [ expr $net_yview \
			- ( ( ($net_size / 2) - ($net_hheight / 2) ) \
				/ $net_scroll ) ]

		if { $yview < 0 } { set yview 0 }
	}

	scrollSet $NET_SBV $net_hheight $net_cheight $net_scroll $yview

	set net_cx [ expr $net_sbx + $scroll_width ]

	place $NET_C -x $net_cx -y $net_cy \
		-width $net_cwidth -height $net_cheight

	set net_sby [ expr $net_cy + $net_cheight ]

	place $NET_SBH -x $net_cx -y $net_sby \
		-width $net_cwidth -height $scroll_width

	if { $net_hheight < $net_cheight } \
		{ set xview 0 } \
\
	else \
	{
		set xview [ expr $net_xview \
			- ( ( ($net_size / 2) - ($net_hwidth / 2) ) \
				/ $net_scroll ) ]

		if { $xview < 0 } { set xview 0 }
	}

	scrollSet $NET_SBH $net_hwidth $net_cwidth $net_scroll $xview

	update

	#debug_proc layout_net_panel exit
}


proc resize_nk_panel { } \
{
	#debug_proc resize_nk_panel entry

	global nk_height
	global nk_width

	global NK

	set ht [ winfo height $NK ]
	set wt [ winfo width $NK ]

	if { $nk_width != $wt || $nk_height != $ht } \
	{
		set nk_height $ht
		set nk_width $wt

		layout_nk_panel

		update
	}

	#debug_proc resize_nk_panel exit
}


proc layout_nk_panel { } \
{
	#debug_proc layout_nk_panel entry

	global net_volume_color_size
	global border_space
	global frame_border
	global nk_height
	global nk_width

	global NK_F

	#
	# Place Top Titles
	#

	place $NK_F -x 0 -y 0 -width $nk_width -height $nk_height

	set nkx [ expr $border_space / 2 ]
	set nky [ expr $border_space / 2 ]

	place $NK_F.title -relx 0.50 -y $nky -anchor n

	update

	set nky [ below $NK_F.title ]

	place $NK_F.label -x $nkx -y $nky -anchor nw

	update

	set nky [ below $NK_F.label ]

	#
	# Place Close Button
	#

	place $NK_F.close -x $nkx -y [ expr $nk_height - $nkx ] -anchor sw

	update

	set lht [ expr ( ( [ above $NK_F.close ] - $nky ) \
		/ $net_volume_color_size ) - (2 * $frame_border) ]

	#
	# Place Level Labels
	#

	set nkcx 0

	for { set i 0 } { $i < $net_volume_color_size } \
		{ set i [ expr $i + 1 ] } \
	{
		place $NK_F.label$i -x $nkx -y $nky -anchor nw \
			-height $lht

		update

		set wt [ winfo width $NK_F.label$i ]

		if { $wt > $nkcx } \
			{ set nkcx $wt }

		set nky [ expr [ below $NK_F.label$i ] + (2 * $frame_border) ]
	}

	set nkcx [ expr $nkx + $nkcx ]

	set cwt [ expr $nk_width - $nkcx - $border_space ]

	#
	# Place Color Labels
	#

	set nky [ below $NK_F.label ]

	for { set i 0 } { $i < $net_volume_color_size } \
		{ set i [ expr $i + 1 ] } \
	{
		place $NK_F.label$i -x $nkcx -y $nky -anchor ne

		place $NK_F.key$i -x $nkcx -y $nky -anchor nw \
			-width $cwt -height $lht

		set nky [ expr [ below $NK_F.label$i ] + (2 * $frame_border) ]
	}

	#debug_proc layout_nk_panel exit
}


proc layout_st_panel { } \
{
	#debug_proc layout_st_panel entry

	global st_labels_width
	global tasks_height
	global frame_border
	global border_space
	global scroll_width
	global st_label_ht
	global st_cheight
	global st_cwidth
	global st_lwidth
	global st_active
	global st_height
	global st_width
	global st_xview
	global stl_yview
	global timex
	global st_x
	global st_y

	global ST
	global ST_L
	global ST_C
	global ST_SBH
	global ST_SBV

	#
	# Top Titles
	#

	set st_lwidth [ expr $st_labels_width + $scroll_width ]

	place $ST -x $st_x -y $st_y -width $st_width -height $st_height

	set stx $border_space
	set sty [ expr $border_space / 2 ]

	place $ST.title -relx 0.50 -y $sty -anchor n

	#
	# Bottom Buttons
	#

	set kly [ expr $st_height - ($border_space / 2) ]

	place $ST.computing_label -relx 0.10 -y $kly -anchor s
	place $ST.user_defined_label -relx 0.32 -y $kly -anchor s
	place $ST.overhead_label -relx 0.52 -y $kly -anchor s
	place $ST.waiting_label -relx 0.69 -y $kly -anchor s
	place $ST.message_label -relx 0.86 -y $kly -anchor s

	update

	set sz [ winfo height $ST.computing_label ]

	set kky [ expr $kly - ( $sz / 2 ) ]

	place $ST.computing_box \
		-x [ right $ST.computing_label ] -y $kky -anchor w \
		-width $sz -height [ expr $sz / 2 ]

	place $ST.user_defined_box \
		-x [ right $ST.user_defined_label ] -y $kky -anchor w \
		-width $sz -height [ expr $sz / 2 ]

	place $ST.overhead_box \
		-x [ right $ST.overhead_label ] -y $kky -anchor w \
		-width $sz -height [ expr $sz / 2 ]

	place $ST.waiting_box \
		-x [ right $ST.waiting_label ] -y $kky -anchor w \
		-width $sz -height [ expr $sz / 2 ]

	place $ST.message_box \
		-x [ right $ST.message_label ] -y $kky -anchor w \
		-width $sz -height 2

	update

	set stmy [ expr [ above $ST.computing_label ] \
		- ($border_space / 2) ]

	place $ST.query_frame -x $border_space -y $stmy -anchor sw \
		-width [ expr $st_width - (2 * $border_space) ] \
		-height [ expr $sz + (2 * $frame_border) ]

	place $ST.query_frame.query -x 0 -y 0 -anchor nw -height $sz

	update

	set stcby [ expr [ above $ST.query_frame ] - ( $border_space / 2 ) ]

	place $ST.close -x $border_space -y $stcby -anchor sw

	update

	set cbx [ expr $border_space + [ winfo width $ST.close ] ]

	set cbwt [ expr $border_space + $st_lwidth - $cbx ]

	set shrinkx [ expr $cbx + ( $cbwt / 5 ) ]

	place $ST.shrink -x $shrinkx -y $stcby -anchor s

	set growx [ expr $cbx + ( 2 * $cbwt / 5 ) ]

	place $ST.grow -x $growx -y $stcby -anchor s

	update

	set setx [ expr $cbx + ( 3.5 * $cbwt / 5 ) ]

	place $ST.task_height_entry -x $setx -y $stcby -anchor s \
		-width [ expr 2 * [ winfo width $ST.shrink ] ]

	update

	#
	# Canvases & Scrollbars
	#

	set st_cy [ below $ST.title ]

	set st_cheight [ expr [ above $ST.close ] - $st_cy \
		- ($border_space / 2) ]

	set st_cwidth [ expr $st_width - $st_lwidth \
		- (2 * $border_space) ]

	place $ST_SBV -x $stx -y $st_cy \
		-width $scroll_width -height $st_cheight

	scrollSet $ST_SBV $tasks_height $st_cheight $st_label_ht $stl_yview

	set st_lx [ expr $stx + $scroll_width ]

	place $ST_L -x $st_lx -y $st_cy \
		-width $st_labels_width -height $st_cheight

	set st_cx [ expr $st_lx + $st_labels_width ]

	place $ST_C -x $st_cx -y $st_cy \
		-width $st_cwidth -height $st_cheight

	set stsby [ expr $st_cy + $st_cheight ]

	place $ST_SBH -x $st_cx -y $stsby \
		-width $st_cwidth -height $scroll_width

	scrollSet $ST_SBH $timex $st_cwidth 1 $st_xview

	update

	#debug_proc layout_st_panel exit
}


proc layout_spawn_dialog { } \
{
	#debug_proc layout_spawn_dialog entry

	global spawn_flags_trace
	global border_space
	global trace_format
	global spawn_where
	global spawn_width

	global SP

	place $SP.cmd -x $border_space -y $border_space

	update

	set nwt [ expr $spawn_width - [ winfo width $SP.cmd ] ]

	place $SP.cmd_entry \
		-x [ right $SP.cmd ] -y $border_space -width $nwt

	update

	place $SP.flags -x $border_space -y [ below $SP.cmd_entry ]

	update

	set indentx [ expr 2 * $border_space ]

	set try [ below $SP.flags ]

	place $SP.flags_trace -x $indentx -y $try

	update

	place $SP.flags_debug -x $indentx -y [ below $SP.flags_trace ]

	update

	place $SP.flags_front -x $indentx -y [ below $SP.flags_debug ]

	update

	place $SP.flags_compl -x $indentx -y [ below $SP.flags_front ]

	if { $spawn_flags_trace == "ON" } \
	{
		if { $trace_format == "3.3" } \
		{
			set tx [ expr [ right $SP.flags_trace ] \
				+ (3 * $border_space) ]

			place $SP.trace_mask -anchor nw -x $tx -y $try
		} \
\
		elseif { $trace_format == "3.4" } \
		{
			set tx [ expr [ right $SP.flags_trace ] \
				+ (3 * $border_space) ]

			place $SP.buf -anchor nw -x $tx -y $try

			update

			set bewt [ expr $spawn_width - $tx \
				- [ winfo width $SP.buf ] + $border_space ]

			place $SP.buf_entry -anchor nw \
				-x [ right $SP.buf ] -y $try -width $bewt

			update

			place $SP.trace_mask -anchor nw -x $tx \
				-y [ below $SP.buf_entry ]
		}
	} \
\
	else \
	{
		place forget $SP.buf
		place forget $SP.buf_entry
		place forget $SP.trace_mask
	}

	update

	place $SP.where -x $border_space -y [ below $SP.flags_compl ]

	update

	place $SP.where_host -x $indentx -y [ below $SP.where ]

	update

	place $SP.where_arch -x $indentx -y [ below $SP.where_host ]

	update

	place $SP.where_default -x $indentx -y [ below $SP.where_arch ]

	update

	if { $spawn_where != "Default" } \
	{
		set ly [ below $SP.where_default ]

		$SP.loc configure -text $spawn_where

		place $SP.loc -x $border_space -y $ly

		update

		set lwt [ expr $spawn_width - [ winfo width $SP.loc ] ]

		place $SP.loc_entry -x [ right $SP.loc ] -y $ly -width $lwt

		update

		set ny [ below $SP.loc_entry ]
	} \
\
	else \
	{
		place forget $SP.loc

		place forget $SP.loc_entry

		set ny [ below $SP.where_default ]
	}

	place $SP.ntasks -x $border_space -y $ny

	update

	set twt [ expr $spawn_width - [ winfo width $SP.ntasks ] ]

	place $SP.ntasks_entry -x [ right $SP.ntasks ] -y $ny -width $twt

	update

	set y [ below $SP.ntasks_entry ]

	place $SP.close_on_start -x $border_space -y $y

	update

	set y [ below $SP.close_on_start ]

	place $SP.close -x $border_space -y $y

	place $SP.start_append -anchor n -relx 0.50 -y $y

	place $SP.start -anchor ne -x $spawn_width -y $y

	update

	$SP configure \
		-width [ expr $spawn_width + (2 * $border_space) + 8 ] \
		-height [ expr $y + [ winfo height $SP.close ] \
			+ $border_space + 4 ]

	#debug_proc layout_spawn_dialog exit
}


proc layout_otf_dialog { } \
{
	#debug_proc layout_otf_dialog entry

	global tracing_status
	global scroll_width
	global border_space
	global otf_cheight
	global otf_cwidth
	global otf_height
	global otf_width

	global OTF
	global OTF_C
	global OTF_CW
	global OTF_SBH
	global OTF_SBV

	$OTF configure -width $otf_width -height $otf_height

	#
	# Top Titles
	#

	set x $border_space
	set y [ expr $border_space / 2 ]

	set cx [ expr $x + $scroll_width ]

	place $OTF.tasks -x $cx -y $y

	#
	# Trace Settings
	#

	set tx [ expr $otf_width - $border_space ]

	place $OTF.tracing -x $tx -y $y -anchor ne

	update

	place $OTF.trace_on -x $tx -y [ below $OTF.tracing ] -anchor ne

	update

	place $OTF.trace_off -x $tx -y [ below $OTF.trace_on ] -anchor ne

	update

	if { $tracing_status == "On" } \
	{
		place $OTF.trace_mask -anchor ne \
			-x $tx -y [ expr [ below $OTF.trace_off ] + $border_space ]
	} \
\
	else \
		{ place forget $OTF.trace_mask }

	update

	#
	# Bottom Buttons
	#

	set cly [ expr $otf_height - $border_space ]

	place $OTF.close -x $x -y $cly -anchor sw

	place $OTF.refresh -relx 0.40 -y $cly -anchor s

	set kax [ expr $otf_width - $border_space ]

	place $OTF.otf_all -x $kax -y $cly -anchor se

	update

	#
	# Canvas & Scrollbars
	#

	set ly [ below $OTF.tasks ]

	set otf_cheight [ expr [ above $OTF.close ] - $ly \
		- $scroll_width - $border_space ]

	set x1 [ left $OTF.tracing ]
	set x2 [ left $OTF.trace_mask ]

	if { $x1 < $x2 } \
		{ set lx $x1 } \
\
	else \
		{ set lx $x2 }

	set otf_cwidth [ expr $lx - $scroll_width - (2 * $border_space) ]

	place $OTF_SBV -x $x -y $ly \
		-width $scroll_width -height $otf_cheight

	place $OTF_C -x $cx -y $ly \
		-width $otf_cwidth -height $otf_cheight

	set sby [ expr $ly + $otf_cheight ]

	place $OTF_SBH -x $cx -y $sby \
		-width $otf_cwidth -height $scroll_width

	update

	#debug_proc layout_otf_dialog exit
}


proc layout_kill_dialog { } \
{
	#debug_proc layout_kill_dialog entry

	global scroll_width
	global border_space
	global kill_cheight
	global kill_cwidth
	global kill_height
	global kill_width

	global KL
	global KL_C
	global KL_CW
	global KL_SBH
	global KL_SBV

	$KL configure -width $kill_width -height $kill_height

	#
	# Top Titles
	#

	set x $border_space
	set y [ expr $border_space / 2 ]

	set cx [ expr $x + $scroll_width ]

	place $KL.tasks -x $cx -y $y

	#
	# Bottom Buttons
	#

	set cly [ expr $kill_height - $border_space ]

	place $KL.close -x $x -y $cly -anchor sw

	place $KL.refresh -relx 0.50 -y $cly -anchor s

	set kax [ expr $kill_width - $border_space ]

	place $KL.kill_all -x $kax -y $cly -anchor se

	update

	#
	# Canvas & Scrollbars
	#

	set ly [ below $KL.tasks ]

	set kill_cheight [ expr [ above $KL.close ] - $ly \
		- $scroll_width - $border_space ]

	set kill_cwidth [ expr $kill_width - $scroll_width \
		- (2 * $border_space)]

	place $KL_SBV -x $x -y $ly \
		-width $scroll_width -height $kill_cheight

	place $KL_C -x $cx -y $ly \
		-width $kill_cwidth -height $kill_cheight

	set sby [ expr $ly + $kill_cheight ]

	place $KL_SBH -x $cx -y $sby \
		-width $kill_cwidth -height $scroll_width

	update

	#debug_proc layout_kill_dialog exit
}


proc signal_button { name index } \
{
	#debug_proc signal_button entry

	global active_fg_color
	global selector_color
	global frame_border
	global fg_color

	global SELECTOR

	global SG_LFCW

	radiobutton $SG_LFCW.$name -text $index -value $index \
		-padx 1 -pady 1 \
		-bd $frame_border -relief flat \
		-variable signal_selected \
		$SELECTOR $selector_color -foreground $fg_color \
		-activeforeground $active_fg_color

	restrict_bindings $SG_LFCW.$name ""

	butt_help $SG_LFCW.$name button "Select Unix Signal $index"

	#debug_proc signal_button exit
}


proc layout_signal_dialog { } \
{
	#debug_proc layout_signal_dialog entry

	global signal_canvas_list_win
	global signal_list_height
	global signal_list_width
	global signal_lcheight
	global signal_cheight
	global signal_cwidth
	global signal_height
	global signal_width
	global scroll_width
	global border_space
	global frame_border

	global FRAME_OFFSET

	global SG
	global SG_C
	global SG_CW
	global SG_SBH
	global SG_SBV

	global SG_LF
	global SG_LFC
	global SG_LFCW
	global SG_LSBV

	$SG configure -width $signal_width -height $signal_height

	#
	# Top Titles
	#

	set x $border_space
	set y [ expr $border_space / 2 ]

	set cx [ expr $x + $scroll_width ]

	place $SG.tasks -x $cx -y $y

	#
	# Bottom Buttons
	#

	set cly [ expr $signal_height - $border_space ]

	place $SG.close -x $x -y $cly -anchor sw

	place $SG.refresh -relx 0.45 -y $cly -anchor s

	set sax [ expr $signal_width - $border_space ]

	place $SG.signal_all -x $sax -y $cly -anchor se

	update

	#
	# Canvases & Scrollbars
	#

	set ly [ below $SG.tasks ]

	set signal_cheight [ expr [ above $SG.close ] - $ly \
		- $scroll_width - $border_space ]

	set signal_cwidth [ expr $signal_width - (2 * $scroll_width) \
		- $signal_list_width - (3 * $border_space)]

	place $SG_SBV -x $x -y $ly \
		-width $scroll_width -height $signal_cheight

	place $SG_C -x $cx -y $ly \
		-width $signal_cwidth -height $signal_cheight

	set lx [ expr $cx + $signal_cwidth + $border_space ]

	set signal_lcheight [ expr $signal_cheight + $scroll_width ]

	place $SG_LSBV -x $lx -y $ly \
		-width $scroll_width -height $signal_lcheight

	set lcx [ expr $lx + $scroll_width ]

	#
	# Signals Column
	#

	place $SG.signals -x $lcx -y $y

	set fo [ expr $FRAME_OFFSET * $frame_border ]

	place $SG_LF \
		-x [ expr $lcx + $fo ] -y [ expr $ly + $fo ] \
		-width [ expr $signal_list_width - (2 * $fo) ] \
		-height [ expr $signal_lcheight - (2 * $fo) ]

	place $SG_LFC -x 0 -y 0 \
		-width [ expr $signal_list_width - (2 * $frame_border) \
			- (2 * $fo) ] \
		-height [ expr $signal_lcheight - (2 * $frame_border) \
			- (2 * $fo) ]

	set bx $frame_border

	place $SG_LFCW.hup	  -x $bx -y $frame_border ; update
	place $SG_LFCW.int	  -x $bx -y [ below $SG_LFCW.hup ] ;	update
	place $SG_LFCW.quit	  -x $bx -y [ below $SG_LFCW.int ] ;	update
	place $SG_LFCW.ill	  -x $bx -y [ below $SG_LFCW.quit ] ;	update
	place $SG_LFCW.trap	  -x $bx -y [ below $SG_LFCW.ill ] ;	update
	place $SG_LFCW.abrt	  -x $bx -y [ below $SG_LFCW.trap ] ;	update
	place $SG_LFCW.emt	  -x $bx -y [ below $SG_LFCW.abrt ] ;	update
	place $SG_LFCW.fpe	  -x $bx -y [ below $SG_LFCW.emt ] ;	update
	place $SG_LFCW.kill	  -x $bx -y [ below $SG_LFCW.fpe ] ;	update
	place $SG_LFCW.bus	  -x $bx -y [ below $SG_LFCW.kill ] ;	update
	place $SG_LFCW.segv	  -x $bx -y [ below $SG_LFCW.bus ] ;	update
	place $SG_LFCW.sys	  -x $bx -y [ below $SG_LFCW.segv ] ;	update
	place $SG_LFCW.pipe	  -x $bx -y [ below $SG_LFCW.sys ] ;	update
	place $SG_LFCW.alrm	  -x $bx -y [ below $SG_LFCW.pipe ] ;	update
	place $SG_LFCW.term	  -x $bx -y [ below $SG_LFCW.alrm ] ;	update
	place $SG_LFCW.urg	  -x $bx -y [ below $SG_LFCW.term ] ;	update
	place $SG_LFCW.stop	  -x $bx -y [ below $SG_LFCW.urg ] ;	update
	place $SG_LFCW.tstp	  -x $bx -y [ below $SG_LFCW.stop ] ;	update
	place $SG_LFCW.cont	  -x $bx -y [ below $SG_LFCW.tstp ] ;	update
	place $SG_LFCW.chld	  -x $bx -y [ below $SG_LFCW.cont ] ;	update
	place $SG_LFCW.ttin	  -x $bx -y [ below $SG_LFCW.chld ] ;	update
	place $SG_LFCW.ttou	  -x $bx -y [ below $SG_LFCW.ttin ] ;	update
	place $SG_LFCW.io	  -x $bx -y [ below $SG_LFCW.ttou ] ;	update
	place $SG_LFCW.xcpu	  -x $bx -y [ below $SG_LFCW.io ] ;		update
	place $SG_LFCW.xfsz	  -x $bx -y [ below $SG_LFCW.xcpu ] ;	update
	place $SG_LFCW.vtalrm -x $bx -y [ below $SG_LFCW.xfsz ] ;	update
	place $SG_LFCW.prof	  -x $bx -y [ below $SG_LFCW.vtalrm ] ; update
	place $SG_LFCW.winch  -x $bx -y [ below $SG_LFCW.prof ] ;	update
	place $SG_LFCW.lost	  -x $bx -y [ below $SG_LFCW.winch ] ;  update
	place $SG_LFCW.usr1	  -x $bx -y [ below $SG_LFCW.lost ] ;	update
	place $SG_LFCW.usr2	  -x $bx -y [ below $SG_LFCW.usr1 ] ;	update

	set signal_list_height [ expr [ below $SG_LFCW.usr2 ] \
		+ $frame_border ]

	$SG_LFC itemconfigure $signal_canvas_list_win -window $SG_LFCW \
		-height $signal_list_height

	set sby [ expr $ly + $signal_cheight ]

	place $SG_SBH -x $cx -y $sby \
		-width $signal_cwidth -height $scroll_width

	update

	#debug_proc layout_signal_dialog exit
}


proc resize_help_win { index } \
{
	#debug_proc resize_help_win entry

	global min_help_height

	global ${index}_height
	global ${index}_width

	set WIN .${index}_help_win

	set ht [ winfo height $WIN ]
	set wt [ winfo width $WIN ]

	if { [ set ${index}_width ] != $wt \
		|| [ set ${index}_height ] != $ht } \
	{
		set ${index}_height $ht
		set ${index}_width $wt

		layout_help_win $index
	}

	#debug_proc resize_help_win exit
}


proc layout_help_win { index } \
{
	#debug_proc layout_help_win entry

	global border_space
	global scroll_width
	global row_height

	global ${index}_canvas_height
	global ${index}_canvas_width
	global ${index}_cheight
	global ${index}_cwidth
	global ${index}_height
	global ${index}_width
	global ${index}_xview
	global ${index}_yview

	set WIN .${index}_help_win

	set WIN_C $WIN.canvas

	set WIN_SBH $WIN.horiz_sb
	set WIN_SBV $WIN.vert_sb

	#
	# Top Titles
	#

	place $WIN.title -relx 0.50 -y 0 -anchor n

	#
	# Bottom Buttons
	#

	set cly [ expr [ set ${index}_height ] - ($border_space / 2) ]

	place $WIN.close -x $border_space -y $cly -anchor sw

	update

	#
	# Canvas & ScrollBars
	#

	set cy [ below $WIN.title ]

	set ${index}_cheight [ expr [ above $WIN.close ] - $cy \
		- $scroll_width - ($border_space / 2 ) ]

	set ${index}_cwidth [ expr [ set ${index}_width ] - $scroll_width \
		- (2 * $border_space)]

	set sbx $border_space

	place $WIN_SBV -x $sbx -y $cy \
		-width $scroll_width -height [ set ${index}_cheight ]

	scrollSet $WIN_SBV \
		[ set ${index}_canvas_height ] [ set ${index}_cheight ] \
		$row_height [ set ${index}_yview ]

	set cx [ expr $sbx + $scroll_width ]

	place $WIN_C -x $cx -y $cy \
		-width [ set ${index}_cwidth ] \
		-height [ set ${index}_cheight ]

	set sby [ expr $cy + [ set ${index}_cheight ] ]

	place $WIN_SBH -x $cx -y $sby \
		-width [ set ${index}_cwidth ] -height $scroll_width

	scrollSet $WIN_SBH \
		[ set ${index}_canvas_width ] [ set ${index}_cwidth ] \
		$row_height [ set ${index}_xview ]

	update

	#debug_proc layout_help_win exit
}


proc fix_menus_resize { } \
{
	#debug_proc fix_menus_resize entry

	replaceMenu .xpvm.hosts_menu .xpvm.hosts .xpvm

	replaceMenu .xpvm.tasks_menu .xpvm.tasks .xpvm

	replaceMenu .xpvm.views_menu .xpvm.views .xpvm

	replaceMenu .xpvm.reset_menu .xpvm.reset .xpvm

	replaceMenu .xpvm.help_menu .xpvm.help .xpvm
	replaceMenu .xpvm.tasks_help_menu .xpvm.help .xpvm
	replaceMenu .xpvm.views_help_menu .xpvm.help .xpvm

	update

	replaceSubMenu .xpvm.spawn .xpvm.tasks_menu \
		.xpvm.tasks_menu.butt_SPAWN .xpvm.spawn.cmd_entry

	replaceSubMenu .xpvm.systasks_menu .xpvm.tasks_menu \
		.xpvm.tasks_menu.butt_SYS_TASKS none

	update

	replaceMenu .xpvm.trace_mask_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.host_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.group_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.task_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.comm_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.buff_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.pack_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.unpack_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.info_ev_menu .xpvm.spawn.trace_mask .xpvm
	replaceMenu .xpvm.misc_ev_menu .xpvm.spawn.trace_mask .xpvm

	#debug_proc fix_menus_resize exit
}


proc resize_ut_panel { } \
{
	#debug_proc resize_ut_panel entry

	global ut_height
	global ut_width

	global UT

	set ht [ winfo height $UT ]
	set wt [ winfo width $UT ]

	if { $ut_width != $wt || $ut_height != $ht } \
	{
		set ut_height $ht
		set ut_width $wt

		layout_ut_panel

		utAdjustHeight

		update
	}

	#debug_proc resize_ut_panel exit
}


proc layout_ut_panel { } \
{
	#debug_proc layout_ut_panel entry

	global border_space
	global scroll_width
	global ut_cheight
	global ut_cwidth
	global col_width
	global ut_height
	global ut_width
	global ut_xview
	global timex

	global UT_F
	global UT_C
	global UT_SBH

	#
	# Place Top Titles
	#

	place $UT_F -x 0 -y 0 -width $ut_width -height $ut_height

	set utx [ expr $border_space / 2 ]
	set uty [ expr $border_space / 2 ]

	place $UT_F.title -relx 0.50 -y $uty -anchor n

	update

	set utcy [ below $UT_F.title ]

	place $UT_F.xN -x $utx -y $utcy

	update

	place $UT_F.xT -x $utx -y [ below $UT_F.xN ]

	update

	place $UT_F.xA -x $utx -y [ below $UT_F.xT ]

	update

	place $UT_F.xS -x $utx -y [ below $UT_F.xA ]

	update

	place $UT_F.xK -x $utx -y [ below $UT_F.xS ]

	update

	place $UT_F.xX -x $utx -y [ below $UT_F.xK ]

	#
	# Place Bottom Buttons
	#

	set kly [ expr $ut_height - ($border_space / 2) ]

	place $UT_F.close -x $border_space -y $kly -anchor sw

	set kky [ expr $kly - 2 ]

	place $UT_F.computing_label -relx 0.32 -y $kky -anchor s
	place $UT_F.overhead_label -relx 0.60 -y $kky -anchor s
	place $UT_F.waiting_label -relx 0.85 -y $kky -anchor s

	update

	set kht [ winfo height $UT_F.computing_label ]
	set kwt [ expr $kht / 2 ]

	place $UT_F.computing_box \
		-x [ right $UT_F.computing_label ] -y $kky -anchor sw \
		-width $kwt -height $kht

	place $UT_F.overhead_box \
		-x [ right $UT_F.overhead_label ] -y $kky -anchor sw \
		-width $kwt -height $kht

	place $UT_F.waiting_box \
		-x [ right $UT_F.waiting_label ] -y $kky -anchor sw \
		-width $kwt -height $kht

	update

	#
	# Place Canvas & Scrollbar
	#

	set ut_cheight [ expr [ above $UT_F.close ] - $utcy \
		- $scroll_width - ($border_space / 2) ]

	set ut_cwidth [ expr $ut_width \
		- (4 * $col_width) - (2 * $border_space) ]

	set utcx [ expr $utx + ($border_space / 2) + (4 * $col_width) ]

	place $UT_F.ntasks -x $utcx -y $utcy -anchor ne

	place $UT_F.tick \
		-x $utcx -y [ expr $utcy + ($ut_cheight / 2) ] -anchor e \
		-width 8 -height 2

	place $UT_F.zero -x $utcx -y [ expr $utcy + $ut_cheight ] -anchor se

	place $UT_C -x $utcx -y $utcy -width $ut_cwidth -height $ut_cheight

	set utsby [ expr $utcy + $ut_cheight ]

	place $UT_SBH -x $utcx -y $utsby \
		-width $ut_cwidth -height $scroll_width

	scrollSet $UT_SBH $timex $ut_cwidth 1 $ut_xview

	update

	#debug_proc layout_ut_panel exit
}


proc resize_mq_panel { } \
{
	#debug_proc resize_mq_panel entry

	global mq_height
	global mq_width

	global MQ

	set ht [ winfo height $MQ ]
	set wt [ winfo width $MQ ]

	if { $mq_width != $wt || $mq_height != $ht } \
	{
		set mq_height $ht
		set mq_width $wt

		layout_mq_panel

		mqArrange
	}

	#debug_proc resize_mq_panel exit
}


proc layout_mq_panel { } \
{
	#debug_proc layout_mq_panel entry

	global mq_tasks_width
	global frame_border
	global border_space
	global scroll_width
	global mq_task_wt
	global mq_cheight
	global mq_cwidth
	global col_width
	global mq_height
	global mq_width
	global mq_xview

	global MQ_F
	global MQ_C
	global MQ_SBH

	#
	# Top Titles
	#

	place $MQ_F -x 0 -y 0 -width $mq_width -height $mq_height

	set mqx [ expr $border_space + (4 * $col_width) ]
	set mqy [ expr $border_space / 2 ]

	place $MQ_F.title -x $mqx -y $mqy -anchor nw

	update

	set mqlx [ expr $border_space / 2 ]

	set mqcy [ below $MQ_F.title ]

	place $MQ_F.xN -x $mqlx -y $mqcy -anchor nw

	update

	place $MQ_F.xB -x $mqlx -y [ below $MQ_F.xN ] -anchor nw

	update

	place $MQ_F.xY -x $mqlx -y [ below $MQ_F.xB ] -anchor nw

	update

	place $MQ_F.xT -x $mqlx -y [ below $MQ_F.xY ] -anchor nw

	update

	place $MQ_F.xE -x $mqlx -y [ below $MQ_F.xT ] -anchor nw

	update

	place $MQ_F.xS -x $mqlx -y [ below $MQ_F.xE ] -anchor nw

	#
	# Bottom Buttons
	#

	set mqclx $border_space
	set mqcly [ expr $mq_height - $border_space ]

	place $MQ_F.close -x $mqclx -y $mqcly -anchor sw

	update

	set mqqwt [ expr $mq_width - [ winfo width $MQ_F.close ] \
		- (3 * $border_space) ]

	set sz [ winfo height $MQ_F.title ]

	place $MQ_F.query_frame \
		-x [ expr [ right $MQ_F.close ] + $border_space ] -y $mqcly \
		-anchor sw \
		-width $mqqwt -height [ expr $sz + (2 * $frame_border) ]

	place $MQ_F.query_frame.query -x 0 -y 0 -anchor nw -height $sz

	update

	#
	# Canvas & Scrollbar
	#

	set mq_cheight [ expr [ above $MQ_F.close ] - $mqcy \
		- $scroll_width - ($border_space / 2) ]

	set mq_cwidth [ expr $mq_width - (4 * $col_width) \
		- (2 * $border_space) ]

	place $MQ_C -x $mqx -y $mqcy -anchor nw \
		-width $mq_cwidth -height $mq_cheight

	place $MQ_F.nbytes -anchor ne -x $mqx -y $mqcy

	place $MQ_F.tick -anchor e \
		-x $mqx -y [ expr $mqcy + ($mq_cheight / 2) ] \
		-width 8 -height 2

	place $MQ_F.zero -anchor se -x $mqx -y [ expr $mqcy + $mq_cheight ]

	set mqsby [ expr $mqcy + $mq_cheight ]

	place $MQ_SBH -x $mqx -y $mqsby \
		-width $mq_cwidth -height $scroll_width

	scrollSet $MQ_SBH $mq_tasks_width $mq_cwidth $mq_task_wt $mq_xview

	update

	#debug_proc layout_mq_panel exit
}


proc resize_ct_panel { } \
{
	#debug_proc resize_ct_panel entry

	global ct_height
	global ct_width

	global CT

	set ht [ winfo height $CT ]
	set wt [ winfo width $CT ]

	if { $ct_width != $wt || $ct_height != $ht } \
	{
		set ct_height $ht
		set ct_width $wt

		layout_ct_panel
	}

	#debug_proc resize_ct_panel exit
}


proc layout_ct_panel { } \
{
	#debug_proc layout_ct_panel entry

	global ct_canvas_width
	global ct_labels_width
	global border_space
	global scroll_width
	global tasks_height
	global st_label_ht
	global ct_cheight
	global ct_cwidth
	global ct_lwidth
	global ct_height
	global ct_width
	global ct_xview
	global ct_yview

	global CT_F
	global CT_L
	global CT_C
	global CT_SBH
	global CT_SBV

	#
	# Top Titles
	#

	place $CT_F -x 0 -y 0 -width $ct_width -height $ct_height

	set ctx $border_space
	set cty [ expr $border_space / 2 ]

	place $CT_F.title -x $ctx -y $cty -anchor nw

	update

	set qwt [ expr $ct_width - [ winfo width $CT_F.title ] \
		- ( 2 * $border_space ) ]

	place $CT_F.query -x [ right $CT_F.title ] -y $cty -anchor nw \
		-width $qwt

	#
	# Bottom Button
	#

	set ctcly [ expr $ct_height - ($border_space / 2) ]

	place $CT_F.close -x $ctx -y $ctcly -anchor sw

	update

	#
	# Canvas & Scrollbars
	#

	set cty [ below $CT_F.query ]

	set ct_cheight [ expr [ above $CT_F.close ] - $cty \
		- ($border_space / 2) ]

	set ct_lwidth [ expr $ct_labels_width + $scroll_width ]

	set ct_cwidth [ expr $ct_width - $ct_lwidth \
		- (2 * $border_space) ]

	place $CT_SBV -x $ctx -y $cty \
		-width $scroll_width -height $ct_cheight

	scrollSet $CT_SBV $tasks_height $ct_cheight $st_label_ht $ct_yview

	set ctlx [ expr $ctx + $scroll_width ]

	place $CT_L -x $ctlx -y $cty \
		-width $ct_labels_width -height $ct_cheight

	set ctcx [ expr $ctlx + $ct_labels_width ]

	place $CT_C -x $ctcx -y $cty \
		-width $ct_cwidth -height $ct_cheight

	set ctsby [ expr $cty + $ct_cheight ]

	place $CT_SBH -x $ctcx -y $ctsby \
		-width $ct_cwidth -height $scroll_width

	scrollSet $CT_SBH $ct_canvas_width $ct_cwidth 1 $ct_xview

	update

	#debug_proc layout_ct_panel exit
}


proc resize_to_panel { } \
{
	#debug_proc resize_to_panel entry

	global to_height
	global to_width

	global TO

	set ht [ winfo height $TO ]
	set wt [ winfo width $TO ]

	if { $to_width != $wt || $to_height != $ht } \
	{
		set to_height $ht
		set to_width $wt

		layout_to_panel
	}

	#debug_proc resize_to_panel exit
}


proc layout_to_panel { } \
{
	#debug_proc layout_to_panel entry

	global to_canvas_width
	global border_space
	global scroll_width
	global frame_border
	global row_height
	global to_cheight
	global to_cwidth
	global to_height
	global to_lasty
	global to_width
	global to_xview
	global to_yview

	global TO_F
	global TO_C
	global TO_SBH
	global TO_SBV

	#
	# Top Titles
	#

	place $TO_F -x 0 -y 0 -width $to_width -height $to_height

	set tox $border_space
	set toy [ expr $border_space / 2 ]

	place $TO_F.title \
		-x $tox -y [ expr $toy + $frame_border ] -anchor nw

	update

	place $TO_F.file_entry -x [ right $TO_F.title ] -y $toy \
		-anchor nw \
		-width [ expr $to_width - [ winfo width $TO_F.title ] \
			- (2 * $border_space) ]

	update

	set toy [ below $TO_F.file_entry ]

	place $TO_F.filter_label \
		-x $tox -y [ expr $toy + $frame_border ] -anchor nw

	update

	place $TO_F.filter_entry -x [ right $TO_F.filter_label ] -y $toy \
		-anchor nw \
		-width [ expr $to_width \
			- [ winfo width $TO_F.filter_label ] \
			- (2 * $border_space) ]

	update

	set toy [ below $TO_F.filter_entry ]

	place $TO_F.search_dir -x $tox -y $toy -anchor nw

	update

	place $TO_F.search_button \
		-x [ right $TO_F.search_dir ] \
		-y [ expr $toy + $frame_border ] \
		-anchor nw

	update

	place $TO_F.search_entry -x [ right $TO_F.search_button ] -y $toy \
		-anchor nw \
		-width [ expr $to_width \
			- [ right $TO_F.search_button ] \
			- $border_space ]

	#
	# Bottom Button
	#

	set tocly [ expr $to_height - ($border_space / 2) ]

	place $TO_F.close -x $tox -y $tocly -anchor sw

	update

	#
	# Canvas & Scrollbars
	#

	set tocy [ below $TO_F.search_dir ]

	set to_cheight [ expr [ above $TO_F.close ] - $tocy \
		- $scroll_width - ($border_space / 2) ]

	set to_cwidth [ expr $to_width - $scroll_width \
		- (2 * $border_space) ]

	place $TO_SBV -x $tox -y $tocy \
		-width $scroll_width -height $to_cheight

	scrollSet $TO_SBV $to_lasty $to_cheight $row_height $to_yview

	set tocx [ expr $tox + $scroll_width ]

	place $TO_C -x $tocx -y $tocy \
		-width $to_cwidth -height $to_cheight

	set tosby [ expr $tocy + $to_cheight ]

	place $TO_SBH -x $tocx -y $tosby \
		-width $to_cwidth -height $scroll_width

	scrollSet $TO_SBH $to_canvas_width $to_cwidth $row_height $to_xview

	update

	#debug_proc layout_to_panel exit
}


proc resize_tv_panel { } \
{
	#debug_proc resize_tv_panel entry

	global tv_height
	global tv_width

	global TV

	set ht [ winfo height $TV ]
	set wt [ winfo width $TV ]

	if { $tv_width != $wt || $tv_height != $ht } \
	{
		set tv_height $ht
		set tv_width $wt

		layout_tv_panel
	}

	#debug_proc resize_tv_panel exit
}


proc layout_tv_panel { } \
{
	#debug_proc layout_tv_panel entry

	global tv_canvas_width
	global border_space
	global scroll_width
	global frame_border
	global row_height
	global tv_cheight
	global tv_cwidth
	global tv_height
	global tv_lasty
	global tv_width
	global tv_xview
	global tv_yview

	global TV_F
	global TV_C
	global TV_SBH
	global TV_SBV

	#
	# Top Titles
	#

	place $TV_F -x 0 -y 0 -width $tv_width -height $tv_height

	set tvx $border_space
	set tvy [ expr $border_space / 2 ]

	place $TV_F.title \
		-x $tvx -y [ expr $tvy + $frame_border ] -anchor nw

	update

	set tvy [ below $TV_F.title ]

	place $TV_F.filter_label \
		-x $tvx -y [ expr $tvy + $frame_border ] -anchor nw

	update

	place $TV_F.filter_entry -x [ right $TV_F.filter_label ] -y $tvy \
		-anchor nw \
		-width [ expr $tv_width \
			- [ winfo width $TV_F.filter_label ] \
			- (2 * $border_space) ]

	update

	set tvy [ below $TV_F.filter_entry ]

	place $TV_F.search_dir -x $tvx -y $tvy -anchor nw

	update

	place $TV_F.search_button \
		-x [ right $TV_F.search_dir ] \
		-y [ expr $tvy + $frame_border ] \
		-anchor nw

	update

	place $TV_F.search_entry -x [ right $TV_F.search_button ] -y $tvy \
		-anchor nw \
		-width [ expr $tv_width \
			- [ right $TV_F.search_button ] \
			- $border_space ]

	#
	# Bottom Button
	#

	set tvcly [ expr $tv_height - ($border_space / 2) ]

	place $TV_F.close -x $tvx -y $tvcly -anchor sw

	update

	#
	# Canvas & Scrollbars
	#

	set tvcy [ below $TV_F.search_dir ]

	set tv_cheight [ expr [ above $TV_F.close ] - $tvcy \
		- $scroll_width - ($border_space / 2) ]

	set tv_cwidth [ expr $tv_width - $scroll_width \
		- (2 * $border_space) ]

	place $TV_SBV -x $tvx -y $tvcy \
		-width $scroll_width -height $tv_cheight

	scrollSet $TV_SBV $tv_lasty $tv_cheight $row_height $tv_yview

	set tvcx [ expr $tvx + $scroll_width ]

	place $TV_C -x $tvcx -y $tvcy \
		-width $tv_cwidth -height $tv_cheight

	set tvsby [ expr $tvcy + $tv_cheight ]

	place $TV_SBH -x $tvcx -y $tvsby \
		-width $tv_cwidth -height $scroll_width

	scrollSet $TV_SBH $tv_canvas_width $tv_cwidth $row_height $tv_xview

	update

	#debug_proc layout_tv_panel exit
}

