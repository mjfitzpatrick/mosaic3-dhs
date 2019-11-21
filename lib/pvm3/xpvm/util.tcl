#
# $Id: util.tcl,v 4.50 1998/04/09 21:12:50 kohl Exp $
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

puts -nonewline "\[util.tcl\]"
flush stdout

#
# XPVM TCL Utility Procs
#

proc scrollInit \
	{ canvas sb max_size canvas_size win_size vvar orient loc incr } \
{
	#debug_proc scrollInit entry

	global $vvar

	if { $loc == "low" } \
	{
		set view 0

		set scroll_view 0
	} \
\
	elseif { $loc == "center" } \
	{
		set view [ scrollCenterValue $max_size $win_size $incr ]

		if { $canvas_size < $win_size } \
			{ set scroll_view 0 } \
\
		else \
		{
			set scroll_view [ expr $view \
				- [ scrollCenterValue $max_size $canvas_size $incr ] ]
		}
	} \
\
	elseif { $loc == "high" } \
	{
		set view [ expr ( $max_size - $win_size ) / $incr ]

		if { $canvas_size < $win_size } \
			{ set scroll_view 0 } \
\
		else \
			{ set scroll_view $view }
	}

	scrollSet $sb $canvas_size $win_size $incr $scroll_view

	if { $orient == "HORIZ" } \
	{
		do_xview $canvas $view

		set $vvar $view
	} \
\
	else \
	{
		do_yview $canvas $view

		set $vvar $view
	}

	#debug_proc scrollInit exit
}

proc scrollSet { sb canvas_size win_size incr first } \
{
	#debug_proc scrollSet entry

	set csize [ expr ( $canvas_size + ($incr - 1) ) / $incr ]

	set wsize [ expr $win_size / $incr ]

	set last [ expr $first + $wsize - 1 ]

	$sb set $csize $wsize $first $last

	#debug_proc scrollSet exit
}

proc rescrollViews { view } \
{
	#debug_proc rescrollViews entry

	global st_xview
	global ut_xview

	if { $view == "ST" } \
	{
		scrollViews $view 1 $st_xview
	} \
\
	elseif { $view == "UT" } \
	{
		scrollViews $view 1 $ut_xview
	}

	#debug_proc rescrollViews exit
}

proc scrollViews { view incr value } \
{
	#debug_proc scrollViews entry

	global ST_SBH
	global UT_SBH
	global ST_C
	global UT_C

	global value_factor
	global last_value

	global frame_border
	global st_active
	global ut_active
	global st_cwidth
	global ut_cwidth
	global tmp_csize
	global timex

	global FRAME_OFFSET

	set fo [ expr $FRAME_OFFSET * $frame_border ]

	set tmp_csize [ expr $timex + ( 2 * ( $frame_border + $fo ) ) ]

	#
	# Process Scroll Acceleration
	#

	set diff [ expr $value - $last_value ]

	if { $diff == 1 || $diff == -1 } \
	{
		set dblclk [ double_click ]

		if { $dblclk == 1 } \
		{
			set value_factor [ expr $value_factor * 2 ]

			if { $value_factor > 40 } \
				{ set value_factor 40 }

			set value [ expr $value + ( $diff * $value_factor ) ]
		} \
\
		else \
			{ set value_factor 1 }
	}

	set last_value $value

	#
	# Note: Really need to set canvas view separately from
	# scrollbar in this case, as new TCL/TK causes major
	# hassle with extra frame border.
	#
	# $fo should be included in canvas view, but not in scrollSet{}.
	#

	if { $view == "ST" } \
	{
		scrollCanvas $ST_C $ST_SBH -1 tmp_csize st_cwidth st_xview \
			HORIZ low $incr $value

		if { $ut_active == "TRUE" } \
		{
			set utxv [ expr $value - \
				( $value * ( $ut_cwidth - $st_cwidth) \
					/ ( $tmp_csize - $st_cwidth ) ) ]

			scrollCanvas $UT_C $UT_SBH -1 tmp_csize ut_cwidth ut_xview \
				HORIZ low $incr $utxv
		}
	} \
\
	elseif { $view == "UT" } \
	{
		scrollCanvas $UT_C $UT_SBH -1 tmp_csize ut_cwidth ut_xview \
			HORIZ low $incr $value

		if { $st_active == "TRUE" } \
		{
			set stxv [ expr $value + \
				( $value * ( $ut_cwidth - $st_cwidth) \
					/ ( $tmp_csize - $ut_cwidth ) ) ]

			scrollCanvas $ST_C $ST_SBH -1 tmp_csize st_cwidth st_xview \
				HORIZ low $incr $stxv
		}
	}

	#debug_proc scrollViews exit
}

proc scrollTwoCanvases \
	{ c1 c2 sb1 max csize wsize vvar1 vvar2 orient loc \
		incr1 incr2 value } \
{
	#debug_proc scrollTwoCanvases entry

	scrollCanvas $c1 $sb1 $max $csize $wsize $vvar1 $orient $loc \
		$incr1 $value

	set fix_csize 0

	if { $incr1 != $incr2 } \
	{
		global $csize
		global $wsize

		set cs [ set $csize ]
		set ws [ set $wsize ]

		set maxvalue1 [ scrollMaxValue $cs $ws $incr1 ]

		if { $value >= $maxvalue1 } \
			{ set value2 [ expr ( $maxvalue1 * $incr1 ) / $incr2 ] } \
\
		else \
			{ set value2 [ expr ( $value * $incr1 ) / $incr2 ] }

		set maxvalue2 [ scrollMaxValue $cs $ws $incr2 ]

		if { $value2 > $maxvalue2 } \
		{
			set tmp [ expr $cs + 1 ]

			while { $value2 > [ scrollMaxValue $tmp $ws $incr2 ] } \
				{ set tmp [ expr $tmp + 1 ] }

			set $csize $tmp

			set fix_csize 1
		}
	} \
\
	else \
		{ set value2 $value }

	scrollCanvas $c2 none $max $csize $wsize $vvar2 $orient $loc \
		$incr2 $value2

	if { $fix_csize == 1 } \
		{ set $csize $cs }

	#debug_proc scrollTwoCanvases exit
}

proc scrollCanvas \
	{ canvas sb max_size canvas_size win_size vvar orient loc \
		incr value } \
{
	#debug_proc scrollCanvas entry

	global $canvas_size
	global $win_size
	global $vvar

	#
	# Limit Scroll Value
	#

	set csize [ set $canvas_size ]

	set wsize [ set $win_size ]

	if { $value < 0 } { set value 0 }

	set maxvalue [ scrollMaxValue $csize $wsize $incr ]

	if { $maxvalue > 0 } \
	{
		if { $value > $maxvalue } \
			{ set value $maxvalue }

		set scroll_value $value

		if { $loc == "center" } \
		{
			set value [ expr $value \
				+ [ scrollCenterValue $max_size $csize $incr ] ]
		} \
\
		elseif { $loc == "high" } \
		{
			set value [ expr ( ( $max_size - $wsize ) / $incr ) \
				- $value ]
		}
	} \
\
	else \
	{
		if { $loc == "low" } \
			{ set value 0 } \
\
		elseif { $loc == "center" } \
			{ set value [ scrollCenterValue $max_size $wsize $incr ] } \
\
		elseif { $loc == "high" } \
			{ set value [ expr ( $max_size - $wsize ) / $incr ] }

		set scroll_value 0
	}

	#
	# Set Scrollbar
	#

	if { $sb != "none" } \
		{ scrollSet $sb $csize $wsize $incr $scroll_value }

	#
	# Scroll Canvas View
	#

	if { $orient == "HORIZ" } \
	{
		do_xview $canvas $value

		set $vvar $value
	} \
\
	else \
	{
		do_yview $canvas $value

		set $vvar $value
	}

	#debug_proc scrollCanvas exit
}

proc scrollMaxValue { csize wsize incr } \
{
	global FRAME_OFFSET

	global frame_border

	set fo [ expr $FRAME_OFFSET * $frame_border ]

	return [ expr ( ( $csize + ($incr - 1) ) / $incr ) \
		- ( ( $wsize - $fo ) / $incr ) ]
}

proc scrollCenterValue { max_size wsize incr } \
{
	return [ expr ( ($max_size / 2) - ($wsize / 2) ) / $incr ]
}

proc makeMenu { name cmd info_list close leavesubs prefix } \
{
	#debug_proc makeMenu entry

	global active_fg_color
	global selector_color
	global frame_border
	global fg_color

	global SELECTOR

	frame $name -bd $frame_border -relief raised

	restrict_bindings $name "Frame"

	set root [ rootName $name ]

	set mil "${root}_list"

	global $mil

	set $mil ""

	set LAST none

	set width 0

	foreach i $info_list \
	{
		set label	[ lindex $i 0 ]
		set type	[ lindex $i 1 ]
		set index	[ lindex $i 2 ]

		#
		# Create Button / Label
		#

		set fix_label [ strip_label $label ]

		if { $type == "checkbutton" } \
		{
			#
			# Get State From Info List
			#

			set state	[ lindex $i 3 ]
			set help	[ lindex $i 4 ]

			#
			# Create checkbutton state var
			#

			if { $prefix == "none" } \
				{ set prefix $root }

			set menuvar "${prefix}_state_$fix_label"

			global $menuvar

			if { $state == "ON" } \
				{ set $menuvar ON } \
\
			else \
				{ set $menuvar OFF }

			#
			# Set checkbutton command / create
			#

			set command [ list $cmd $index $label $menuvar FALSE ]

			set NAME "$name.ckbutt_$fix_label"

			checkbutton $NAME -text $label -command $command \
				-padx 1 -pady 1 \
				-bd 1 -relief raised -anchor nw \
				-onvalue ON -offvalue OFF -variable $menuvar \
				$SELECTOR $selector_color -foreground $fg_color \
				-activeforeground $active_fg_color

			restrict_bindings $NAME ""
		} \
\
		elseif { $type == "command" } \
		{
			set help	[ lindex $i 3 ]

			set command [ list $cmd $index $label none FALSE ]

			set NAME "$name.butt_$fix_label"

			button $NAME -text $label -command $command \
				-padx 1 -pady 1 \
				-bd 1 -relief raised \
				-foreground $fg_color -activeforeground $active_fg_color

			restrict_bindings $NAME ""
		} \
\
		elseif { $type == "radiobutton" } \
		{
			set rvar	[ lindex $i 3 ]
			set help	[ lindex $i 4 ]

			set command [ list $cmd $index $label none FALSE ]

			set NAME "$name.rbutt_$fix_label"

			radiobutton $NAME -text $label -command $command \
				-padx 1 -pady 1 \
				-value $index -variable $rvar \
				-bd 1 -relief raised \
				$SELECTOR $selector_color -foreground $fg_color \
				-activeforeground $active_fg_color

			restrict_bindings $NAME ""
		} \
\
		elseif { $type == "submenu" } \
		{
			set focus_entry [ lindex $i 3 ]
			set subs		[ lindex $i 4 ]
			set help		[ lindex $i 5 ]

			set NAME "$name.butt_$fix_label"

			set command [ list raiseSubMenu $index $name $NAME \
				$focus_entry $subs ]

			button $NAME -text $label -command $command \
				-padx 1 -pady 1 \
				-bd 1 -relief raised -foreground $fg_color \
				-activeforeground $active_fg_color

			restrict_bindings $NAME ""
		} \
\
		elseif { $type == "exchange" } \
		{
			set button	[ lindex $i 3 ]
			set parent	[ lindex $i 4 ]
			set subs	[ lindex $i 5 ]
			set help	[ lindex $i 6 ]

			set command \
				[ list exchangeMenu $index $name $button $parent $subs ]

			set NAME "$name.butt_$fix_label"

			button $NAME -text $label -command $command \
				-padx 1 -pady 1 \
				-bd 1 -relief raised -foreground $fg_color \
				-activeforeground $active_fg_color

			restrict_bindings $NAME ""
		} \
\
		elseif { $type == "exchangeIndirect" } \
		{
			set bname	[ lindex $i 3 ]
			set parent	[ lindex $i 4 ]
			set subs	[ lindex $i 5 ]
			set help	[ lindex $i 6 ]

			set command \
				[ list exchangeMenuIndirect $index $name $bname \
					$parent $subs ]

			set NAME "$name.butt_$fix_label"

			button $NAME -text $label -command $command \
				-padx 1 -pady 1 \
				-bd 1 -relief raised -foreground $fg_color \
				-activeforeground $active_fg_color

			restrict_bindings $NAME ""
		} \
\
		else \
		{
			set NAME "$name.label_$fix_label"

			label $NAME -text $label -foreground $fg_color

			restrict_bindings $NAME "Label"
		}

		if { $type != "label" } \
			{ butt_help $NAME button $help }

		#
		# Place Button / Label on Menu Frame
		#

		if { $LAST != "none" } \
			{ set mby [ below $LAST ] } \
\
		else \
			{ set mby $frame_border }

		place $NAME -x $frame_border -y $mby

		update

		#
		# Process Button Width
		#

		set wt [ winfo width $NAME ]

		if { $wt > $width } \
			{ set width $wt }

		#
		# Save Button Name
		#

		set LAST $NAME

		lappend $mil $NAME
	}

	#
	# Create "Done" Button
	#

	set NAME $name.butt_Done

	if { "$close" == "lower" } \
		{ set cmd [ list lowerMenu $name ] } \
\
	else \
		{ set cmd "$close" }

	button $NAME -text {Done} \
		-padx 1 -pady 1 \
		-bd 1 -relief raised -command $cmd \
		-foreground $fg_color -activeforeground $active_fg_color

	restrict_bindings $NAME ""

	bind $name <Leave> [ list leaveMenu $name %x %y $leavesubs $cmd ]

	if { $LAST != "none" } \
		{ set dy [ below $LAST ] } \
\
	else \
		{ set dy $frame_border }

	place $NAME -x $frame_border -y $dy

	butt_help $NAME close "Lower Menu"

	lappend $mil $NAME

	update

	#
	# Process Final Button Width
	#

	set wt [ winfo width $NAME ]

	if { $wt > $width } \
		{ set width $wt }

	#
	# Set Main Menu Frame Size
	#

	set menu_height [ expr [ below $NAME ] + 4 ]

	set menu_width [ expr $width + (4 * $frame_border) ]

	$name configure -width $menu_width -height $menu_height

	place forget $name

	#
	# Adjust Menu Button Widths
	#

	foreach i [ set $mil ] \
		{ place $i -width $width }

	#debug_proc makeMenu exit
}

proc makeHostsMenu { name cmd info_list close leavesubs prefix } \
{
	#debug_proc makeHostsMenu entry

	global active_fg_color
	global selector_color
	global scroll_width
	global frame_border
	global row_height
	global fg_color

	global YSCROLLINCREMENT
	global FRAME_OFFSET
	global SELECTOR

	set fo [ expr $FRAME_OFFSET * $frame_border ]

	#
	# Split Apart Lists
	#

	set cmds_list [ lindex $info_list 0 ]
	set hosts_list [ lindex $info_list 1 ]

	#
	# Create Main Menu Frame
	#

	frame $name -bd $frame_border -relief raised

	restrict_bindings $name "Frame"

	set root [ rootName $name ]

	#
	# Add Command Buttons
	#

	set cmil "${root}_cmd_list"

	global $cmil

	set $cmil ""

	set LAST none

	set cwidth 0

	foreach i $cmds_list \
	{
		set label	[ lindex $i 0 ]
		set index	[ lindex $i 1 ]
		set help	[ lindex $i 2 ]

		set fix_label [ strip_label $label ]

		#
		# Create Button
		#

		set command [ list $cmd $index $label none FALSE ]

		set NAME "$name.butt_$fix_label"

		button $NAME -text $label -command $command \
			-padx 1 -pady 1 \
			-bd 1 -relief raised \
			-foreground $fg_color -activeforeground $active_fg_color

		restrict_bindings $NAME ""

		butt_help $NAME button $help

		#
		# Place Button on Menu Frame
		#

		if { $LAST != "none" } \
			{ set mby [ below $LAST ] } \
\
		else \
			{ set mby $frame_border }

		place $NAME -x $frame_border -y $mby

		update

		#
		# Process Button Width
		#

		set wt [ winfo width $NAME ]

		if { $wt > $cwidth } \
			{ set cwidth $wt }

		#
		# Save Button Name
		#

		set LAST $NAME

		lappend $cmil $NAME
	}

	#
	# Get Y Cutoff Between Commands & Hosts
	#

	set ycutoff [ below $NAME ]

	#
	# Determine Menu Scrolling
	#

	set num [ llength $hosts_list ]

	if { $num > 8 } \
	{
		global ${root}_scroll_window

		set scrolling "TRUE"

		frame $name.scroll_frame -relief sunken -bd $frame_border

		restrict_bindings $name.scroll_frame "Frame"

		canvas $name.scroll_frame.canvas -relief flat \
			$YSCROLLINCREMENT $row_height -confine 0 \
			-scrollregion "0 0 500 $num"

		restrict_bindings $name.scroll_frame.canvas "Canvas"

		set fname $name.scroll_frame.canvas.frame

		frame $fname

		restrict_bindings $fname "Frame"

		set ${root}_scroll_window \
			[ $name.scroll_frame.canvas create window 0 0 -anchor nw ]

		set LAST none

		set cnt 0
	} \
\
	else \
	{
		set scrolling "FALSE"

		set fname $name
	}

	#
	# Do Hosts Buttons
	#

	set hmil "${root}_hosts_list"

	global $hmil

	set $hmil ""

	set hwidth 0

	foreach i $hosts_list \
	{
		set label	[ lindex $i 0 ]
		set index	[ lindex $i 1 ]
		set state	[ lindex $i 2 ]
		set help	[ lindex $i 3 ]

		set fix_label [ strip_label $label ]

		#
		# Create State Var
		#

		if { $prefix == "none" } \
			{ set prefix $root }

		set menuvar "${prefix}_state_${fix_label}_$index"

		global $menuvar

		if { $state == "ON" } \
			{ set $menuvar ON } \
\
		else \
			{ set $menuvar OFF }

		netSetMenuvar $index $menuvar

		#
		# Create Host Button
		#

		set command [ list $cmd $index $label $menuvar FALSE ]

		set NAME "$fname.ckbutt_$fix_label"

		checkbutton $NAME -text $label -command $command \
			-padx 1 -pady 1 \
			-bd 1 -relief raised -anchor nw \
			-onvalue ON -offvalue OFF -variable $menuvar \
			$SELECTOR $selector_color -foreground $fg_color \
			-activeforeground $active_fg_color

		restrict_bindings $NAME ""

		butt_help $NAME button $help

		#
		# Place Button on Menu Frame
		#

		if { $LAST != "none" } \
			{ set mby [ below $LAST ] } \
\
		else \
			{ set mby $frame_border }

		place $NAME -x $frame_border -y $mby

		update

		#
		# Check for Scrolling Height
		#

		if { $scrolling == "TRUE" } \
		{
			set cnt [ expr $cnt + 1 ]

			if { $cnt == 9 } \
			{
				global ${root}_scroll_cheight

				set ${root}_scroll_cheight \
					[ expr $mby + ($row_height / 2) ]
			}
		}

		#
		# Process Button Width
		#

		set wt [ winfo width $NAME ]

		if { $wt > $hwidth } \
			{ set hwidth $wt }

		#
		# Save Button Name
		#

		set LAST $NAME

		lappend $hmil $NAME
	}

	#
	# Create "Done" Button
	#

	set NAME $name.butt_Done

	if { "$close" == "lower" } \
		{ set cmd [ list lowerMenu $name ] } \
\
	else \
		{ set cmd "$close" }

	button $NAME -text {Done} \
		-padx 1 -pady 1 \
		-bd 1 -relief raised -command $cmd \
		-foreground $fg_color -activeforeground $active_fg_color

	restrict_bindings $NAME ""

	if { $scrolling == "FALSE" } \
	{
		bind $name <Leave> \
			[ list leaveMenu $name %x %y $leavesubs $cmd ]
	}

	if { $scrolling == "TRUE" } \
	{
		set dy [ expr [ set ${root}_scroll_cheight ] + $ycutoff \
			+ (5 * $frame_border) - $fo ]
	} \
\
	else \
	{
		if { $LAST != "none" } \
			{ set dy [ below $LAST ] } \
\
		else \
			{ set dy $frame_border }
	}

	place $NAME -x $frame_border -y $dy

	butt_help $NAME close "Lower Menu"

	lappend $cmil $NAME

	update

	#
	# Process Done Button Width
	#

	set wt [ winfo width $NAME ]

	if { $wt > $cwidth } \
		{ set cwidth $wt }

	#
	# Determine Total Hosts Width
	#

	if { $scrolling == "TRUE" } \
	{
		set swidth [ expr $hwidth + $scroll_width \
			+ (4 * $frame_border) + (2 * $fo) ]
	} \
\
	else \
		{ set swidth $hwidth }

	#
	# Determine Aggregate Button Widths
	#

	if { $cwidth > $swidth } \
	{
		set width $cwidth

		if { $scrolling == "TRUE" } \
		{
			set hwidth [ expr $width - $scroll_width \
				- (4 * $frame_border) - (2 * $fo) ]
		} \
\
		else \
			{ set hwidth $cwidth }
	} \
\
	else \
		{ set width $swidth }

	#
	# Set Main Menu Frame Size
	#

	set menu_height [ expr [ below $NAME ] + (2 * $frame_border) ]

	set menu_width [ expr $width + (4 * $frame_border) ]

	$name configure -width $menu_width -height $menu_height

	place forget $name

	#
	# Adjust Menu Button Widths
	#

	foreach i [ set $cmil ] \
		{ place $i -width $width }

	foreach i [ set $hmil ] \
		{ place $i -width $hwidth }

	#
	# Handle Scrolling Menu Assembly
	#

	if { $scrolling == "TRUE" } \
	{
		global ${root}_scroll_height
		global ${root}_scroll_yview

		set ${root}_scroll_height [ below $LAST ]

		set ${root}_scroll_yview 0

		#
		# Create Scrollbars
		#

		set cmd [ list scrollCanvas $name.scroll_frame.canvas \
			$name.scroll_bar -1 \
			${root}_scroll_height ${root}_scroll_cheight \
			${root}_scroll_yview VERT low $row_height ]

		scrollbar $name.scroll_bar -orient vert -bd $frame_border \
			-relief sunken -width $scroll_width -command $cmd

		restrict_bindings $name.scroll_bar "Scrollbar"

		scrollInit $name.scroll_frame.canvas $name.scroll_bar -1 \
			[ set ${root}_scroll_height ] \
			[ set ${root}_scroll_cheight ] \
			[ set ${root}_scroll_yview ] \
			VERT low $row_height

		#
		# Place Widgets
		#

		place $name.scroll_frame \
			-x [ expr $frame_border + $fo ] \
			-y [ expr $ycutoff + $fo ] \
			-width [ expr $hwidth + (4 * $frame_border) ] \
			-height [ expr [ set ${root}_scroll_cheight ] \
				+ (4 * $frame_border) - (2 * $fo) ]

		place $name.scroll_frame.canvas -x 0 -y 0 \
			-width [ expr $hwidth + (2 * $frame_border) ] \
			-height [ expr [ set ${root}_scroll_cheight ] \
				+ (2 * $frame_border) - (2 * $fo) ]

		update

		place $name.scroll_bar \
			-x [ right $name.scroll_frame ] -y $ycutoff \
			-width $scroll_width \
			-height [ expr [ set ${root}_scroll_cheight ] \
				+ (4 * $frame_border) ]

		#
		# Attach Scroll Window to Canvas
		#

		$name.scroll_frame.canvas itemconfigure \
			[ set ${root}_scroll_window ] -window $fname \
			-width [ expr $hwidth + (2 * $frame_border) ] \
			-height [ below $LAST ]
	}

	update

	#debug_proc makeHostsMenu exit
}

proc raiseMenu { name button parent subs } \
{
	#debug_proc raiseMenu entry

	foreach i $subs \
		{ place forget $i }

	set ckm [ winfo ismapped $name ]

	if { $ckm == 1 } \
		{ place forget $name } \
\
	else \
		{ placeMenu $name $button $parent }

	#debug_proc raiseMenu exit
}

proc raiseMenuIndirect { name bname button parent subs } \
{
	#debug_proc raiseMenuIndirect entry

	global $bname

	foreach i $subs \
		{ place forget $i }

	set ckm [ winfo ismapped $name ]

	if { $ckm == 1 } \
		{ place forget $name } \
\
	else \
	{
		set $bname $button

		placeMenu $name $button $parent
	}

	#debug_proc raiseMenuIndirect exit
}

proc placeMenu { name button parent } \
{
	#debug_proc placeMenu entry

	set px [ winfo rootx $parent ]
	set py [ winfo rooty $parent ]

	set x [ expr [ winfo rootx $button ] - $px ]

	set y [ expr [ winfo rooty $button ] \
		+ [ winfo height $button ] - $py ]

	place $name -x $x -y $y

	raise $name

	update

	check_in_main $name $x $y

	update

	#debug_proc placeMenu exit
}

proc replaceMenu { name button parent } \
{
	#debug_proc replaceMenu entry

	set ckup [ winfo ismapped $name ]

	if { $ckup == 1 } \
		{ placeMenu $name $button $parent }

	#debug_proc replaceMenu exit
}

proc raiseSubMenu { submenu menu button focus_entry subs } \
{
	#debug_proc raiseSubMenu entry

	foreach i $subs \
		{ place forget $i }

	set ckup [ winfo ismapped $submenu ]

	if { $ckup == 1 } \
		{ place forget $submenu } \
\
	else \
		{ placeSubMenu $submenu $menu $button $focus_entry }

	#debug_proc raiseSubMenu exit
}

proc placeSubMenu { submenu menu button focus_entry } \
{
	#debug_proc placeSubMenu entry

	set x [ winfo x $menu ]
	set y [ winfo y $menu ]

	set x [ expr $x + [ left $button ] ]
	set y [ expr $y + [ below $button ] ]

	place $submenu -x $x -y $y

	raise $submenu

	update

	check_in_main $submenu $x $y

	if { $focus_entry != "none" } \
		{ focus $focus_entry }

	update

	#debug_proc placeSubMenu exit
}

proc replaceSubMenu { submenu menu button focus_entry } \
{
	#debug_proc replaceSubMenu entry

	set ckup [ winfo ismapped $submenu ]

	if { $ckup == 1 } \
		{ placeSubMenu $submenu $menu $button $focus_entry }

	#debug_proc replaceSubMenu exit
}

proc exchangeMenu { submenu menu button parent subs } \
{
	#debug_proc exchangeMenu entry

	set cksm [ winfo ismapped $submenu ]

	if { $cksm == 1 } \
	{
		place forget $submenu

		raiseMenu $menu $button $parent $subs
	} \
\
	else \
	{
		place forget $menu

		raiseMenu $submenu $button $parent $subs
	}

	#debug_proc exchangeMenu exit
}

proc exchangeMenuIndirect { submenu menu bname parent subs } \
{
	#debug_proc exchangeMenuIndirect entry

	global $bname

	set cksm [ winfo ismapped $submenu ]

	if { $cksm == 1 } \
	{
		place forget $submenu

		raiseMenu $menu [ set $bname ] $parent $subs
	} \
\
	else \
	{
		place forget $menu

		raiseMenu $submenu [ set $bname ] $parent $subs
	}

	#debug_proc exchangeMenuIndirect exit
}

proc check_in_main { name x y } \
{
	#debug_proc check_in_main entry

	global main_height
	global main_width

	set wt [ winfo width $name ]

	set ckx [ expr $x + $wt ]

	if { $ckx > $main_width } \
	{
		set x [ expr $main_width - $wt ]

		if { $x < 0 } { set x 0 }

		place $name -x $x
	}

	set ht [ winfo height $name ]

	set cky [ expr $y + $ht ]

	if { $cky > $main_height } \
	{
		set y [ expr $main_height - $ht ]

		if { $y < 0 } { set y 0 }

		place $name -y $y
	}

	#debug_proc check_in_main exit
}

proc leaveMenu { name x y subs cmd } \
{
	#debug_proc leaveMenu entry

	set ht [ winfo height $name ]
	set wt [ winfo width $name ]

	if { $x < 0 || $y < 0 || $x >= $wt || $y >= $ht } \
	{
		foreach w $subs \
		{
			set ckw [ winfo ismapped $w ]

			if { $ckw == 1 } \
				{ return }
		}

		eval $cmd
	}

	#debug_proc leaveMenu exit
}

proc lowerMenu { name } \
{
	#debug_proc lowerMenu entry

	place forget $name

	#debug_proc lowerMenu exit
}

proc refresh_tasks { type } \
{
	#debug_proc refresh_tasks entry

	global border_space

	global ${type}_tasklist
	global ${type}_lasty

	#
	# Delete Any Old Task Buttons
	#

	foreach tb [ set ${type}_tasklist ] \
	{
		if { "$tb" != "none" } \
		{
			set bid [ lindex $tb 3 ]

			.xpvm.$type.canvas delete $bid
		}
	}

	set ${type}_tasklist none

	set ${type}_lasty $border_space

	#
	# Get New Task List & Create Buttons
	#

	pvm_get_tasks $type

	update

	#debug_proc refresh_tasks exit
}

proc add_list_task { type host name tid } \
{
	#debug_proc add_list_task entry

	global ${type}_tasklist
	global ${type}_cheight
	global ${type}_lasty
	global border_space
	global row_height
	global main_font
	global fg_color

	set str "$host:$name, TID=$tid"

	set id [ .xpvm.$type.canvas create text \
		$border_space [ set ${type}_lasty ] \
		-text $str -anchor nw -fill $fg_color -font $main_font ]

	set ${type}_lasty [ expr [ set ${type}_lasty ] + $row_height ]

	scrollSet .xpvm.$type.vert_sb [ set ${type}_lasty ] \
		[ set ${type}_cheight ] $row_height 0

	set t [ list $host $name $tid $id ]

	set ${type}_tasklist [ linsert [ set ${type}_tasklist ] 0 $t ]

	#debug_proc add_list_task exit
}

proc trace_controls { status } \
{
	#debug_proc trace_controls entry

	global trace_control_status

	global CTRL

	# Un-Highlight Previous Button

	if { $trace_control_status != "none" } \
		{ $CTRL.$trace_control_status configure -relief raised }

	# Highlight New Button

	$CTRL.$status configure -relief sunken

	set trace_control_status $status

	update

	#debug_proc trace_controls exit
}

proc setMsg { text } \
{
	#debug_proc setMsg entry

	.xpvm.message configure -text "Status:   $text"

	update

	#debug_proc setMsg exit
}

proc setMsgSafe { text } \
{
	#debug_proc setMsgSafe entry

	.xpvm.message configure -text "Status:   $text"

	#
	# No Update, Lest Someone Rub Your Rhubarb Behind Your Back...
	#

	#debug_proc setMsgSafe exit
}

proc setTmpMsg { hdr text } \
{
	#debug_proc setTmpMsg entry

	global tmp_msg

	.xpvm.tmp_msg configure -text "$hdr:   $text"

	set tmp_msg [ list $hdr $text ]

	update

	#debug_proc setTmpMsg exit
}

proc popMsg { } \
{
	#debug_proc popMsg entry

	global tmp_msg

	.xpvm.tmp_msg configure -text "Help:"

	set tmp_msg [ list "" "" ]

	update

	#debug_proc popMsg exit
}

proc tmpMsg { } \
{
	#debug_proc tmpMsg entry

	global tmp_msg

	return [ lindex $tmp_msg 1 ]

	#debug_proc tmpMsg exit
}

proc rootName { path } \
{
	#debug_proc rootName entry

	set tmp [ split $path . ]

	set num [ llength $tmp ]

	set root [ lindex $tmp [ expr $num - 1 ] ]

	return $root

	#debug_proc rootName exit
}

proc above { win } \
{
	#debug_proc above entry

	set y [ winfo y $win ]

	return $y

	#debug_proc above exit
}

proc below { win } \
{
	#debug_proc below entry

	set y [ expr [ winfo y $win ] + [ winfo height $win ] ]

	return $y

	#debug_proc below exit
}

proc left { win } \
{
	#debug_proc left entry

	set x [ winfo x $win ]

	return $x

	#debug_proc left exit
}

proc right { win } \
{
	#debug_proc right entry

	set x [ expr [ winfo x $win ] + [ winfo width $win ] ]

	return $x

	#debug_proc right exit
}

proc get_user_name { } \
{
	#debug_proc get_user_name entry

	set home [ glob ~ ]

	set ckhome [ info exists home ]

	if { $ckhome == 1 } \
	{
		set len [ string length $home ]

		set slash_index [ string last / $home ]

		set user_index [ expr $slash_index + 1 ]

		set user [ string range $home $user_index $len ]
	} \
\
	else \
	{
		set user [ pid ]
	}

	return $user

	#debug_proc get_user_name exit
}

proc get_base_text_size { } \
{
	#debug_proc get_base_text_size entry

	global row_height
	global col_width

	#
	# Create Dummy Label
	#

	set DUMMY .xpvm.dummy.dummy

	label $DUMMY -text "xxx"

	restrict_bindings $DUMMY "Label"

	place $DUMMY -x 0 -y 0

	#
	# Run Tests
	#

	set test_list "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z"

	set row_height 0
	set col_width 0

	foreach c $test_list \
	{
		set wtht1 [ get_real_text_size "$c" ]

		set wtht2 [ get_real_text_size "$c$c" ]

		set wt1 [ lindex $wtht1 0 ]
		set wt2 [ lindex $wtht2 0 ]

		set wt [ expr $wt2 - $wt1 ]

		set ht [ lindex $wtht1 1 ]

		set col_width [ max_len $wt $col_width ]

		set row_height [ max_len $ht $row_height ]
	}

	destroy $DUMMY

	#debug_proc get_base_text_size exit
}

proc get_fixed_text_size { } \
{
	#debug_proc get_fixed_text_size entry

	global fixed_row_height
	global fixed_col_width
	global fixed_font

	#
	# Create Dummy Label
	#

	set DUMMY .xpvm.dummy.dummy

	label $DUMMY -text "xxx" -font $fixed_font

	restrict_bindings $DUMMY "Label"

	place $DUMMY -x 0 -y 0

	#
	# Run Tests
	#

	set test_list "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z"

	set fixed_row_height 0
	set fixed_col_width 0

	foreach c $test_list \
	{
		set wtht1 [ get_real_text_size "$c" ]

		set wtht2 [ get_real_text_size "$c$c" ]

		set wt1 [ lindex $wtht1 0 ]
		set wt2 [ lindex $wtht2 0 ]

		set wt [ expr $wt2 - $wt1 ]

		set ht [ lindex $wtht1 1 ]

		set fixed_col_width [ max_len $wt $fixed_col_width ]

		set fixed_row_height [ max_len $ht $fixed_row_height ]
	}

	destroy $DUMMY

	#debug_proc get_fixed_text_size exit
}

proc get_real_text_size { str } \
{
	#debug_proc get_real_text_size entry

	set DUMMY .xpvm.dummy.dummy

	$DUMMY configure -text $str

	update

	set ht [ winfo height $DUMMY ]

	set wt [ winfo width $DUMMY ]

	return "$wt $ht"

	#debug_proc get_real_text_size exit
}

proc max_len { s1 s2 } \
{
	#debug_proc max_len entry

	if { $s1 > $s2 } \
		{ return $s1 } \
\
	else \
		{ return $s2 }

	#debug_proc max_len exit
}

proc helpSelect { index label menuvar setflag } \
{
	#debug_proc helpSelect entry

	global active_fg_color
	global min_help_height
	global min_help_width
	global scroll_width
	global border_space
	global frame_border
	global row_height
	global col_width
	global main_font
	global fg_color
	global help_dir

	#
	# Check for Existing Help Window
	#

	set WIN .${index}_help_win

	set hwinck [ winfo exists $WIN ]

	if { $hwinck == 1 } \
	{
		set hwstate [ wm state $WIN ]

		if { $hwstate == "normal" } \
		{
			global ${index}_visibility

			set vis [ set ${index}_visibility ]

			if { $vis == "VisibilityUnobscured" } \
				{ wm withdraw $WIN } \
\
			else \
			{
				set ${index}_visibility "VisibilityUnobscured"

				raise $WIN
			}
		} \
\
		else \
			{ wm deiconify $WIN }

		return
	}

	#
	# Get Help File
	#

	set fname $help_dir/$index.help

	set fileck [ file exists $fname ]

	if { $fileck != 1 } \
	{
		setMsg { Help File $fname Not Found. }

		return
	}

	set fp [ open $fname ]

	set whole [ read $fp ]

	set title [ lindex $whole 0 ]

	set text [ lreplace $whole 0 0 ]

	close $fp

	#
	# Create Help Window
	#

	global help_stagger_index
	global ${index}_canvas_height
	global ${index}_canvas_width
	global ${index}_visibility
	global ${index}_cheight
	global ${index}_cwidth
	global ${index}_height
	global ${index}_width

	set ${index}_height [ expr $row_height * 30 ]
	set ${index}_width [ expr $col_width * 40 ]

	set ht [ set ${index}_height ]
	set wt [ set ${index}_width ]

	set stagx [ expr 50 + ( $help_stagger_index * $frame_border * 2 ) ]
	set stagy [ expr 125 + ( $help_stagger_index * $border_space ) ]

	set help_stagger_index [ expr $help_stagger_index + 1 ]

	set hgeom "[expr $wt]x[expr $ht]+[expr $stagx]+[expr $stagy]"

	toplevel $WIN

	wm geometry $WIN $hgeom

	wm minsize $WIN $min_help_width $min_help_height

	set ${index}_visibility "VisibilityUnobscured"

	global XSCROLLINCREMENT
	global YSCROLLINCREMENT

	set WIN_C $WIN.canvas

	set WIN_SBH $WIN.horiz_sb
	set WIN_SBV $WIN.vert_sb

	set ${index}_cheight [ expr [ set ${index}_height ] \
		- $scroll_width - (2 * $row_height) - (2 * $border_space) ]

	set ${index}_cwidth [ expr [ set ${index}_width ] - $scroll_width \
		- (2 * $border_space) ]

	label $WIN.title -text $title -foreground $fg_color

	restrict_bindings $WIN.title "Label"

	set cht [ expr $row_height * ( [ llength $text ] + 1 ) ]

	set cwt [ expr $col_width * 80 ]

	canvas $WIN_C -bd $frame_border -relief sunken \
		$XSCROLLINCREMENT $row_height $YSCROLLINCREMENT $row_height \
		-confine 0 -scrollregion "0 0 $cwt $cht"

	restrict_bindings $WIN_C "Canvas"

	set ${index}_canvas_height $cht

	set ${index}_canvas_width $cwt

	set cmd "scrollCanvas $WIN_C $WIN_SBH \
		-1 ${index}_canvas_width ${index}_cwidth ${index}_xview \
		HORIZ low $row_height"

	scrollbar $WIN_SBH -orient horiz -bd $frame_border -relief sunken \
		-width $scroll_width -command $cmd

	restrict_bindings $WIN_SBH "Scrollbar"

	scrollInit $WIN_C $WIN_SBH -1 $cwt \
		[ set ${index}_cwidth ] ${index}_xview HORIZ low $row_height

	set cmd "scrollCanvas $WIN_C $WIN_SBV \
		-1 ${index}_canvas_height ${index}_cheight ${index}_yview \
		VERT low $row_height"

	scrollbar $WIN_SBV -orient vert -bd $frame_border -relief sunken \
		-width $scroll_width -command $cmd

	restrict_bindings $WIN_SBV "Scrollbar"

	scrollInit $WIN_C $WIN_SBV -1 $cht \
		[ set ${index}_cheight ] ${index}_yview VERT low $row_height

	button $WIN.close -text {Close} -command "wm withdraw $WIN" \
		-padx 1 -pady 1 \
		-foreground $fg_color -activeforeground $active_fg_color

	layout_help_win $index

	bind $WIN <Configure> [ list resize_help_win $index ]

	bind $WIN <Visibility> [ list help_visibility $index %s ]

	#
	# Fill Canvas With Text
	#

	set ht $border_space
	set wt $border_space

	set cnt 0

	foreach line $text \
	{
		set lid "$WIN.line$cnt"

		set fix_line [ fix_help_line $line ]

		set wid [ $WIN_C create text $wt $ht -text $fix_line \
			-anchor nw -fill $fg_color -font $main_font ]

		set ht [ expr $ht + $row_height ]

		set cnt [ expr $cnt + 1 ]
	}

	#debug_proc helpSelect exit
}

proc help_visibility { index detail } \
{
	#debug_proc help_visibility entry

	global ${index}_visibility

	set ${index}_visibility $detail

	#debug_proc help_visibility exit
}

proc butt_help { butt type msg } \
{
	#debug_proc butt_help entry

	bind $butt <Enter> [ list do_butt_help IN $butt $type $msg ]

	bind $butt <Motion> [ list do_butt_help IN $butt $type $msg ]

	if { $type == "button" } \
		{ bind $butt <ButtonPress> [ list $butt invoke ] } \
\
	elseif { $type == "close" } \
		{ bind $butt <ButtonPress> "popMsg ; $butt invoke" }

	bind $butt <Leave> [ list do_butt_help OUT $butt $type $msg ]

	#debug_proc butt_help exit
}

proc do_butt_help { cmd butt type msg } \
{
	#debug_proc do_butt_help entry

	if { $cmd == "IN" } \
	{
		if { $type == "entry" } \
		{
			set msg [ lindex $msg 1 ]

			focus $butt
		} \
\
		elseif { $type == "entry_nf" } \
			{ set msg [ lindex $msg 1 ] }

		if { $type == "button" } \
			{ $butt configure -state active } \
\
		elseif { $type == "close" } \
			{ $butt configure -state active }

		if { $msg != [ tmpMsg ] } \
			{ setTmpMsg "Help" $msg }
	} \
\
	elseif { $cmd == "OUT" } \
	{
		popMsg

		if { $type == "entry" } \
		{
			set cmd [ lindex $msg 0 ]

			eval $cmd
		}

		if { $type == "button" } \
			{ $butt configure -state normal } \
\
		elseif { $type == "close" } \
			{ $butt configure -state normal }
	}

	#debug_proc do_butt_help exit
}

proc unmap_all { } \
{
	#debug_proc unmap_all entry

	global win_map_list

	global NK
	global UT
	global MQ
	global CT
	global TO
	global TV

	set win_map_list ""

	set tmp_list ".xpvm"

	foreach w "$NK $UT $MQ $CT $TO $TV" \
	{
		set ckup [ winfo ismapped $w ]

		if { $ckup == 1 } \
			{ lappend tmp_list $w }
	}

	set tmp_list [ sort_map_list $tmp_list ]

	foreach w $tmp_list \
		{ wm iconify $w }

	set win_map_list $tmp_list

	#debug_proc unmap_all exit
}

proc remap_all { } \
{
	#debug_proc remap_all entry

	global win_map_list

	set save_list $win_map_list

	set win_map_list ""

	foreach w $save_list \
		{ wm deiconify $w }

	foreach w $save_list \
		{ raise $w }

	#debug_proc remap_all exit
}

proc sort_map_list { wlist } \
{
	#debug_proc sort_map_list entry

	set num [ llength $wlist ]

	set done 0

	while { $done == 0 } \
	{
		set done 1

		for { set i 0 } { $i < $num - 1 } { set i [ expr $i + 1 ] } \
		{
			set ip1 [ expr $i + 1 ]

			set w [ lindex $wlist $i ]
			set t [ lindex $wlist $ip1 ]

			set tw [ top_window $w $t ]

			if { $tw == $w } \
			{
				set wlist [ lreplace $wlist $i $ip1 $t $w ]

				set done 0
			}
		}
	}

	#debug_proc sort_map_list exit

	return $wlist
}

proc top_window { w1 w2 } \
{
	#debug_proc top_window entry

	set tw [ do_top_window $w1 $w2 ]

	if { $tw == "" } \
		{ set tw [ do_top_window $w2 $w1 ] }

	#debug_proc top_window exit

	return $tw
}

proc do_top_window { w1 w2 } \
{
	#debug_proc do_top_window entry

	set x1 [ winfo x $w1 ]
	set y1 [ winfo y $w1 ]

	set x2 [ winfo x $w2 ]
	set y2 [ winfo y $w2 ]

	set ht1 [ winfo height $w1 ]
	set wt1 [ winfo width $w1 ]

	set ht2 [ winfo height $w2 ]
	set wt2 [ winfo width $w2 ]

	if { $x1 <= $x2 && $x2 <= $x1 + $wt1 \
		&& $y1 <= $y2 && $y2 <= $y1 + $ht1 } \
	{
		#
		# ( x2, y2 ) inside w1
		#

		set tw [ winfo containing $x2 $y2 ]
	} \
\
	elseif { $x1 <= $x2 + $wt2 && $x2 + $wt2 <= $x1 + $wt1 \
		&& $y1 <= $y2 && $y2 <= $y1 + $ht1 } \
	{
		#
		# ( x2 + wt2, y2 ) inside w1
		#

		set tw [ winfo containing [ expr $x2 + $wt2 ] $y2 ]
	} \
\
	elseif { $x1 <= $x2 && $x2 <= $x1 + $wt1 \
		&& $y1 <= $y2 + $ht2 && $y2 + $ht2 <= $y1 + $ht1 } \
	{
		#
		# ( x2, y2 + ht2 ) inside w1
		#

		set tw [ winfo containing $x2 [ expr $y2 + $ht2 ] ]
	} \
\
	elseif { $x1 <= $x2 + $wt2 && $x2 + $wt2 <= $x1 + $wt1 \
		&& $y1 <= $y2 + $ht2 && $y2 + $ht2 <= $y1 + $ht1 } \
	{
		#
		# ( x2 + wt2, y2 + ht2 ) inside w1
		#

		set tw [ winfo containing \
			[ expr $x2 + $wt2 ] [ expr $y2 + $ht2 ] ]
	} \
\
	else \
		{ set tw "" }

	if { $tw != "" } \
		{ set tw [ winfo toplevel $tw ] }

	#debug_proc do_top_window exit

	return $tw
}

proc exit { } \
{
	#debug_proc exit entry

	pvm_quit

	#debug_proc exit exit
}

proc quit { } \
{
	#debug_proc quit entry

	pvm_quit

	#debug_proc quit exit
}

proc halt { } \
{
	#debug_proc halt entry

	pvm_halt

	#debug_proc halt exit
}

proc debug_proc { routine inout } \
{
	global proc_debug

	if { $proc_debug == "TRUE" } \
		{ puts "(proc debug: $routine{} $inout)" }
}

