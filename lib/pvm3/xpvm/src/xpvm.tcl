#
# $Id: xpvm.tcl,v 4.50 1998/04/09 21:12:32 kohl Exp $
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


#
# Main TCL Source File
#


#
# Hide Main Window
#

wm withdraw .


#
# Setup Main TCL Directories
#

set ckenv [ info exists env(XPVM_ROOT) ]

if { $ckenv == 1 } \
	{ set xpvm_directory $env(XPVM_ROOT) } \
\
else \
{
	set home	[ glob ~ ]
	set def		[ get_xpvm_default_dir ]

	set xpvm_directory "$home/$def"
}

set help_dir $xpvm_directory/src/help


#
# Source Compatibility Globals
#

set ckdir_globs [ file exists globs.tcl ]

if { $ckdir_globs == 1 } \
	{ source globs.tcl } \
\
else \
	{ source $xpvm_directory/globs.tcl }


#
# Source Main TCL Procs
#

set ckdir_procs [ file exists procs.tcl ]

if { $ckdir_procs == 1 } \
	{ source procs.tcl } \
\
else \
	{ source $xpvm_directory/procs.tcl }


#
# Source TCL Utility Procs
#

set ckdir_util [ file exists util.tcl ]

if { $ckdir_util == 1 } \
	{ source util.tcl } \
\
else \
	{ source $xpvm_directory/util.tcl }


#
# Set X11 Bitmap Directory
#

set xbm_dir "$xpvm_directory/src/xbm"

set ckdir_xbm [ file exists $xbm_dir ]

if { $ckdir_xbm == 0 } \
{
	set ckdir_xbm [ file exists "$xpvm_directory/xbm" ]

	if { $ckdir_xbm == 1 } \
		{ set xbm_dir "$xpvm_directory/xbm" } \
\
	else \
	{
		set ckdir_xbm [ file exists src/xbm ]

		if { $ckdir_xbm == 1 } \
			{ set xbm_dir ./src/xbm } \
\
		else \
		{
			set ckdir_xbm [ file exists xbm ]

			if { $ckdir_xbm == 1 } \
				{ set xbm_dir ./xbm } \
\
			else \
			{
				puts ""
				puts "Error Finding xbm Subdirectory - Exiting."

				exit
			}
		}
	}
}


#
# Startup Message
#

puts ""

puts -nonewline "Initializing XPVM..."
flush stdout


#
# Proc Debug Flag
#

set proc_debug FALSE
#set proc_debug TRUE


#
# Get User Name
#

set user [ get_user_name ]


#
# Create Toplevel Window
#

set main_height 870
set main_width 700

set main_geom "[ expr $main_width ]x[ expr $main_height ]+10+10"

toplevel .xpvm

wm geometry .xpvm $main_geom

wm withdraw .xpvm


#
# Create Dummy Frame for Text Sizing
#

frame .xpvm.dummy

restrict_bindings .xpvm.dummy "Frame"

get_base_text_size


#
# Set up Spacing Constants
#

set scroll_width 20

set border_space 10

set frame_border 2


#
# General View Constants / Globals
#

set scroll_canvas_width 8000

set scale_stack [ list empty ]

set scale 20000

set timeid_prim ""

set timex 0


#
# Help Window Constants
#

set min_help_height [ expr (2 * $row_height) + (3 * $scroll_width) \
	+ (2 * $border_space) ]

set min_help_width 250

set help_stagger_index 0


#
# Network View Constants
#

set net_active TRUE

set net_scroll 10

set net_size 2000

set icon_border 2

set icon_space 10

set net_space 24

set net_flash 10

set min_net_height [ expr (2 * $row_height) + (3 * $scroll_width) \
	+ (2 * $border_space) ]


#
# View Select & Playback Controls Constants
#

set min_ctrl_height [ expr (2 * $row_height) + (3 * $border_space) ]


#
# Space-Time View Constants
#

set st_labels_width [ expr 15 * $col_width ]

set change_st_height_lock 0

set st_canvas_height 2000

set st_active TRUE

set st_highlight_type ""

set st_highlight_id -1

set st_label_ht $row_height

set st_rect_ht 8

set st_timeid -1

set min_st_height [ expr (2 * $row_height) + (3 * $scroll_width) \
	+ (3 * $border_space) ]

set st_query_keep FALSE

set value_factor 1

set last_value -1000

set zoom_lock 0


#
# Utilization View Constants
#

set ut_canvas_height 4000

set ut_active FALSE

set ut_state [ list none ]

set ut_timeid -1

set ut_height 350
set ut_width 700


#
# Message Queue View Constants
#

set mq_canvas_height 1000
set mq_canvas_width 2000

set mq_query_index_type none
set mq_query_index 0

set mq_height 250
set mq_width 400

set mq_task_wt 20

set mq_active FALSE

set mq_query_keep FALSE


#
# Call Trace View Constants
#

set ct_labels_width [ expr 15 * $col_width ]

set ct_canvas_height 4000
set ct_canvas_width 2000

set ct_query_index -1

set ct_height 340
set ct_width 650

set ct_active FALSE


#
# Task Output View Constants
#

set to_canvas_height 8000
set to_canvas_width 2000

set to_height 300
set to_width 600

set to_active FALSE


#
# Task Trace Event History View Constants
#

set tv_canvas_height 8000
set tv_canvas_width 2000

set tv_height 300
set tv_width 600

set tv_active FALSE


#
# Set up Colors
#

set depth [ winfo depth .xpvm ]

if { $depth == 1 } \
{
	set active_fg_color white

	set selector_color black

	set fg_color black

	set net_running_color black
	set net_system_color white
	set net_idle_color white
	set net_empty_color white
	set net_fg_color black

	set net_alive_color black
	set net_dead_color black

	set net_volume_color(0) black
	set net_volume_color(1) black
	set net_volume_color(2) black
	set net_volume_color(3) black
	set net_volume_color(4) black
	set net_volume_color(5) black
	set net_volume_color(6) black
	set net_volume_color(7) black
	set net_volume_color_size 8

	set st_user_defined_color black
	set st_running_color black
	set st_system_color white
	set st_idle_color white
	set st_comm_color black
	set st_outline_color black

	set ut_running_color black
	set ut_system_color white
	set ut_idle_color black

	set mq_msg_nosize_color white
	set mq_msg_size_color white
	set mq_outline_color black
} \
\
else \
{
	set active_fg_color purple

	set selector_color purple

	set fg_color blue

	set net_running_color green
	set net_system_color yellow
	set net_idle_color yellow
	set net_empty_color white
	set net_fg_color black

	set net_alive_color blue
	set net_dead_color red

	set net_volume_color(0) blue
	set net_volume_color(1) purple
	set net_volume_color(2) magenta
	set net_volume_color(3) green
	set net_volume_color(4) lightblue
	set net_volume_color(5) orange
	set net_volume_color(6) red
	set net_volume_color(7) maroon
	set net_volume_color_size 8

	set st_user_defined_color purple
	set st_running_color green
	set st_system_color yellow
	set st_idle_color white
	set st_comm_color red
	set st_outline_color black

	set ut_running_color green
	set ut_system_color yellow
	set ut_idle_color red

	set mq_msg_nosize_color orange
	set mq_msg_size_color purple
	set mq_outline_color black
}


#
# Network Volume Key Constants
#

set_net_volume_levels

set nk_height [ expr $row_height * ( $net_volume_color_size + 4 ) ]

set nk_width 150


#
# Define PVM Architectures
#

define_arch AFX8		"$xbm_dir/alliant.xbm"
define_arch AIX46K		"$xbm_dir/aix46k.xbm"
define_arch AIX4MP		"$xbm_dir/aix4mp.xbm"
define_arch AIX4SP2		"$xbm_dir/aix4sp2.xbm"
define_arch ALPHA		"$xbm_dir/alpha.xbm"
define_arch ALPHAMP		"$xbm_dir/alphamp.xbm"
define_arch BAL			"$xbm_dir/sequent.xbm"
define_arch CM2			"$xbm_dir/cm2.xbm"
define_arch CM5			"$xbm_dir/cm5.xbm"
define_arch CNVX		"$xbm_dir/convex.xbm"
define_arch CRAY		"$xbm_dir/cray.xbm"
define_arch CRAYSMP		"$xbm_dir/cray.xbm"
define_arch CSPP		"$xbm_dir/cspp.xbm"
define_arch HPPA		"$xbm_dir/hppa.xbm"
define_arch I860		"$xbm_dir/i860.xbm"
define_arch IPSC2		"$xbm_dir/ipsc2.xbm"
define_arch LINUX		"$xbm_dir/linux.xbm"
define_arch LINUXSPARC	"$xbm_dir/linuxsparc.xbm"
define_arch LINUXALPHA	"$xbm_dir/linuxalpha.xbm"
define_arch KSR1		"$xbm_dir/ksr1.xbm"
define_arch NEXT		"$xbm_dir/next.xbm"
define_arch OS2			"$xbm_dir/os2.xbm"
define_arch PGON		"$xbm_dir/pgon.xbm"
define_arch PMAX		"$xbm_dir/pmax.xbm"
define_arch RS6K		"$xbm_dir/rs6k.xbm"
define_arch RS6KMP		"$xbm_dir/rs6kmp.xbm"
define_arch RT			"$xbm_dir/rs6k.xbm"
define_arch SGI			"$xbm_dir/sgi.xbm"
define_arch SGI5		"$xbm_dir/sgi5.xbm"
define_arch SGI6		"$xbm_dir/sgi6.xbm"
define_arch SGI64		"$xbm_dir/sgi64.xbm"
define_arch SGIMP		"$xbm_dir/sgimp.xbm"
define_arch SGIMP6		"$xbm_dir/sgimp6.xbm"
define_arch SGIMP64		"$xbm_dir/sgimp64.xbm"
define_arch SUN3		"$xbm_dir/sun3.xbm"
define_arch SUN4		"$xbm_dir/sparc.xbm"
define_arch SUN4SOL2	"$xbm_dir/sun4sol2.xbm"
define_arch SUNMP		"$xbm_dir/sunmp.xbm"
define_arch SYMM		"$xbm_dir/symmetry.xbm"
define_arch WIN32		"$xbm_dir/win32.xbm"

# define_arch BFLY
# define_arch BSD386
# define_arch CNVXN
# define_arch DGAV
# define_arch HP300
# define_arch TITN
# define_arch UVAX

define_arch DUMMY	"$xbm_dir/dummy_arch.xbm"


#
# Load Bitmaps
#

load_bitmap_file "$xbm_dir/rewind.xbm"
load_bitmap_file "$xbm_dir/stop.xbm"
load_bitmap_file "$xbm_dir/fastfwd.xbm"
load_bitmap_file "$xbm_dir/fwd.xbm"
load_bitmap_file "$xbm_dir/fwdstep.xbm"

load_bitmap_file "$xbm_dir/shrink.xbm"
load_bitmap_file "$xbm_dir/grow.xbm"
load_bitmap_file "$xbm_dir/adj.xbm"


#
# Set Main Window Min Size
#

set min_main_height [ expr (3 * $row_height) + (3 * $border_space) \
	+ $min_net_height + $min_ctrl_height + $min_st_height ]

set min_main_width 400

wm minsize .xpvm $min_main_width $min_main_height


#
# Create Title & Version Number
#

set title_str "X P V M  [ title_info ]"

label .xpvm.title -text $title_str -foreground $fg_color

restrict_bindings .xpvm.title "Label"

label .xpvm.active -text " " -foreground $fg_color

restrict_bindings .xpvm.active "Label"


#
# Create Info Message Label
#

label .xpvm.message -text {Status:   Welcome to XPVM} \
	-foreground $fg_color -relief sunken -anchor nw

restrict_bindings .xpvm.message "Label"

label .xpvm.tmp_msg -text {Help:} \
	-foreground $fg_color -relief sunken -anchor nw

restrict_bindings .xpvm.tmp_msg "Label"

set tmp_msg ""


#
# Place Title & Message
#

place .xpvm.title -relx 0.5 -y $border_space -anchor n

place .xpvm.active -x $border_space -y $border_space -anchor nw

set msg_y [ expr $row_height + (3 * $border_space / 2) ]

set msg_wt [ expr $main_width - (2 * $border_space) ]

place .xpvm.message -x $border_space -y $msg_y -width $msg_wt

update

set msg_y [ below .xpvm.message ]

place .xpvm.tmp_msg -x $border_space -y $msg_y -width $msg_wt

update


#
# Get Main Font for Entry Widgets
#

set main_font [ lindex [ .xpvm.title configure -font ] 3 ]


#
# Get Fixed Font for Text Stuff
#

set fixed_font "fixed"

get_fixed_text_size


#
# Set Up Submenu Dependencies
#

set hosts_subs [ list .xpvm.otherhost ]

set tm_subs [ list .xpvm.host_ev_menu .xpvm.group_ev_menu \
	.xpvm.task_ev_menu .xpvm.ctxt_ev_menu .xpvm.comm_ev_menu \
	.xpvm.coll_ev_menu .xpvm.mbox_ev_menu .xpvm.buff_ev_menu \
	.xpvm.pack_ev_menu .xpvm.unpack_ev_menu .xpvm.info_ev_menu \
	.xpvm.misc_ev_menu ]

set tm33_subs [ list .xpvm.host_ev33_menu .xpvm.group_ev33_menu \
	.xpvm.task_ev33_menu .xpvm.comm_ev33_menu .xpvm.coll_ev33_menu \
	.xpvm.buff_ev33_menu .xpvm.pack_ev33_menu .xpvm.unpack_ev33_menu \
	.xpvm.info_ev33_menu .xpvm.misc_ev33_menu ]

set spawn_subs [ concat $tm_subs $tm33_subs \
	.xpvm.trace_mask_menu .xpvm.trace33_mask_menu ]

set otf_subs [ concat $tm_subs $tm33_subs \
	.xpvm.trace_mask_menu .xpvm.trace33_mask_menu ]

set tasks_subs [ concat $spawn_subs .xpvm.spawn \
	$otf_subs .xpvm.otf .xpvm.kill .xpvm.signal \
	.xpvm.systasks_menu ]


#
# Create Console Buttons
#

button .xpvm.file -text {File...} \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color \
	-command [ list raiseMenu .xpvm.file_menu .xpvm.file .xpvm {} ]

restrict_bindings .xpvm.file ""

button .xpvm.hosts -text {Hosts...} \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color \
	-command [ list raiseMenu .xpvm.hosts_menu .xpvm.hosts .xpvm \
		$hosts_subs ]

restrict_bindings .xpvm.hosts ""

button .xpvm.tasks -text {Tasks...} \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color \
	-command [ list raiseMenu .xpvm.tasks_menu .xpvm.tasks .xpvm \
		$tasks_subs ]

restrict_bindings .xpvm.tasks ""

button .xpvm.views -text {Views...} \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color \
	-command [ list raiseMenu .xpvm.views_menu .xpvm.views .xpvm {} ]

restrict_bindings .xpvm.views ""

button .xpvm.options -text {Options...} \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color \
	-command [ list raiseMenu .xpvm.options_menu .xpvm.options .xpvm \
		{} ]

restrict_bindings .xpvm.options ""

button .xpvm.reset -text {Reset...} \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color \
	-command [ list raiseMenu .xpvm.reset_menu .xpvm.reset .xpvm {} ]

restrict_bindings .xpvm.reset ""

set help_subs [ list .xpvm.tasks_help_menu .xpvm.views_help_menu ]

button .xpvm.help -text {Help...} \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color \
	-command [ list raiseMenu .xpvm.help_menu .xpvm.help .xpvm \
		$help_subs ]

restrict_bindings .xpvm.help ""


#
# Place Console Command Buttons
#

set cons_y [ expr $msg_y + $row_height + $border_space ]

place .xpvm.file	-relx 0.075 -y $cons_y	-anchor n
place .xpvm.hosts	-relx 0.195 -y $cons_y	-anchor n
place .xpvm.tasks	-relx 0.335 -y $cons_y	-anchor n
place .xpvm.views	-relx 0.480 -y $cons_y	-anchor n
place .xpvm.options	-relx 0.630 -y $cons_y	-anchor n
place .xpvm.reset	-relx 0.780 -y $cons_y	-anchor n
place .xpvm.help	-relx 0.910 -y $cons_y	-anchor n

update


#
# Set Up Initial Main Panel Constants
#

set start_main_y [ expr [ below .xpvm.hosts ] + ($border_space / 2) ]

set net_y $start_main_y

set ctrl_width $main_width
set net_width $main_width
set st_width $main_width

set ctrl_x 0
set net_x 0
set st_x 0


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create View Select & Playback Controls
#

set CTRL .xpvm.controls

set ADJ $CTRL.adjust

frame $CTRL -bd $frame_border -relief sunken

restrict_bindings $CTRL "Frame"

button $ADJ -bitmap "@$xbm_dir/adj.xbm" \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $ADJ ""

button $CTRL.rewind -bitmap "@$xbm_dir/rewind.xbm" \
	-padx 1 -pady 1 \
	-command trace_rewind \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $CTRL.rewind ""

button $CTRL.stop -bitmap "@$xbm_dir/stop.xbm" \
	-padx 1 -pady 1 \
	-command trace_stop \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $CTRL.stop ""

button $CTRL.fwdstep -bitmap "@$xbm_dir/fwdstep.xbm" \
	-padx 1 -pady 1 \
	-command trace_fwdstep \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $CTRL.fwdstep ""

button $CTRL.fwd -bitmap "@$xbm_dir/fwd.xbm" \
	-padx 1 -pady 1 \
	-command trace_fwd \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $CTRL.fwd ""

button $CTRL.fastfwd -bitmap "@$xbm_dir/fastfwd.xbm" \
	-padx 1 -pady 1 \
	-command trace_fastfwd \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $CTRL.fastfwd ""

set trace_control_status "none"

label $CTRL.time -text {Time: 0.000000} -foreground $fg_color

restrict_bindings $CTRL.time "Label"

label $CTRL.file -text {Trace File:} -foreground $fg_color

restrict_bindings $CTRL.file "Label"

entry $CTRL.file_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $CTRL.file_entry "set_trace_file"

restrict_bindings $CTRL.file_entry "Entry"

set tfck [ info exists trace_file ]

if { $tfck == 0 } { set trace_file "/tmp/xpvm.trace.$user" }

$CTRL.file_entry insert 0 $trace_file

radiobutton $CTRL.overwrite -text {OverWrite } \
	-padx 1 -pady 1 \
	-command "handleTraceFileStatus" \
	-bd $frame_border -relief sunken -value "OverWrite" \
	-variable trace_file_status \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $CTRL.overwrite ""

radiobutton $CTRL.playback -text {PlayBack } \
	-padx 1 -pady 1 \
	-command "handleTraceFileStatus" \
	-bd $frame_border -relief sunken -value "PlayBack" \
	-variable trace_file_status \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $CTRL.playback ""

set trace_file_status [ get_initial_trace_file_status ]


#
# Place Playback Controls
#

set butt_y [ expr $border_space / 2 ]

place $ADJ				-relx 0.02 -y $butt_y

place $CTRL.rewind		-relx 0.27 -y $butt_y
place $CTRL.stop		-relx 0.35 -y $butt_y
place $CTRL.fwdstep		-relx 0.43 -y $butt_y
place $CTRL.fwd			-relx 0.51 -y $butt_y
place $CTRL.fastfwd		-relx 0.59 -y $butt_y

place $CTRL.time -anchor ne -relx 0.995 -y $butt_y

update

set x [ expr $ctrl_width - $border_space ]

set y [ expr [ below $ADJ ] + ($border_space / 2) ]

place $CTRL.overwrite -x $x -y $y -anchor ne

update

set x [ expr [ left $CTRL.overwrite ] - $border_space ]

place $CTRL.playback -x $x -y $y -anchor ne

update

set x [ expr $border_space / 2 ]

place $CTRL.file -x $x -y $y

update

set wt [ expr [ left $CTRL.playback ] - $x \
	- [ winfo width $CTRL.file ] - $border_space ]

set ht [ winfo height $CTRL.overwrite ]

place $CTRL.file_entry -x [ right $CTRL.file ] -y $y \
	-width $wt -height $ht

update


#
# Finish Main Panel Layout Constants
#

set ctrl_height [ expr [ winfo height $ADJ ] \
	+ [ winfo height $CTRL.file_entry ] + (2 * $border_space) ]

set height_left [ expr $main_height - $start_main_y - $ctrl_height ]

set net_height [ expr $height_left / 2 ]

set st_height [ expr $height_left - $net_height ]

set ctrl_y [ expr $net_y + $net_height ]

set st_y [ expr $ctrl_y + $ctrl_height ]


#
# Place Control Panel
#

place $CTRL -x $ctrl_x -y $ctrl_y \
	-width $ctrl_width -height $ctrl_height


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Network Canvas
#

set NET	.xpvm.network

set NET_C $NET.canvas

set NET_SBH $NET.horiz_sb
set NET_SBV $NET.vert_sb

set net_cheight [ expr $net_height - $scroll_width \
	- (2 * $row_height) - (2 * $border_space) ]

set net_cwidth [ expr $net_width - $scroll_width - (2 * $border_space)]

set net_hheight $net_size
set net_hwidth $net_size

frame $NET

restrict_bindings $NET "Frame"

label $NET.title -text {Network View} -foreground $fg_color

restrict_bindings $NET.title "Label"

set pcoord [ expr $net_size / 2 ]

set ncoord [ expr -1 * ( $net_size / 2 ) ]

canvas $NET_C -bd $frame_border -relief sunken \
	$XSCROLLINCREMENT $net_scroll $YSCROLLINCREMENT $net_scroll \
	-confine 0 -scrollregion "$ncoord $ncoord $pcoord $pcoord"

restrict_bindings $NET_C "Canvas"

set bg_color [ $NET_C config -background ]

set cmd [ list scrollCanvas $NET_C $NET_SBH \
	$net_size net_hwidth net_cwidth net_xview HORIZ center \
	$net_scroll ]

scrollbar $NET_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $NET_SBH "Scrollbar"

scrollInit $NET_C $NET_SBH $net_size $net_hwidth $net_cwidth \
	net_xview HORIZ center $net_scroll

set cmd [ list scrollCanvas $NET_C $NET_SBV \
	$net_size net_hheight net_cheight net_yview VERT center \
	$net_scroll ]

scrollbar $NET_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $NET_SBV "Scrollbar"

scrollInit $NET_C $NET_SBV $net_size $net_hheight $net_cheight \
	net_yview VERT center $net_scroll

set cmd [ list viewSelect network Network views_menu_state_Network \
	TRUE ]

button $NET.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $NET.close ""


#
# Create Network Key
#

label $NET.active_label -text {Active} \
	-foreground $fg_color

restrict_bindings $NET.active_label "Label"

label $NET.active_box -text {    } \
	-bd 1 -relief sunken -background $net_running_color

restrict_bindings $NET.active_box "Label"

label $NET.system_label -text {System} \
	-foreground $fg_color

restrict_bindings $NET.system_label "Label"

label $NET.system_box -text {    } \
	-bd 1 -relief sunken -background $net_system_color

restrict_bindings $NET.system_box "Label"

label $NET.empty_label -text {No Tasks} \
	-foreground $fg_color

restrict_bindings $NET.empty_label "Label"

label $NET.empty_box -text {    } \
	-bd 1 -relief sunken -background $net_empty_color

restrict_bindings $NET.empty_box "Label"

if { $depth != 1 } \
{
	button $NET.netkey -text {Net Key} -command "toggle_netkey" \
		-padx 1 -pady 1 \
		-foreground $fg_color -activeforeground $active_fg_color

	restrict_bindings $NET.netkey ""
}


#
# Layout Network View
#

layout_net_panel


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Network View Volume Color Key
#

if { $depth != 1 } \
{
	#
	# Create Toplevel for Network Key
	#

	set NK .network_key

	set nk_geom "[ expr $nk_width ]x[ expr $nk_height ]"

	toplevel $NK

	wm geometry $NK $nk_geom

	set min_nk_height [ expr $row_height \
		* ( $net_volume_color_size + 4 ) ]

	set min_nk_width 150

	wm minsize $NK $min_nk_width $min_nk_height


	#
	# Withdraw Window - Not Raised Initially
	#

	wm withdraw $NK


	#
	# Create Utilization View
	#

	set NK_F $NK.frame

	frame $NK_F

	restrict_bindings $NK_F "Frame"

	label $NK_F.title -text {Network Volume} -foreground $fg_color

	restrict_bindings $NK_F.title "Label"

	label $NK_F.label -text "Bytes:" -foreground $fg_color

	restrict_bindings $NK_F.label "Label"

	for { set i 0 } { $i < $net_volume_color_size } \
		{ set i [ expr $i + 1 ] } \
	{
		label $NK_F.label$i -text [get_net_volume_level $i] \
			-foreground $fg_color

		restrict_bindings $NK_F.label$i "Label"

		label $NK_F.key$i -text {    } \
			-bd 1 -relief sunken -background $net_volume_color($i)

		restrict_bindings $NK_F.key$i "Label"
	}

	button $NK_F.close -text {Close} -command "toggle_netkey" \
		-padx 1 -pady 1 \
		-foreground $fg_color -activeforeground $active_fg_color

	restrict_bindings $NK_F.close ""

	layout_nk_panel
} \
else \
	{ set NK "" }


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Space-Time View
#

set ST .xpvm.space_time

set ST_L $ST.labels

set ST_C $ST.canvas

set ST_SBH $ST.horiz_sb
set ST_SBV $ST.vert_sb

set st_lwidth [ expr $st_labels_width + $scroll_width ]

set st_cheight [ expr $st_height - $scroll_width \
	- (3 * $row_height) - (7 * $border_space / 2) ]

set st_cwidth [ expr $st_width - $st_lwidth - (2 * $border_space) ]

set tasks_height 0

frame $ST

restrict_bindings $ST "Frame"

label $ST.title -text {Space-Time: Tasks vs. Time} \
	-foreground $fg_color

restrict_bindings $ST.title "Label"

canvas $ST_L -bd $frame_border -relief sunken \
	$YSCROLLINCREMENT $st_label_ht -confine 0 \
	-scrollregion "0 0 $st_labels_width $st_canvas_height"

restrict_bindings $ST_L "Canvas"

canvas $ST_C -bd $frame_border -relief sunken \
	$XSCROLLINCREMENT 1 $YSCROLLINCREMENT 1 -confine 0 \
	-scrollregion "0 0 $scroll_canvas_width $st_canvas_height"

restrict_bindings $ST_C "Canvas"

set cmd [ list scrollViews ST 1 ]

scrollbar $ST_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $ST_SBH "Scrollbar"

scrollInit $ST_C $ST_SBH -1 $st_cwidth $st_cwidth st_xview \
	HORIZ low 1

set cmd [ list scrollTwoCanvases $ST_L $ST_C $ST_SBV \
	-1 tasks_height st_cheight stl_yview stc_yview VERT low \
	$st_label_ht 1 ]

scrollbar $ST_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $ST_SBV "Scrollbar"

scrollInit $ST_C $ST_SBV -1 $tasks_height $st_cheight stc_yview \
	VERT low 1

scrollInit $ST_L $ST_SBV -1 $tasks_height $st_cheight stl_yview \
	VERT low $st_label_ht

set st_timeline \
	[ $ST_C create line 0 0 0 $st_canvas_height -fill blue ]

label $ST.query_frame -bd $frame_border -relief sunken

restrict_bindings $ST.query_frame "Label"

label $ST.query_frame.query -text {View Info:} \
	-foreground $fg_color -anchor nw

restrict_bindings $ST.query_frame.query "Label"

set st_query_text_scroll 0

set st_query_text ""

button $ST.shrink -bitmap "@$xbm_dir/shrink.xbm" \
	-padx 1 -pady 1 \
	-command [ list change_st_height shrink ] \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $ST.shrink ""

button $ST.grow -bitmap "@$xbm_dir/grow.xbm" \
	-padx 1 -pady 1 \
	-command [ list change_st_height grow ] \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $ST.grow ""

entry $ST.task_height_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $ST.task_height_entry "change_st_height set"

restrict_bindings $ST.task_height_entry "Entry"

$ST.task_height_entry delete 0 end

$ST.task_height_entry insert 0 $st_label_ht

set cmd [ list viewSelect space_time {Space Time} \
	views_menu_state_Space_Time TRUE ]

button $ST.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $ST.close ""


#
# Create Space-Time Key
#

label $ST.computing_label -text {Computing} \
	-foreground $fg_color

restrict_bindings $ST.computing_label "Label"

label $ST.computing_box -text {    } \
	-bd 1 -relief sunken -background $st_running_color

restrict_bindings $ST.computing_box "Label"

label $ST.user_defined_label -text {User Defined} \
	-foreground $fg_color

restrict_bindings $ST.user_defined_label "Label"

label $ST.user_defined_box -text {    } \
	-bd 1 -relief sunken -background $st_user_defined_color

restrict_bindings $ST.user_defined_box "Label"

label $ST.overhead_label -text {Overhead} \
	-foreground $fg_color

restrict_bindings $ST.overhead_label "Label"

label $ST.overhead_box -text {    } \
	-bd 1 -relief sunken -background $st_system_color

restrict_bindings $ST.overhead_box "Label"

label $ST.waiting_label -text {Waiting} \
	-foreground $fg_color

restrict_bindings $ST.waiting_label "Label"

label $ST.waiting_box -text {    } \
	-bd 1 -relief sunken -background $st_idle_color

restrict_bindings $ST.waiting_box "Label"

label $ST.message_label -text {Message} \
	-foreground $fg_color

restrict_bindings $ST.message_label "Label"

label $ST.message_box -text {    } \
	-bd 1 -relief sunken -background $st_comm_color

restrict_bindings $ST.message_box "Label"

layout_st_panel


#
# Flush Main XPVM Window Creation
#

update


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Main File Menu
#

set file_info_list \
	[ list \
		[ list "Quit XPVM"			command	 		quit \
			"Quit XPVM, Leave PVM Running" ] \
		[ list "Halt PVM"			command	 		halt \
			"Quit XPVM, Halt PVM Virtual Machine" ] \
	]

makeMenu .xpvm.file_menu fileHandle $file_info_list lower {} none


#
# Create Main Hosts Menu
#

makeHostsMenu .xpvm.hosts_menu do_pvm_hosts [ get_hosts_menu_list ] \
	[ list raiseMenu .xpvm.hosts_menu .xpvm.hosts .xpvm $hosts_subs ] \
	$hosts_subs none


#
# Create Main Tasks Menu
#

set tasks_info_list \
	[ list \
		[ list "SPAWN"		submenu		.xpvm.spawn	\
			.xpvm.spawn.cmd_entry $spawn_subs \
			"Raise / Lower Task Spawning Dialog" ] \
		[ list "ON-THE-FLY"	submenu		.xpvm.otf		none $otf_subs \
			"Raise / Lower Task On-The-Fly Tracing Dialog" ] \
		[ list "KILL"		submenu		.xpvm.kill		none {} \
			"Raise / Lower Task Killing Dialog" ] \
		[ list "SIGNAL"		submenu		.xpvm.signal	none {} \
			"Raise / Lower Task Signaling Dialog" ] \
		[ list "SYS TASKS"	submenu		.xpvm.systasks_menu none {} \
			"Raise / Lower PVM System Tasks Menu" ] \
	]

makeMenu .xpvm.tasks_menu none $tasks_info_list \
	[ list raiseMenu .xpvm.tasks_menu .xpvm.tasks .xpvm $tasks_subs ] \
	$tasks_subs none


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Spawn Dialog Box
#

set spawn_width 400

set SP .xpvm.spawn

frame $SP -bd $frame_border -relief raised

restrict_bindings $SP "Frame"

label $SP.cmd -text {Command:} -foreground $fg_color

restrict_bindings $SP.cmd "Label"

entry $SP.cmd_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $SP.cmd_entry "do_spawn fresh"

restrict_bindings $SP.cmd_entry "Entry"

label $SP.flags -text {Flags:} -foreground $fg_color

restrict_bindings $SP.flags "Label"

checkbutton $SP.flags_debug -text {PvmTaskDebug} \
	-padx 1 -pady 1 \
	-onvalue ON -offvalue OFF -variable spawn_flags_debug \
	-bd $frame_border -relief flat \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.flags_debug ""

checkbutton $SP.flags_trace -text {PvmTaskTrace} \
	-padx 1 -pady 1 \
	-onvalue ON -offvalue OFF -variable spawn_flags_trace \
	-bd $frame_border -relief flat \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command layout_spawn_dialog

restrict_bindings $SP.flags_trace ""

label $SP.buf -text {Trace Buffer:} -foreground $fg_color

restrict_bindings $SP.buf "Label"

entry $SP.buf_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $SP.buf_entry "do_trace_buffer ; focus .xpvm"

restrict_bindings $SP.buf_entry "Entry"

$SP.buf_entry insert 0 0

button $SP.trace_mask -text {Trace Mask} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $SP.trace_mask ""

set trace_format [ get_trace_format ]

checkbutton $SP.flags_front -text {PvmMppFront} \
	-padx 1 -pady 1 \
	-onvalue ON -offvalue OFF -variable spawn_flags_front \
	-bd $frame_border -relief flat \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.flags_front ""

checkbutton $SP.flags_compl -text {PvmHostCompl} \
	-padx 1 -pady 1 \
	-onvalue ON -offvalue OFF -variable spawn_flags_compl \
	-bd $frame_border -relief flat \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.flags_compl ""

set spawn_flags_trace ON

label $SP.where -text {Where:} -foreground $fg_color

restrict_bindings $SP.where "Label"

radiobutton $SP.where_host -text {Host} \
	-padx 1 -pady 1 \
	-command checkSpawnWhere \
	-bd $frame_border -relief flat -value "Host" \
	-variable spawn_where \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.where_host ""

radiobutton $SP.where_arch -text {Architecture} \
	-padx 1 -pady 1 \
	-command checkSpawnWhere \
	-bd $frame_border -relief flat -value "Arch" \
	-variable spawn_where \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.where_arch ""

radiobutton $SP.where_default -text {Default} \
	-padx 1 -pady 1 \
	-command checkSpawnWhere \
	-bd $frame_border -relief flat -value "Default" \
	-variable spawn_where \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.where_default ""

set spawn_where Default

set last_spawn_where Default

label $SP.loc -text {Host:} -foreground $fg_color

restrict_bindings $SP.loc "Label"

entry $SP.loc_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $SP.loc_entry "focus .xpvm"

restrict_bindings $SP.loc_entry "Entry"

set spawn_host_string ""
set spawn_arch_string ""

label $SP.ntasks -text {NTasks:} -foreground $fg_color

restrict_bindings $SP.ntasks "Label"

entry $SP.ntasks_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $SP.ntasks_entry "focus .xpvm"

restrict_bindings $SP.ntasks_entry "Entry"

$SP.ntasks_entry insert 0 1

checkbutton $SP.close_on_start -text {Close on Start} \
	-padx 1 -pady 1 \
	-onvalue ON -offvalue OFF -variable spawn_close_on_start \
	-bd $frame_border -relief flat \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.close_on_start ""

set spawn_close_on_start "ON"

set cmd [ list raiseSubMenu $SP .xpvm.tasks_menu \
	.xpvm.tasks_menu.butt_SPAWN .xpvm.spawn.cmd_entry $spawn_subs ]

button $SP.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SP.close ""

button $SP.start_append -text {Start Append} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_spawn append ]

restrict_bindings $SP.start_append ""

button $SP.start -text {Start} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_spawn fresh ]

restrict_bindings $SP.start ""

layout_spawn_dialog

place forget $SP


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create On-The-Fly Dialog Box
#

set otf_height 300
set otf_width 380

set otf_canvas_height 200
set otf_canvas_width 80

set otf_tasklist none

set OTF .xpvm.otf

set OTF_C $OTF.canvas

set OTF_SBH $OTF.horiz_sb
set OTF_SBV $OTF.vert_sb

# Main Frame

frame $OTF -bd $frame_border -relief raised

restrict_bindings $OTF "Frame"

# Scrolling Task Window

label $OTF.tasks -text {Tasks:} -foreground $fg_color

restrict_bindings $OTF.tasks "Label"

set otf_cheight [ expr $otf_height - $scroll_width \
	- (2 * $row_height) - (3 * $border_space) ]

set otf_cwidth [ expr $otf_width - $scroll_width - (2 * $border_space) ]

canvas $OTF_C -bd $frame_border -relief sunken \
	$YSCROLLINCREMENT $row_height -confine 0 \
	-scrollregion "0 0 $otf_canvas_height $otf_canvas_width"

restrict_bindings $OTF_C "Canvas"

set cmd [ list scrollCanvas $OTF_C $OTF_SBH \
	-1 otf_canvas_width otf_cwidth otf_xview HORIZ low $row_height ]

scrollbar $OTF_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $OTF_SBH "Scrollbar"

scrollInit $OTF_C $OTF_SBH -1 \
	$otf_canvas_width $otf_cwidth otf_xview HORIZ low $row_height

set cmd [ list scrollCanvas $OTF_C $OTF_SBV \
	-1 otf_lasty otf_cheight otf_yview VERT low $row_height ]

set otf_lasty 0

scrollbar $OTF_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $OTF_SBV "Scrollbar"

scrollInit $OTF_C $OTF_SBV -1 \
	$otf_lasty $otf_cheight otf_yview VERT low $row_height

# Trace Settings

label $OTF.tracing -text {Set Tracing:} -foreground $fg_color

restrict_bindings $OTF.tracing "Label"

radiobutton $OTF.trace_on -text {On} \
	-padx 1 -pady 1 \
	-command "layout_otf_dialog" \
	-bd $frame_border -relief sunken -value "On" \
	-variable tracing_status \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OTF.trace_on ""

radiobutton $OTF.trace_off -text {Off} \
	-padx 1 -pady 1 \
	-command "layout_otf_dialog" \
	-bd $frame_border -relief sunken -value "Off" \
	-variable tracing_status \
	$SELECTOR $selector_color -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OTF.trace_off ""

set tracing_status "On"

button $OTF.trace_mask -text {Trace Mask} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $OTF.trace_mask ""

# Control Buttons

set cmd [ list raiseSubMenu $OTF .xpvm.tasks_menu \
	.xpvm.tasks_menu.butt_ON_THE_FLY none $otf_subs ]

button $OTF.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OTF.close ""

button $OTF.refresh -text {Refresh Tasks} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_otf refresh ]

restrict_bindings $OTF.refresh ""

button $OTF.otf_all -text {Adjust All Tasks} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_otf all ]

restrict_bindings $OTF.otf_all ""

layout_otf_dialog

place forget $OTF


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Kill Dialog Box
#

set kill_height 300
set kill_width 300

set kill_canvas_height 200
set kill_canvas_width 80

set kill_tasklist none

set KL .xpvm.kill

set KL_C $KL.canvas

set KL_SBH $KL.horiz_sb
set KL_SBV $KL.vert_sb

# Main Frame

frame $KL -bd $frame_border -relief raised

restrict_bindings $KL "Frame"

# Scrolling Task Window

label $KL.tasks -text {Tasks:} -foreground $fg_color

restrict_bindings $KL.tasks "Label"

set kill_cheight [ expr $kill_height - $scroll_width \
	- (2 * $row_height) - (3 * $border_space) ]

set kill_cwidth [ expr $kill_width - $scroll_width \
	- (2 * $border_space)]

canvas $KL_C -bd $frame_border -relief sunken \
	$YSCROLLINCREMENT $row_height -confine 0 \
	-scrollregion "0 0 $kill_canvas_height $kill_canvas_width"

restrict_bindings $KL_C "Canvas"

set cmd [ list scrollCanvas $KL_C $KL_SBH \
	-1 kill_canvas_width kill_cwidth kill_xview HORIZ low $row_height ]

scrollbar $KL_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $KL_SBH "Scrollbar"

scrollInit $KL_C $KL_SBH -1 \
	$kill_canvas_width $kill_cwidth kill_xview HORIZ low $row_height

set cmd [ list scrollCanvas $KL_C $KL_SBV \
	-1 kill_lasty kill_cheight kill_yview VERT low $row_height ]

set kill_lasty 0

scrollbar $KL_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $KL_SBV "Scrollbar"

scrollInit $KL_C $KL_SBV -1 \
	$kill_lasty $kill_cheight kill_yview VERT low $row_height

# Control Buttons

set cmd [ list raiseSubMenu $KL .xpvm.tasks_menu \
	.xpvm.tasks_menu.butt_KILL none {} ]

button $KL.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $KL.close ""

button $KL.refresh -text {Refresh Tasks} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_kill refresh ]

restrict_bindings $KL.refresh ""

button $KL.kill_all -text {Kill All} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_kill all ]

restrict_bindings $KL.kill_all ""

layout_kill_dialog

place forget $KL


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Signal Dialog Box
#

set num_signals 31

set signal_height 300
set signal_width 375

set signal_canvas_height 200
set signal_canvas_width 80

set signal_tasklist none

set signal_list_height \
	[ expr ($num_signals * $row_height) + (2 * $frame_border) ]

set signal_list_width [ expr 12 * $col_width ]

set SG .xpvm.signal

set SG_C $SG.canvas

set SG_SBH $SG.horiz_sb
set SG_SBV $SG.vert_sb

set SG_LF $SG.list_frame
set SG_LFC $SG_LF.canvas
set SG_LFCW $SG_LFC.win

set SG_LSBV $SG.list_vert_sb

# Main Frame

frame $SG -bd $frame_border -relief raised

restrict_bindings $SG "Frame"

# Scrolling Task Window

label $SG.tasks -text {Tasks:} -foreground $fg_color

restrict_bindings $SG.tasks "Label"

set signal_cheight [ expr $signal_height - $scroll_width \
	- (2 * $row_height) - (3 * $border_space) ]

set signal_cwidth [ expr $signal_width - (2 * $scroll_width) \
	- $signal_list_width - (3 * $border_space)]

set signal_lcheight [ expr $signal_cheight + $scroll_width ]

canvas $SG_C -bd $frame_border -relief sunken \
	$YSCROLLINCREMENT $row_height -confine 0 \
	-scrollregion "0 0 $signal_canvas_height $signal_canvas_width"

restrict_bindings $SG_C "Canvas"

set signal_win_width [ expr $signal_canvas_width * $col_width ]

set cmd [ list scrollCanvas $SG_C $SG_SBH \
	-1 signal_win_width signal_cwidth signal_xview HORIZ low \
	$row_height ]

scrollbar $SG_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $SG_SBH "Scrollbar"

scrollInit $SG_C $SG_SBH -1 \
	$signal_win_width $signal_cwidth signal_xview \
	HORIZ low $row_height

set cmd [ list scrollCanvas $SG_C $SG_SBV \
	-1 signal_lasty signal_cheight signal_yview VERT low $row_height ]

set signal_lasty 0

scrollbar $SG_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $SG_SBV "Scrollbar"

scrollInit $SG_C $SG_SBV -1 \
	$signal_lasty $signal_cheight signal_yview VERT low $row_height

# Scrolling Signal Window

label $SG.signals -text {Signals:} -foreground $fg_color

restrict_bindings $SG.signals "Label"

frame $SG_LF -relief sunken -bd $frame_border

restrict_bindings $SG_LF "Frame"

canvas $SG_LFC -relief flat \
	$YSCROLLINCREMENT $row_height -confine 0 \
	-scrollregion "0 0 $signal_canvas_height $num_signals"

restrict_bindings $SG_LFC "Canvas"

frame $SG_LFCW

restrict_bindings $SG_LFCW "Frame"

set signal_canvas_list_win \
	[ $SG_LFC create window $frame_border $frame_border -anchor nw \
		-width $signal_list_width ]

signal_button hup		"SIGHUP         "
signal_button int		"SIGINT         "
signal_button quit		"SIGQUIT        "
signal_button ill		"SIGILL         "
signal_button trap		"SIGTRAP        "
signal_button abrt		"SIGABRT        "
signal_button emt		"SIGEMT         "
signal_button fpe		"SIGFPE         "
signal_button kill		"SIGKILL        "
signal_button bus		"SIGBUS         "
signal_button segv		"SIGSEGV        "
signal_button sys		"SIGSYS         "
signal_button pipe		"SIGPIPE        "
signal_button alrm		"SIGALRM        "
signal_button term		"SIGTERM        "
signal_button urg		"SIGURG         "
signal_button stop		"SIGSTOP        "
signal_button tstp		"SIGTSTP        "
signal_button cont		"SIGCONT        "
signal_button chld		"SIGCHLD        "
signal_button ttin		"SIGTTIN        "
signal_button ttou		"SIGTTOU        "
signal_button io		"SIGIO          "
signal_button xcpu		"SIGXCPU        "
signal_button xfsz		"SIGXFSZ        "
signal_button vtalrm	"SIGVTALRM      "
signal_button prof		"SIGPROF        "
signal_button winch		"SIGWINCH       "
signal_button lost		"SIGLOST        "
signal_button usr1		"SIGUSR1        "
signal_button usr2		"SIGUSR2        "

set signal_selected none

set cmd [ list scrollCanvas $SG_LFC $SG_LSBV \
	-1 signal_list_height signal_lcheight signal_list_yview \
	VERT low $row_height ]

scrollbar $SG_LSBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $SG_LSBV "Scrollbar"

scrollInit $SG_LFC $SG_LSBV -1 \
	$signal_list_height $signal_lcheight signal_list_yview \
	VERT low $row_height

# Control Buttons

set cmd [ list raiseSubMenu $SG .xpvm.tasks_menu \
	.xpvm.tasks_menu.butt_SIGNAL none {} ]

button $SG.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $SG.close ""

button $SG.refresh -text {Refresh Tasks} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_signal refresh ]

restrict_bindings $SG.refresh ""

button $SG.signal_all -text {Signal All} \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color \
	-command [ list do_signal all ]

restrict_bindings $SG.signal_all ""

layout_signal_dialog

place forget $SG


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create PVM 3.4 Trace Mask Menus
#

set trace_mask_info_list \
	[ list \
		[ list "ALL 3.4 EVENTS"		checkbutton	"ALL_3_4_EVENTS" OFF \
			"Set Trace Mask Entries for Every 3.4 Event On / Off" ] \
		[ list "USER DEFINED"		checkbutton	"USER_DEFINED" OFF \
			"Set Trace Mask Entry for User Defined Events On / Off" ] \
		[ list "Host Events"	exchangeIndirect \
			.xpvm.host_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Host Events" ] \
		[ list "Group Events"	exchangeIndirect \
			.xpvm.group_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Group Events" ] \
		[ list "Task Events"	exchangeIndirect \
			.xpvm.task_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Task Events" ] \
		[ list "Context Events"	exchangeIndirect \
			.xpvm.ctxt_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Context Events" ] \
		[ list "Comm Events"	exchangeIndirect \
			.xpvm.comm_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Communication Events" ] \
		[ list "Collect Events"	exchangeIndirect \
			.xpvm.coll_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Collective Comm Events" ] \
		[ list "Mailbox Events"	exchangeIndirect \
			.xpvm.mbox_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Message Mailbox Events" ] \
		[ list "Buffer Events"	exchangeIndirect \
			.xpvm.buff_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Message Buffer Events" ] \
		[ list "Pack Events"	exchangeIndirect \
			.xpvm.pack_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Message Packing Events" ] \
		[ list "Unpack Events"	exchangeIndirect \
			.xpvm.unpack_ev_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Message Unpacking Events" ] \
		[ list "Info Events"	exchangeIndirect \
			.xpvm.info_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Informational Events" ] \
		[ list "Misc Events"	exchangeIndirect \
			.xpvm.misc_ev_menu		trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.4 Miscellaneous Events" ] \
	]

makeMenu .xpvm.trace_mask_menu trace_mask $trace_mask_info_list \
	lower {} trace_mask

### Note, no define_trace_mask for ALL_EVENTS, hard-wired, obvious


#
# Tick
#

puts -nonewline "."
flush stdout


set host_ev_info_list \
	[ list \
		[ list "3.4 Host Events:"		label	 	none ] \
		[ list "ALL HOST EVENTS"	checkbutton	"ALL_HOST_EVENTS" OFF \
			"Set Trace Mask Entries for Every Host Event On / Off" ] \
		[ list "ADDHOSTS"			checkbutton	"ADDHOSTS" OFF \
			"Toggle Trace Mask Entry for ADDHOSTS Event" ] \
		[ list "DELHOSTS"			checkbutton	"DELHOSTS" OFF \
			"Toggle Trace Mask Entry for DELHOSTS Event" ] \
		[ list "CONFIG"				checkbutton	"CONFIG" OFF \
			"Toggle Trace Mask Entry for CONFIG Event" ] \
		[ list "MSTAT"				checkbutton	"MSTAT" OFF \
			"Toggle Trace Mask Entry for MSTAT Event" ] \
		[ list "ARCHCODE"			checkbutton "ARCHCODE" OFF \
			"Toggle Trace Mask Entry for ARCHCODE Event" ] \
		[ list "HOSTSYNC"			checkbutton "HOSTSYNC" OFF \
			"Toggle Trace Mask Entry for HOSTSYNC Event" ] \
	]

define_trace_mask 3.4 ALL_HOST_EVENTS \
	[ list ADDHOSTS DELHOSTS CONFIG MSTAT ARCHCODE HOSTSYNC ]

set close [ list exchangeMenuIndirect .xpvm.host_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.host_ev_menu trace_mask $host_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set group_ev_info_list \
	[ list \
		[ list "3.4 Group Events:"		label	 	none ] \
		[ list "ALL GROUP EVENTS"	checkbutton	"ALL_GROUP_EVENTS" OFF \
			"Set Trace Mask Entries for Every Group Event On / Off" ] \
		[ list "BARRIER"			checkbutton	"BARRIER" OFF \
			"Toggle Trace Mask Entry for BARRIER Event" ] \
		[ list "BCAST"				checkbutton	"BCAST" OFF \
			"Toggle Trace Mask Entry for BCAST Event" ] \
		[ list "DELETE"				checkbutton	"DELETE" OFF \
			"Toggle Trace Mask Entry for DELETE Event" ] \
		[ list "GETINST"			checkbutton	"GETINST" OFF \
			"Toggle Trace Mask Entry for GETINST Event" ] \
		[ list "GETTID"				checkbutton	"GETTID" OFF \
			"Toggle Trace Mask Entry for GETTID Event" ] \
		[ list "GSIZE"				checkbutton	"GSIZE" OFF \
			"Toggle Trace Mask Entry for GSIZE Event" ] \
		[ list "JOINGROUP"			checkbutton	"JOINGROUP" OFF \
			"Toggle Trace Mask Entry for JOINGROUP Event" ] \
		[ list "LVGROUP"			checkbutton	"LVGROUP" OFF \
			"Toggle Trace Mask Entry for LVGROUP Event" ] \
	]

define_trace_mask 3.4 ALL_GROUP_EVENTS \
	[ list BARRIER BCAST DELETE GETINST GETTID \
		GSIZE JOINGROUP LVGROUP ]

set close [ list exchangeMenuIndirect .xpvm.group_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.group_ev_menu trace_mask $group_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set task_ev_info_list \
	[ list \
		[ list "3.4 Task Events:"		label	 	none ] \
		[ list "ALL TASK EVENTS"	checkbutton	"ALL_TASK_EVENTS" OFF \
			"Set Trace Mask Entries for Every Task Event On / Off" ] \
		[ list "MYTID"				checkbutton	"MYTID" OFF \
			"Toggle Trace Mask Entry for MYTID Event" ] \
		[ list "PARENT"				checkbutton	"PARENT" OFF \
			"Toggle Trace Mask Entry for PARENT Event" ] \
		[ list "SIBLINGS"			checkbutton	"SIBLINGS" OFF \
			"Toggle Trace Mask Entry for SIBLINGS Event" ] \
		[ list "SPAWN"				checkbutton	"SPAWN" OFF \
			"Toggle Trace Mask Entry for SPAWN Event" ] \
		[ list "SENDSIG"			checkbutton	"SENDSIG" OFF \
			"Toggle Trace Mask Entry for SENDSIG Event" ] \
		[ list "KILL"				checkbutton	"KILL" OFF \
			"Toggle Trace Mask Entry for KILL Event" ] \
		[ list "EXIT"				checkbutton	"EXIT" OFF \
			"Toggle Trace Mask Entry for EXIT Event" ] \
	]

define_trace_mask 3.4 ALL_TASK_EVENTS \
	[ list MYTID PARENT SIBLINGS SPAWN SENDSIG KILL EXIT ]

set close [ list exchangeMenuIndirect .xpvm.task_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.task_ev_menu trace_mask $task_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set ctxt_ev_info_list \
	[ list \
		[ list "3.4 Context Events:"		label	 	none ] \
		[ list "ALL CONTEXT EVENTS"	checkbutton	\
			"ALL_CONTEXT_EVENTS" OFF \
		"Set Trace Mask Entries for Every Context Event On / Off" ] \
		[ list "NEWCONTEXT"		checkbutton	"NEWCONTEXT" OFF \
			"Toggle Trace Mask Entry for NEWCONTEXT Event" ] \
		[ list "SETCONTEXT"		checkbutton	"SETCONTEXT" OFF \
			"Toggle Trace Mask Entry for SETCONTEXT Event" ] \
		[ list "GETCONTEXT"		checkbutton	"GETCONTEXT" OFF \
			"Toggle Trace Mask Entry for GETCONTEXT Event" ] \
		[ list "FREECONTEXT"	checkbutton	"FREECONTEXT" OFF \
			"Toggle Trace Mask Entry for FREECONTEXT Event" ] \
	]

define_trace_mask 3.4 ALL_CONTEXT_EVENTS \
	[ list NEWCONTEXT SETCONTEXT GETCONTEXT FREECONTEXT ]

set close [ list exchangeMenuIndirect .xpvm.ctxt_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.ctxt_ev_menu trace_mask $ctxt_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set comm_ev_info_list \
	[ list \
		[ list "3.4 Comm Events:"		label	 	none ] \
		[ list "ALL COMM EVENTS"	checkbutton	"ALL_COMM_EVENTS" OFF \
	"Set Trace Mask Entries for Every Communication Event On / Off" ] \
		[ list "MCAST"				checkbutton	"MCAST" OFF \
			"Toggle Trace Mask Entry for MCAST Event" ] \
		[ list "SEND"				checkbutton	"SEND" OFF \
			"Toggle Trace Mask Entry for SEND Event" ] \
		[ list "RECV"				checkbutton	"RECV" OFF \
			"Toggle Trace Mask Entry for RECV Event" ] \
		[ list "NRECV"				checkbutton	"NRECV" OFF \
			"Toggle Trace Mask Entry for NRECV Event" ] \
		[ list "RECVF"				checkbutton	"RECVF" OFF \
			"Toggle Trace Mask Entry for RECVF Event" ] \
		[ list "PSEND"				checkbutton	"PSEND" OFF \
			"Toggle Trace Mask Entry for PSEND Event" ] \
		[ list "PRECV"				checkbutton	"PRECV" OFF \
			"Toggle Trace Mask Entry for PRECV Event" ] \
		[ list "TRECV"				checkbutton	"TRECV" OFF \
			"Toggle Trace Mask Entry for TRECV Event" ] \
		[ list "PROBE"				checkbutton	"PROBE" OFF \
			"Toggle Trace Mask Entry for PROBE Event" ] \
		[ list "ADDMHF"				checkbutton	"ADDMHF" OFF \
			"Toggle Trace Mask Entry for ADDMHF Event" ] \
		[ list "DELMHF"				checkbutton	"DELMHF" OFF \
			"Toggle Trace Mask Entry for DELMHF Event" ] \
		[ list "MHF_INVOKE"			checkbutton	"MHF_INVOKE" OFF \
			"Toggle Trace Mask Entry for MHF_INVOKE Event" ] \
	]

define_trace_mask 3.4 ALL_COMM_EVENTS \
	[ list MCAST SEND RECV NRECV RECVF \
		PSEND PRECV TRECV PROBE ADDMHF DELMHF MHF_INVOKE ]

set close [ list exchangeMenuIndirect .xpvm.comm_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.comm_ev_menu trace_mask $comm_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set coll_ev_info_list \
	[ list \
		[ list "3.4 Collective Events:"		label	 	none ] \
		[ list "ALL COLL EVENTS"	checkbutton	"ALL_COLL_EVENTS" OFF \
"Set Trace Mask Entries for Every Coll Communication Event On / Off" ] \
		[ list "GATHER"				checkbutton	"GATHER" OFF \
			"Toggle Trace Mask Entry for GATHER Event" ] \
		[ list "SCATTER"			checkbutton	"SCATTER" OFF \
			"Toggle Trace Mask Entry for SCATTER Event" ] \
		[ list "REDUCE"				checkbutton	"REDUCE" OFF \
			"Toggle Trace Mask Entry for REDUCE Event" ] \
	]

define_trace_mask 3.4 ALL_COLL_EVENTS \
	[ list GATHER SCATTER REDUCE ]

set close [ list exchangeMenuIndirect .xpvm.coll_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.coll_ev_menu trace_mask $coll_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set mbox_ev_info_list \
	[ list \
		[ list "3.4 Mailbox Events:"		label	 	none ] \
		[ list "ALL MAILBOX EVENTS"		checkbutton	\
			"ALL_MAILBOX_EVENTS" OFF \
"Set Trace Mask Entries for Every Message Mailbox Event On / Off" ] \
		[ list "PUTINFO"			checkbutton	"PUTINFO" OFF \
			"Toggle Trace Mask Entry for PUTINFO Event" ] \
		[ list "GETINFO"			checkbutton	"GETINFO" OFF \
			"Toggle Trace Mask Entry for GETINFO Event" ] \
		[ list "DELINFO"			checkbutton	"DELINFO" OFF \
			"Toggle Trace Mask Entry for DELINFO Event" ] \
		[ list "GETMBOXINFO"		checkbutton	"GETMBOXINFO" OFF \
			"Toggle Trace Mask Entry for GETMBOXINFO Event" ] \
	]

define_trace_mask 3.4 ALL_MAILBOX_EVENTS \
	[ list PUTINFO GETINFO DELINFO GETMBOXINFO ]

set close [ list exchangeMenuIndirect .xpvm.mbox_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.mbox_ev_menu trace_mask $mbox_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set buff_ev_info_list \
	[ list \
		[ list "3.4 Buffer Events:"		label	 	none ] \
		[ list "ALL BUF EVENTS"		checkbutton	"ALL_BUF_EVENTS" OFF \
	"Set Trace Mask Entries for Every Message Buffer Event On / Off" ] \
		[ list "INITSEND"			checkbutton	"INITSEND" OFF \
			"Toggle Trace Mask Entry for INITSEND Event" ] \
		[ list "MKBUF"				checkbutton	"MKBUF" OFF \
			"Toggle Trace Mask Entry for MKBUF Event" ] \
		[ list "BUFINFO"			checkbutton	"BUFINFO" OFF \
			"Toggle Trace Mask Entry for BUFINFO Event" ] \
		[ list "FREEBUF"			checkbutton	"FREEBUF" OFF \
			"Toggle Trace Mask Entry for FREEBUF Event" ] \
		[ list "GETRBUF"			checkbutton	"GETRBUF" OFF \
			"Toggle Trace Mask Entry for GETRBUF Event" ] \
		[ list "SETRBUF"			checkbutton	"SETRBUF" OFF \
			"Toggle Trace Mask Entry for SETRBUF Event" ] \
		[ list "GETSBUF"			checkbutton	"GETSBUF" OFF \
			"Toggle Trace Mask Entry for GETSBUF Event" ] \
		[ list "SETSBUF"			checkbutton	"SETSBUF" OFF \
			"Toggle Trace Mask Entry for SETSBUF Event" ] \
		[ list "GETMWID"			checkbutton	"GETMWID" OFF \
			"Toggle Trace Mask Entry for GETMWID Event" ] \
		[ list "SETMWID"			checkbutton	"SETMWID" OFF \
			"Toggle Trace Mask Entry for SETMWID Event" ] \
	]

define_trace_mask 3.4 ALL_BUF_EVENTS \
	[ list INITSEND MKBUF BUFINFO FREEBUF \
		GETRBUF SETRBUF GETSBUF SETSBUF GETMWID SETMWID ]

set close [ list exchangeMenuIndirect .xpvm.buff_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.buff_ev_menu trace_mask $buff_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set pack_ev_info_list \
	[ list \
		[ list "3.4 Packing Events:"	label	 	none ] \
		[ list "ALL PACK EVENTS"	checkbutton	"ALL_PACK_EVENTS" OFF \
"Set Trace Mask Entries for Every Message Packing Event On / Off" ] \
		[ list "PKBYTE"				checkbutton	"PKBYTE" OFF \
			"Toggle Trace Mask Entry for PKBYTE Event" ] \
		[ list "PKCPLX"				checkbutton	"PKCPLX" OFF \
			"Toggle Trace Mask Entry for PKCPLX Event" ] \
		[ list "PKDCPLX"			checkbutton	"PKDCPLX" OFF \
			"Toggle Trace Mask Entry for PKDCPLX Event" ] \
		[ list "PKDOUBLE"			checkbutton	"PKDOUBLE" OFF \
			"Toggle Trace Mask Entry for PKDOUBLE Event" ] \
		[ list "PKFLOAT"			checkbutton	"PKFLOAT" OFF \
			"Toggle Trace Mask Entry for PKFLOAT Event" ] \
		[ list "PKINT"				checkbutton	"PKINT" OFF \
			"Toggle Trace Mask Entry for PKINT Event" ] \
		[ list "PKUINT"				checkbutton	"PKUINT" OFF \
			"Toggle Trace Mask Entry for PKUINT Event" ] \
		[ list "PKLONG"				checkbutton	"PKLONG" OFF \
			"Toggle Trace Mask Entry for PKLONG Event" ] \
		[ list "PKULONG"			checkbutton	"PKULONG" OFF \
			"Toggle Trace Mask Entry for PKULONG Event" ] \
		[ list "PKSHORT"			checkbutton	"PKSHORT" OFF \
			"Toggle Trace Mask Entry for PKSHORT Event" ] \
		[ list "PKUSHORT"			checkbutton	"PKUSHORT" OFF \
			"Toggle Trace Mask Entry for PKUSHORT Event" ] \
		[ list "PKSTR"				checkbutton	"PKSTR" OFF \
			"Toggle Trace Mask Entry for PKSTR Event" ] \
		[ list "PACKF"				checkbutton	"PACKF" OFF \
			"Toggle Trace Mask Entry for PACKF Event" ] \
	]

define_trace_mask 3.4 ALL_PACK_EVENTS \
	[ list PKBYTE PKCPLX PKDCPLX PKDOUBLE PKFLOAT \
		PKINT PKUINT PKLONG PKULONG PKSHORT PKUSHORT PKSTR PACKF ]

set close [ list exchangeMenuIndirect .xpvm.pack_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.pack_ev_menu trace_mask $pack_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set unpack_ev_info_list \
	[ list \
		[ list "3.4 Unpacking Events:"	label	 	none ] \
		[ list "ALL UNPACK EVENTS"	checkbutton	"ALL_UNPACK_EVENTS" \
			OFF \
"Set Trace Mask Entries for Every Message Unpacking Event On / Off" ] \
		[ list "UPKBYTE"			checkbutton	"UPKBYTE" OFF \
			"Toggle Trace Mask Entry for UPKBYTE Event" ] \
		[ list "UPKCPLX"			checkbutton	"UPKCPLX" OFF \
			"Toggle Trace Mask Entry for UPKCPLX Event" ] \
		[ list "UPKDCPLX"			checkbutton	"UPKDCPLX" OFF \
			"Toggle Trace Mask Entry for UPKDCPLX Event" ] \
		[ list "UPKDOUBLE"			checkbutton	"UPKDOUBLE" OFF \
			"Toggle Trace Mask Entry for UPKDOUBLE Event" ] \
		[ list "UPKFLOAT"			checkbutton	"UPKFLOAT" OFF \
			"Toggle Trace Mask Entry for UPKFLOAT Event" ] \
		[ list "UPKINT"				checkbutton	"UPKINT" OFF \
			"Toggle Trace Mask Entry for UPKINT Event" ] \
		[ list "UPKUINT"			checkbutton	"UPKUINT" OFF \
			"Toggle Trace Mask Entry for UPKUINT Event" ] \
		[ list "UPKLONG"			checkbutton	"UPKLONG" OFF \
			"Toggle Trace Mask Entry for UPKLONG Event" ] \
		[ list "UPKULONG"			checkbutton	"UPKULONG" OFF \
			"Toggle Trace Mask Entry for UPKULONG Event" ] \
		[ list "UPKSHORT"			checkbutton	"UPKSHORT" OFF \
			"Toggle Trace Mask Entry for UPKSHORT Event" ] \
		[ list "UPKUSHORT"			checkbutton	"UPKUSHORT" OFF \
			"Toggle Trace Mask Entry for UPKUSHORT Event" ] \
		[ list "UPKSTR"				checkbutton	"UPKSTR" OFF \
			"Toggle Trace Mask Entry for UPKSTR Event" ] \
		[ list "UNPACKF"			checkbutton	"UNPACKF" OFF \
			"Toggle Trace Mask Entry for UNPACKF Event" ] \
	]

define_trace_mask 3.4 ALL_UNPACK_EVENTS \
	[ list UPKBYTE UPKCPLX UPKDCPLX UPKDOUBLE UPKFLOAT \
		UPKINT UPKUINT UPKLONG UPKULONG UPKSHORT UPKUSHORT UPKSTR \
		UNPACKF ]

set close [ list exchangeMenuIndirect .xpvm.unpack_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.unpack_ev_menu trace_mask $unpack_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set info_ev_info_list \
	[ list \
		[ list "3.4 Info Events:"		label	 	none ] \
		[ list "ALL INFO EVENTS"	checkbutton	"ALL_INFO_EVENTS" OFF \
	"Set Trace Mask Entries for Every Informational Event On / Off" ] \
		[ list "PSTAT"				checkbutton	"PSTAT" OFF \
			"Toggle Trace Mask Entry for PSTAT Event" ] \
		[ list "TASKS"				checkbutton	"TASKS" OFF \
			"Toggle Trace Mask Entry for TASKS Event" ] \
		[ list "GETOPT"				checkbutton	"GETOPT" OFF \
			"Toggle Trace Mask Entry for GETOPT Event" ] \
		[ list "SETOPT"				checkbutton	"SETOPT" OFF \
			"Toggle Trace Mask Entry for SETOPT Event" ] \
		[ list "TIDTOHOST"			checkbutton	"TIDTOHOST" OFF \
			"Toggle Trace Mask Entry for TIDTOHOST Event" ] \
		[ list "GETFDS"				checkbutton	"GETFDS" OFF \
			"Toggle Trace Mask Entry for GETFDS Event" ] \
	]

define_trace_mask 3.4 ALL_INFO_EVENTS \
	[ list PSTAT TASKS GETOPT SETOPT TIDTOHOST GETFDS ]

set close [ list exchangeMenuIndirect .xpvm.info_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.info_ev_menu trace_mask $info_ev_info_list \
	$close {} trace_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set misc_ev_info_list \
	[ list \
		[ list "3.4 Misc Events:"		label	 	none ] \
		[ list "ALL MISC EVENTS"	checkbutton	"ALL_MISC_EVENTS" OFF \
	"Set Trace Mask Entries for Every Miscellaneous Event On / Off" ] \
		[ list "START PVMD"			checkbutton	"START_PVMD" OFF \
			"Toggle Trace Mask Entry for START PVMD Event" ] \
		[ list "REG HOSTER"			checkbutton	"REG_HOSTER" OFF \
			"Toggle Trace Mask Entry for REG HOSTER Event" ] \
		[ list "REG RM"				checkbutton	"REG_RM" OFF \
			"Toggle Trace Mask Entry for REG RM Event" ] \
		[ list "REG TASKER"			checkbutton	"REG_TASKER" OFF \
			"Toggle Trace Mask Entry for REG TASKER Event" ] \
		[ list "REG TRACER"			checkbutton	"REG_TRACER" OFF \
			"Toggle Trace Mask Entry for REG TRACER Event" ] \
		[ list "CATCHOUT"			checkbutton	"CATCHOUT" OFF \
			"Toggle Trace Mask Entry for CATCHOUT Event" ] \
		[ list "NOTIFY"				checkbutton	"NOTIFY" OFF \
			"Toggle Trace Mask Entry for NOTIFY Event" ] \
		[ list "GETTMASK"			checkbutton	"GETTMASK" OFF \
			"Toggle Trace Mask Entry for GETTMASK Event" ] \
		[ list "SETTMASK"			checkbutton	"SETTMASK" OFF \
			"Toggle Trace Mask Entry for SETTMASK Event" ] \
		[ list "PERROR"				checkbutton	"PERROR" OFF \
			"Toggle Trace Mask Entry for PERROR Event" ] \
		[ list "TICKLE"				checkbutton	"TICKLE" OFF \
			"Toggle Trace Mask Entry for TICKLE Event" ] \
		[ list "VERSION"			checkbutton	"VERSION" OFF \
			"Toggle Trace Mask Entry for VERSION Event" ] \
		[ list "HALT"				checkbutton	"HALT" OFF \
			"Toggle Trace Mask Entry for HALT Event" ] \
	]

define_trace_mask 3.4 ALL_MISC_EVENTS \
	[ list START_PVMD REG_HOSTER REG_RM REG_TASKER \
		CATCHOUT NOTIFY GETTMASK SETTMASK PERROR TICKLE VERSION HALT ]

set close [ list exchangeMenuIndirect .xpvm.misc_ev_menu \
	.xpvm.trace_mask_menu trace_mask_button .xpvm $tm_subs ]

makeMenu .xpvm.misc_ev_menu trace_mask $misc_ev_info_list \
	$close {} trace_mask


#
# Set Default PVM 3.4 Trace Mask
#

define_trace_mask 3.4 DEFAULT \
	[ list BARRIER BCAST DELETE GETINST GETTID \
			GSIZE JOINGROUP LVGROUP \
		MYTID PARENT SIBLINGS SPAWN SENDSIG KILL EXIT \
		MCAST SEND RECV RECVF NRECV \
			PSEND PRECV TRECV PROBE ADDMHF DELMHF MHF_INVOKE \
		USER_DEFINED ]


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create PVM 3.3 Trace Mask Menus
#

set trace33_mask_info_list \
	[ list \
		[ list "ALL 3.3 EVENTS"	checkbutton	"ALL_3_3_EVENTS" OFF \
			"Set Trace Mask Entries for Every 3.3 Event On / Off" ] \
		[ list "Host Events"	exchangeIndirect \
			.xpvm.host_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Host Events" ] \
		[ list "Group Events"	exchangeIndirect \
			.xpvm.group_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Group Events" ] \
		[ list "Task Events"	exchangeIndirect \
			.xpvm.task_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Task Events" ] \
		[ list "Comm Events"	exchangeIndirect \
			.xpvm.comm_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Communication Events" ] \
		[ list "Collect Events"	exchangeIndirect \
			.xpvm.coll_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Collective Comm Events" ] \
		[ list "Buffer Events"	exchangeIndirect \
			.xpvm.buff_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Message Buffer Events" ] \
		[ list "Pack Events"	exchangeIndirect \
			.xpvm.pack_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Message Packing Events" ] \
		[ list "Unpack Events"	exchangeIndirect \
			.xpvm.unpack_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Message Unpacking Events" ] \
		[ list "Info Events"	exchangeIndirect \
			.xpvm.info_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Informational Events" ] \
		[ list "Misc Events"	exchangeIndirect \
			.xpvm.misc_ev33_menu	trace_mask_button	.xpvm	{} \
			"Raise Trace Mask Menu for 3.3 Miscellaneous Events" ] \
	]

makeMenu .xpvm.trace33_mask_menu trace_mask $trace33_mask_info_list \
	lower {} trace33_mask

### Note, no define_trace_mask for ALL_EVENTS, hard-wired, obvious


#
# Tick
#

puts -nonewline "."
flush stdout


set host_ev_info_list \
	[ list \
		[ list "3.3 Host Events:"		label	 	none ] \
		[ list "ALL HOST EVENTS"	checkbutton	"ALL_HOST_EVENTS" OFF \
			"Set Trace Mask Entries for Every Host Event On / Off" ] \
		[ list "ADDHOSTS"			checkbutton	"ADDHOSTS" OFF \
			"Toggle Trace Mask Entry for ADDHOSTS Event" ] \
		[ list "DELHOSTS"			checkbutton	"DELHOSTS" OFF \
			"Toggle Trace Mask Entry for DELHOSTS Event" ] \
		[ list "CONFIG"				checkbutton	"CONFIG" OFF \
			"Toggle Trace Mask Entry for CONFIG Event" ] \
		[ list "MSTAT"				checkbutton	"MSTAT" OFF \
			"Toggle Trace Mask Entry for MSTAT Event" ] \
		[ list "ARCHCODE"			checkbutton "ARCHCODE" OFF \
			"Toggle Trace Mask Entry for ARCHCODE Event" ] \
		[ list "HOSTSYNC"			checkbutton "HOSTSYNC" OFF \
			"Toggle Trace Mask Entry for HOSTSYNC Event" ] \
	]

define_trace_mask 3.3 ALL_HOST_EVENTS \
	[ list ADDHOSTS DELHOSTS CONFIG MSTAT ARCHCODE HOSTSYNC ]

set close [ list exchangeMenuIndirect .xpvm.host_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.host_ev33_menu trace_mask $host_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set group_ev_info_list \
	[ list \
		[ list "3.3 Group Events:"		label	 	none ] \
		[ list "ALL GROUP EVENTS"	checkbutton	"ALL_GROUP_EVENTS" OFF \
			"Set Trace Mask Entries for Every Group Event On / Off" ] \
		[ list "BARRIER"			checkbutton	"BARRIER" OFF \
			"Toggle Trace Mask Entry for BARRIER Event" ] \
		[ list "BCAST"				checkbutton	"BCAST" OFF \
			"Toggle Trace Mask Entry for BCAST Event" ] \
		[ list "DELETE"				checkbutton	"DELETE" OFF \
			"Toggle Trace Mask Entry for DELETE Event" ] \
		[ list "GETINST"			checkbutton	"GETINST" OFF \
			"Toggle Trace Mask Entry for GETINST Event" ] \
		[ list "GETTID"				checkbutton	"GETTID" OFF \
			"Toggle Trace Mask Entry for GETTID Event" ] \
		[ list "GSIZE"				checkbutton	"GSIZE" OFF \
			"Toggle Trace Mask Entry for GSIZE Event" ] \
		[ list "INSERT"				checkbutton	"INSERT" OFF \
			"Toggle Trace Mask Entry for INSERT Event" ] \
		[ list "JOINGROUP"			checkbutton	"JOINGROUP" OFF \
			"Toggle Trace Mask Entry for JOINGROUP Event" ] \
		[ list "LOOKUP"				checkbutton	"LOOKUP" OFF \
			"Toggle Trace Mask Entry for LOOKUP Event" ] \
		[ list "LVGROUP"			checkbutton	"LVGROUP" OFF \
			"Toggle Trace Mask Entry for LVGROUP Event" ] \
	]

define_trace_mask 3.3 ALL_GROUP_EVENTS \
	[ list BARRIER BCAST DELETE GETINST GETTID \
		GSIZE INSERT JOINGROUP LOOKUP LVGROUP ]

set close [ list exchangeMenuIndirect .xpvm.group_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.group_ev33_menu trace_mask $group_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set task_ev_info_list \
	[ list \
		[ list "3.3 Task Events:"		label	 	none ] \
		[ list "ALL TASK EVENTS"	checkbutton	"ALL_TASK_EVENTS" OFF \
			"Set Trace Mask Entries for Every Task Event On / Off" ] \
		[ list "MYTID"				checkbutton	"MYTID" OFF \
			"Toggle Trace Mask Entry for MYTID Event" ] \
		[ list "PARENT"				checkbutton	"PARENT" OFF \
			"Toggle Trace Mask Entry for PARENT Event" ] \
		[ list "SPAWN"				checkbutton	"SPAWN" OFF \
			"Toggle Trace Mask Entry for SPAWN Event" ] \
		[ list "SENDSIG"			checkbutton	"SENDSIG" OFF \
			"Toggle Trace Mask Entry for SENDSIG Event" ] \
		[ list "KILL"				checkbutton	"KILL" OFF \
			"Toggle Trace Mask Entry for KILL Event" ] \
		[ list "EXIT"				checkbutton	"EXIT" OFF \
			"Toggle Trace Mask Entry for EXIT Event" ] \
	]

define_trace_mask 3.3 ALL_TASK_EVENTS \
	[ list MYTID PARENT SPAWN SENDSIG KILL EXIT ]

set close [ list exchangeMenuIndirect .xpvm.task_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.task_ev33_menu trace_mask $task_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set comm_ev_info_list \
	[ list \
		[ list "3.3 Comm Events:"		label	 	none ] \
		[ list "ALL COMM EVENTS"	checkbutton	"ALL_COMM_EVENTS" OFF \
	"Set Trace Mask Entries for Every Communication Event On / Off" ] \
		[ list "MCAST"				checkbutton	"MCAST" OFF \
			"Toggle Trace Mask Entry for MCAST Event" ] \
		[ list "SEND"				checkbutton	"SEND" OFF \
			"Toggle Trace Mask Entry for SEND Event" ] \
		[ list "RECV"				checkbutton	"RECV" OFF \
			"Toggle Trace Mask Entry for RECV Event" ] \
		[ list "NRECV"				checkbutton	"NRECV" OFF \
			"Toggle Trace Mask Entry for NRECV Event" ] \
		[ list "RECVF"				checkbutton	"RECVF" OFF \
			"Toggle Trace Mask Entry for RECVF Event" ] \
		[ list "PSEND"				checkbutton	"PSEND" OFF \
			"Toggle Trace Mask Entry for PSEND Event" ] \
		[ list "PRECV"				checkbutton	"PRECV" OFF \
			"Toggle Trace Mask Entry for PRECV Event" ] \
		[ list "TRECV"				checkbutton	"TRECV" OFF \
			"Toggle Trace Mask Entry for TRECV Event" ] \
		[ list "PROBE"				checkbutton	"PROBE" OFF \
			"Toggle Trace Mask Entry for PROBE Event" ] \
	]

define_trace_mask 3.3 ALL_COMM_EVENTS \
	[ list MCAST SEND RECV NRECV RECVF \
		PSEND PRECV TRECV PROBE ]

set close [ list exchangeMenuIndirect .xpvm.comm_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.comm_ev33_menu trace_mask $comm_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set coll_ev_info_list \
	[ list \
		[ list "3.3 Collective Events:"		label	 	none ] \
		[ list "ALL COLL EVENTS"	checkbutton	"ALL_COLL_EVENTS" OFF \
"Set Trace Mask Entries for Every Coll Communication Event On / Off" ] \
		[ list "GATHER"				checkbutton	"GATHER" OFF \
			"Toggle Trace Mask Entry for GATHER Event" ] \
		[ list "SCATTER"			checkbutton	"SCATTER" OFF \
			"Toggle Trace Mask Entry for SCATTER Event" ] \
		[ list "REDUCE"				checkbutton	"REDUCE" OFF \
			"Toggle Trace Mask Entry for REDUCE Event" ] \
	]

define_trace_mask 3.3 ALL_COLL_EVENTS \
	[ list GATHER SCATTER REDUCE ]

set close [ list exchangeMenuIndirect .xpvm.coll_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.coll_ev33_menu trace_mask $coll_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set buff_ev_info_list \
	[ list \
		[ list "3.3 Buffer Events:"		label	 	none ] \
		[ list "ALL BUF EVENTS"		checkbutton	"ALL_BUF_EVENTS" OFF \
	"Set Trace Mask Entries for Every Message Buffer Event On / Off" ] \
		[ list "INITSEND"			checkbutton	"INITSEND" OFF \
			"Toggle Trace Mask Entry for INITSEND Event" ] \
		[ list "MKBUF"				checkbutton	"MKBUF" OFF \
			"Toggle Trace Mask Entry for MKBUF Event" ] \
		[ list "BUFINFO"			checkbutton	"BUFINFO" OFF \
			"Toggle Trace Mask Entry for BUFINFO Event" ] \
		[ list "FREEBUF"			checkbutton	"FREEBUF" OFF \
			"Toggle Trace Mask Entry for FREEBUF Event" ] \
		[ list "GETRBUF"			checkbutton	"GETRBUF" OFF \
			"Toggle Trace Mask Entry for GETRBUF Event" ] \
		[ list "SETRBUF"			checkbutton	"SETRBUF" OFF \
			"Toggle Trace Mask Entry for SETRBUF Event" ] \
		[ list "GETSBUF"			checkbutton	"GETSBUF" OFF \
			"Toggle Trace Mask Entry for GETSBUF Event" ] \
		[ list "SETSBUF"			checkbutton	"SETSBUF" OFF \
			"Toggle Trace Mask Entry for SETSBUF Event" ] \
		[ list "GETMWID"			checkbutton	"GETMWID" OFF \
			"Toggle Trace Mask Entry for GETMWID Event" ] \
		[ list "SETMWID"			checkbutton	"SETMWID" OFF \
			"Toggle Trace Mask Entry for SETMWID Event" ] \
	]

define_trace_mask 3.3 ALL_BUF_EVENTS \
	[ list INITSEND MKBUF BUFINFO FREEBUF \
		GETRBUF SETRBUF GETSBUF SETSBUF GETMWID SETMWID ]

set close [ list exchangeMenuIndirect .xpvm.buff_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.buff_ev33_menu trace_mask $buff_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set pack_ev_info_list \
	[ list \
		[ list "3.3 Packing Events:"	label	 	none ] \
		[ list "ALL PACK EVENTS"	checkbutton	"ALL_PACK_EVENTS" OFF \
"Set Trace Mask Entries for Every Message Packing Event On / Off" ] \
		[ list "PKBYTE"				checkbutton	"PKBYTE" OFF \
			"Toggle Trace Mask Entry for PKBYTE Event" ] \
		[ list "PKCPLX"				checkbutton	"PKCPLX" OFF \
			"Toggle Trace Mask Entry for PKCPLX Event" ] \
		[ list "PKDCPLX"			checkbutton	"PKDCPLX" OFF \
			"Toggle Trace Mask Entry for PKDCPLX Event" ] \
		[ list "PKDOUBLE"			checkbutton	"PKDOUBLE" OFF \
			"Toggle Trace Mask Entry for PKDOUBLE Event" ] \
		[ list "PKFLOAT"			checkbutton	"PKFLOAT" OFF \
			"Toggle Trace Mask Entry for PKFLOAT Event" ] \
		[ list "PKINT"				checkbutton	"PKINT" OFF \
			"Toggle Trace Mask Entry for PKINT Event" ] \
		[ list "PKUINT"				checkbutton	"PKUINT" OFF \
			"Toggle Trace Mask Entry for PKUINT Event" ] \
		[ list "PKLONG"				checkbutton	"PKLONG" OFF \
			"Toggle Trace Mask Entry for PKLONG Event" ] \
		[ list "PKULONG"			checkbutton	"PKULONG" OFF \
			"Toggle Trace Mask Entry for PKULONG Event" ] \
		[ list "PKSHORT"			checkbutton	"PKSHORT" OFF \
			"Toggle Trace Mask Entry for PKSHORT Event" ] \
		[ list "PKUSHORT"			checkbutton	"PKUSHORT" OFF \
			"Toggle Trace Mask Entry for PKUSHORT Event" ] \
		[ list "PKSTR"				checkbutton	"PKSTR" OFF \
			"Toggle Trace Mask Entry for PKSTR Event" ] \
		[ list "PACKF"				checkbutton	"PACKF" OFF \
			"Toggle Trace Mask Entry for PACKF Event" ] \
	]

define_trace_mask 3.3 ALL_PACK_EVENTS \
	[ list PKBYTE PKCPLX PKDCPLX PKDOUBLE PKFLOAT \
		PKINT PKUINT PKLONG PKULONG PKSHORT PKUSHORT PKSTR PACKF ]

set close [ list exchangeMenuIndirect .xpvm.pack_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.pack_ev33_menu trace_mask $pack_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set unpack_ev_info_list \
	[ list \
		[ list "3.3 Unpacking Events:"	label	 	none ] \
		[ list "ALL UNPACK EVENTS"	checkbutton	"ALL_UNPACK_EVENTS" \
			OFF \
"Set Trace Mask Entries for Every Message Unpacking Event On / Off" ] \
		[ list "UPKBYTE"			checkbutton	"UPKBYTE" OFF \
			"Toggle Trace Mask Entry for UPKBYTE Event" ] \
		[ list "UPKCPLX"			checkbutton	"UPKCPLX" OFF \
			"Toggle Trace Mask Entry for UPKCPLX Event" ] \
		[ list "UPKDCPLX"			checkbutton	"UPKDCPLX" OFF \
			"Toggle Trace Mask Entry for UPKDCPLX Event" ] \
		[ list "UPKDOUBLE"			checkbutton	"UPKDOUBLE" OFF \
			"Toggle Trace Mask Entry for UPKDOUBLE Event" ] \
		[ list "UPKFLOAT"			checkbutton	"UPKFLOAT" OFF \
			"Toggle Trace Mask Entry for UPKFLOAT Event" ] \
		[ list "UPKINT"				checkbutton	"UPKINT" OFF \
			"Toggle Trace Mask Entry for UPKINT Event" ] \
		[ list "UPKUINT"			checkbutton	"UPKUINT" OFF \
			"Toggle Trace Mask Entry for UPKUINT Event" ] \
		[ list "UPKLONG"			checkbutton	"UPKLONG" OFF \
			"Toggle Trace Mask Entry for UPKLONG Event" ] \
		[ list "UPKULONG"			checkbutton	"UPKULONG" OFF \
			"Toggle Trace Mask Entry for UPKULONG Event" ] \
		[ list "UPKSHORT"			checkbutton	"UPKSHORT" OFF \
			"Toggle Trace Mask Entry for UPKSHORT Event" ] \
		[ list "UPKUSHORT"			checkbutton	"UPKUSHORT" OFF \
			"Toggle Trace Mask Entry for UPKUSHORT Event" ] \
		[ list "UPKSTR"				checkbutton	"UPKSTR" OFF \
			"Toggle Trace Mask Entry for UPKSTR Event" ] \
		[ list "UNPACKF"			checkbutton	"UNPACKF" OFF \
			"Toggle Trace Mask Entry for UNPACKF Event" ] \
	]

define_trace_mask 3.3 ALL_UNPACK_EVENTS \
	[ list UPKBYTE UPKCPLX UPKDCPLX UPKDOUBLE UPKFLOAT \
		UPKINT UPKUINT UPKLONG UPKULONG UPKSHORT UPKUSHORT UPKSTR \
		UNPACKF ]

set close [ list exchangeMenuIndirect .xpvm.unpack_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.unpack_ev33_menu trace_mask $unpack_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set info_ev_info_list \
	[ list \
		[ list "3.3 Info Events:"		label	 	none ] \
		[ list "ALL INFO EVENTS"	checkbutton	"ALL_INFO_EVENTS" OFF \
	"Set Trace Mask Entries for Every Informational Event On / Off" ] \
		[ list "PSTAT"				checkbutton	"PSTAT" OFF \
			"Toggle Trace Mask Entry for PSTAT Event" ] \
		[ list "TASKS"				checkbutton	"TASKS" OFF \
			"Toggle Trace Mask Entry for TASKS Event" ] \
		[ list "GETOPT"				checkbutton	"GETOPT" OFF \
			"Toggle Trace Mask Entry for GETOPT Event" ] \
		[ list "SETOPT"				checkbutton	"SETOPT" OFF \
			"Toggle Trace Mask Entry for SETOPT Event" ] \
		[ list "TIDTOHOST"			checkbutton	"TIDTOHOST" OFF \
			"Toggle Trace Mask Entry for TIDTOHOST Event" ] \
		[ list "GETFDS"				checkbutton	"GETFDS" OFF \
			"Toggle Trace Mask Entry for GETFDS Event" ] \
	]

define_trace_mask 3.3 ALL_INFO_EVENTS \
	[ list PSTAT TASKS GETOPT SETOPT TIDTOHOST GETFDS ]

set close [ list exchangeMenuIndirect .xpvm.info_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.info_ev33_menu trace_mask $info_ev_info_list \
	$close {} trace33_mask


#
# Tick
#

puts -nonewline "."
flush stdout


set misc_ev_info_list \
	[ list \
		[ list "3.3 Misc Events:"		label	 	none ] \
		[ list "ALL MISC EVENTS"	checkbutton	"ALL_MISC_EVENTS" OFF \
	"Set Trace Mask Entries for Every Miscellaneous Event On / Off" ] \
		[ list "START PVMD"			checkbutton	"START_PVMD" OFF \
			"Toggle Trace Mask Entry for START PVMD Event" ] \
		[ list "REG HOSTER"			checkbutton	"REG_HOSTER" OFF \
			"Toggle Trace Mask Entry for REG HOSTER Event" ] \
		[ list "REG RM"				checkbutton	"REG_RM" OFF \
			"Toggle Trace Mask Entry for REG RM Event" ] \
		[ list "REG TASKER"			checkbutton	"REG_TASKER" OFF \
			"Toggle Trace Mask Entry for REG TASKER Event" ] \
		[ list "CATCHOUT"			checkbutton	"CATCHOUT" OFF \
			"Toggle Trace Mask Entry for CATCHOUT Event" ] \
		[ list "NOTIFY"				checkbutton	"NOTIFY" OFF \
			"Toggle Trace Mask Entry for NOTIFY Event" ] \
		[ list "GETTMASK"			checkbutton	"GETTMASK" OFF \
			"Toggle Trace Mask Entry for GETTMASK Event" ] \
		[ list "SETTMASK"			checkbutton	"SETTMASK" OFF \
			"Toggle Trace Mask Entry for SETTMASK Event" ] \
		[ list "PERROR"				checkbutton	"PERROR" OFF \
			"Toggle Trace Mask Entry for PERROR Event" ] \
		[ list "TICKLE"				checkbutton	"TICKLE" OFF \
			"Toggle Trace Mask Entry for TICKLE Event" ] \
		[ list "VERSION"			checkbutton	"VERSION" OFF \
			"Toggle Trace Mask Entry for VERSION Event" ] \
		[ list "HALT"				checkbutton	"HALT" OFF \
			"Toggle Trace Mask Entry for HALT Event" ] \
	]

define_trace_mask 3.3 ALL_MISC_EVENTS \
	[ list START_PVMD REG_HOSTER REG_RM REG_TASKER \
		CATCHOUT NOTIFY GETTMASK SETTMASK PERROR TICKLE VERSION HALT ]

set close [ list exchangeMenuIndirect .xpvm.misc_ev33_menu \
	.xpvm.trace33_mask_menu trace_mask_button .xpvm $tm33_subs ]

makeMenu .xpvm.misc_ev33_menu trace_mask $misc_ev_info_list \
	$close {} trace33_mask


#
# Set Default PVM 3.3 Trace Mask
#

define_trace_mask 3.3 DEFAULT \
	[ list BARRIER BCAST DELETE GETINST GETTID \
			GSIZE INSERT JOINGROUP LOOKUP LVGROUP \
		MYTID PARENT SPAWN SENDSIG KILL EXIT \
		MCAST SEND RECV RECVF NRECV \
			PSEND PRECV TRECV PROBE ]


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Set Up Trace Mask Menus via Trace Format
#

set_trace_format $trace_format


#
# Create System Tasks Menu
#

set systasks_info_list \
	[ list \
		[ list "Group Servers"	checkbutton	 group OFF \
			"Toggle Whether Group Server Tasks are Shown in Views" ] \
	]

makeMenu .xpvm.systasks_menu systasks $systasks_info_list \
	lower {} none


#
# Create View Selection Menu
#

set view_info_list \
	[ list \
		[ list "Network"		checkbutton	 network		ON \
			"Turn Network View On / Off" ] \
		[ list "Space Time"		checkbutton	 space_time		ON \
			"Turn Space-Time View On / Off" ] \
		[ list "Utilization"	checkbutton	 utilization	OFF \
			"Turn Utilization View On / Off" ] \
		[ list "Message Queue"	checkbutton	 msgq			OFF \
			"Turn Message Queue View On / Off" ] \
		[ list "Call Trace"		checkbutton	 call_trace		OFF \
			"Turn Call Trace View On / Off" ] \
		[ list "Task Output"	checkbutton	 task_output	OFF \
			"Turn Task Output View On / Off" ] \
		[ list "Event History"	checkbutton	 task_tevhist	OFF \
			"Turn Trace Event History View On / Off" ] \
	]

makeMenu .xpvm.views_menu viewSelect $view_info_list lower {} none


#
# Create Options Menu
#

set options_info_list \
	[ list \
		[ list "Modes:          "	label			modes ] \
		[ list "Query       "		radiobutton 	query \
			qc_info_mode "Set Interaction Mode To Query" ] \
		[ list "Correlate  "		radiobutton 	correlate \
			qc_info_mode "Set Interaction Mode to Correlate" ] \
		[ list "Task Sorting: "		label			task_sort ] \
		[ list "Alpha       "		radiobutton 	alpha \
			task_sort_mode \
			"Set Task Sorting To Alphabetical by Host / Task Name" ] \
		[ list "Task ID     "		radiobutton 	tid \
			task_sort_mode \
			"Set Task Sorting To Numerically by Task ID" ] \
		[ list "Custom    "			radiobutton 	custom \
			task_sort_mode \
			"Set Task Sorting To Manual, Custom Arrangement by User" ] \
	]

set qc_info_mode "query"

set correlate_keep FALSE
set correlate_lock FALSE

set task_sort_mode "alpha"

makeMenu .xpvm.options_menu optionsHandle $options_info_list \
	lower {} none


#
# Create Reset Menu
#

set reset_info_list \
	[ list \
		[ list "Reset PVM"		command	 pvm \
			"Reset the PVM Virtual Machine, Kill All Tasks" ] \
		[ list "Reset Views"	command	 views \
		"Clear XPVM Views, Leave PVM Tasks and Trace File Intact" ] \
		[ list "Reset Trace"	command	 trace_file \
	"Reset Trace File (Overwrite), Leave PVM Tasks and Views Intact" ] \
		[ list "Reset All"		command	 all \
"Reset PVM Virtual Machine, Clear XPVM Views, Overwrite Trace File" ] \
	]

makeMenu .xpvm.reset_menu reset $reset_info_list lower {} none


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Help Menus
#

set help_info_list \
	[ list \
		[ list "General Help:"	label		none ] \
		[ list "About XPVM"		command		xpvm \
			"Raise / Lower Help Window About XPVM" ] \
		[ list "File..."		exchange	.xpvm.file_help_menu \
			.xpvm.help .xpvm {} \
			"Raise / Lower File Options Help Submenu" ] \
		[ list "Hosts"			command		hosts \
			"Raise / Lower Help Window About Hosts" ] \
		[ list "Tasks..."		exchange	.xpvm.tasks_help_menu \
			.xpvm.help .xpvm {} \
			"Raise / Lower Tasks Help Submenu" ] \
		[ list "Views..."		exchange	.xpvm.views_help_menu \
			.xpvm.help .xpvm {} \
			"Raise / Lower Views Help Submenu" ] \
		[ list "Options..."		exchange	.xpvm.options_help_menu \
			.xpvm.help .xpvm {} \
			"Raise / Lower General Options Help Submenu" ] \
		[ list "Reset"			command		reset \
			"Raise / Lower Help Window About Reset Commands" ] \
		[ list "Traces"			command		traces \
			"Raise / Lower Help Window About XPVM Tracing" ] \
		[ list "Author"			command		author \
"Raise / Lower Help Window About the Author - Contact Information" ] \
	]

makeMenu .xpvm.help_menu helpSelect $help_info_list lower {} none


set file_help_info_list \
	[ list \
		[ list "File Help:"	label	 none ] \
		[ list "Quit XPVM"		command		file_quit \
			"Raise / Lower Help Window About Quitting XPVM" ] \
		[ list "Halt PVM"		command		file_halt \
			"Raise / Lower Help Window About Halting XPVM" ] \
	]

set close [ list exchangeMenu .xpvm.file_help_menu .xpvm.help_menu \
	.xpvm.help .xpvm {} ]

makeMenu .xpvm.file_help_menu helpSelect $file_help_info_list \
	$close {} none


set tasks_help_info_list \
	[ list \
		[ list "Task Help:"	label	 none ] \
		[ list "Spawn"		command	 tasks_spawn \
			"Raise / Lower Help Window About Spawning Tasks in XPVM" ] \
		[ list "On-The-Fly"	command	 tasks_otf \
		"Raise / Lower Help Window About On-The-Fly Task Tracing" ] \
		[ list "Kill"		command	 tasks_kill \
			"Raise / Lower Help Window About Killing PVM Tasks" ] \
		[ list "Signal"		command	 tasks_signal \
			"Raise / Lower Help Window About Signaling PVM Tasks" ] \
		[ list "Sys Tasks"	command	 tasks_systasks \
	"Raise / Lower Help Window About Viewing System Tasks in XPVM" ] \
	]

set close [ list exchangeMenu .xpvm.tasks_help_menu .xpvm.help_menu \
	.xpvm.help .xpvm {} ]

makeMenu .xpvm.tasks_help_menu helpSelect $tasks_help_info_list \
	$close {} none


set views_help_info_list \
	[ list \
		[ list "Views Help:"	label	 none ] \
		[ list "Network"		command	 views_network \
			"Raise / Lower Help Window About the Network View" ] \
		[ list "Space Time"		command	 views_space_time \
			"Raise / Lower Help Window About the Space-Time View" ] \
		[ list "Utilization"	command	 views_utilization \
			"Raise / Lower Help Window About the Utilization View" ] \
		[ list "Message Queue"	command	 views_msgq \
			"Raise / Lower Help Window About the Message Queue View" ] \
		[ list "Call Trace"		command	 views_call_trace \
			"Raise / Lower Help Window About the Call Trace View" ] \
		[ list "Task Output"	command	 views_task_output \
			"Raise / Lower Help Window About the Task Output View" ] \
		[ list "Event History"	command	 views_tevhist \
			"Raise / Lower Help Window About the Event History View" ] \
	]

set close [ list exchangeMenu .xpvm.views_help_menu .xpvm.help_menu \
	.xpvm.help .xpvm {} ]

makeMenu .xpvm.views_help_menu helpSelect $views_help_info_list \
	$close {} none


set options_help_info_list \
	[ list \
		[ list "Options Help:"	label	 none ] \
		[ list "Action Modes"	command		options_mode \
			"Raise / Lower Help Window About XPVM Action Modes" ] \
		[ list "Task Sorting"	command		options_tasksort \
			"Raise / Lower Help Window About Task Sorting" ] \
	]

set close [ list exchangeMenu .xpvm.options_help_menu .xpvm.help_menu \
	.xpvm.help .xpvm {} ]

makeMenu .xpvm.options_help_menu helpSelect $options_help_info_list \
	$close {} none


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Other Host Add Dialog Box
#

set OH .xpvm.otherhost

frame $OH -bd 4 -relief raised

restrict_bindings $OH "Frame"

label $OH.label -text {Enter New Host Name (with options):} \
	-foreground $fg_color

restrict_bindings $OH.label "Label"

entry $OH.host_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $OH.host_entry "do_pvm_other_host"

restrict_bindings $OH.host_entry "Entry"

button $OH.cancel -text {Cancel} -command [ list place forget $OH ] \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OH.cancel ""

button $OH.accept -text {Accept} -command "do_pvm_other_host" \
	-padx 1 -pady 1 \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OH.accept ""

set y $border_space

place $OH.label -relx 0.50 -y $y -anchor n

update

set y [ below $OH.label ]

place $OH.host_entry -relx 0.50 -y $y -anchor n \
	-width [ winfo width $OH.label ]

update

set y [ below $OH.host_entry ]

place $OH.cancel -relx 0.28 -y $y -anchor n

place $OH.accept -relx 0.71 -y $y -anchor n

place forget $OH


#
# Create Trace Overwrite Check Dialog Box
#

set OV .xpvm.overwrite

frame $OV -bd 4 -relief raised

restrict_bindings $OV "Frame"

label $OV.label -text {Overwrite Trace File:} -foreground $fg_color

restrict_bindings $OV.label "Label"

label $OV.file -text {Dummy File} -foreground $fg_color

restrict_bindings $OV.file "Label"

button $OV.yes -text {Yes} \
	-padx 1 -pady 1 \
	-command "place forget $OV ; trace_overwrite_result TRUE" \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OV.yes ""

button $OV.no -text {No} \
	-padx 1 -pady 1 \
	-command "place forget $OV ; trace_overwrite_result FALSE" \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OV.no ""

set y $border_space

place $OV.label -relx 0.50 -y $y -anchor n

set y [ expr $y + $row_height + $border_space ]

place $OV.file -relx 0.50 -y $y -anchor n

set y [ expr $y + $row_height + $border_space ]

place $OV.yes -relx 0.35 -y $y -anchor n

place $OV.no -relx 0.65 -y $y -anchor n

place forget $OV


#
# Flush Main Menus / Dialogs Creation
#

update


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Toplevel for Utilization
#

set UT .utilization

set ut_geom "[ expr $ut_width ]x[ expr $ut_height ]"

toplevel $UT

wm geometry $UT $ut_geom

set min_ut_height [ expr (4 * $row_height) + $scroll_width \
	+ (5 * $border_space / 2) + 20 ]

set min_ut_width 350

wm minsize $UT $min_ut_width $min_ut_height


#
# Withdraw Window - Not Raised Initially
#

wm withdraw $UT


#
# Create Utilization View
#

set UT_F $UT.frame

set UT_C $UT_F.canvas

set UT_SBH $UT_F.horiz_sb

set ut_cheight [ expr $ut_height - $scroll_width \
	- $row_height - (5 * $border_space) ]

set ut_cwidth [ expr $ut_width \
	- (3 * $col_width) - (2 * $border_space) ]

frame $UT_F

restrict_bindings $UT_F "Frame"

label $UT_F.title -text {Utilization vs. Time} -foreground $fg_color

restrict_bindings $UT_F.title "Label"

label $UT_F.xN -text {N} -foreground $fg_color
restrict_bindings $UT_F.xN "Label"

label $UT_F.xT -text {T} -foreground $fg_color
restrict_bindings $UT_F.xT "Label"

label $UT_F.xA -text {A} -foreground $fg_color
restrict_bindings $UT_F.xA "Label"

label $UT_F.xS -text {S} -foreground $fg_color
restrict_bindings $UT_F.xS "Label"

label $UT_F.xK -text {K} -foreground $fg_color
restrict_bindings $UT_F.xK "Label"

label $UT_F.xX -text {S} -foreground $fg_color
restrict_bindings $UT_F.xX "Label"

label $UT_F.ntasks -text {1} -foreground $fg_color

restrict_bindings $UT_F.ntasks "Label"

label $UT_F.tick -text {    } -background $fg_color

restrict_bindings $UT_F.tick "Label"

label $UT_F.zero -text {0} -foreground $fg_color

restrict_bindings $UT_F.zero "Label"

canvas $UT_C -bd $frame_border -relief sunken \
	$XSCROLLINCREMENT 1 -confine 0 \
	-scrollregion "0 0 $scroll_canvas_width $ut_canvas_height"

restrict_bindings $UT_C "Canvas"

set cmd [ list scrollViews UT 1 ]

scrollbar $UT_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $UT_SBH "Scrollbar"

scrollInit $UT_C $UT_SBH -1 $ut_cwidth $ut_cwidth ut_xview HORIZ low 1

set cmd [ list viewSelect utilization Utilization \
	views_menu_state_Utilization TRUE ]

button $UT_F.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $UT_F.close ""


#
# Create Utilization Key
#

label $UT_F.computing_label -text {Computing} -foreground $fg_color

restrict_bindings $UT_F.computing_label "Label"

label $UT_F.computing_box -text {    } \
	-bd 1 -relief sunken -background $ut_running_color

restrict_bindings $UT_F.computing_box "Label"

label $UT_F.overhead_label -text {Overhead} -foreground $fg_color

restrict_bindings $UT_F.overhead_label "Label"

label $UT_F.overhead_box -text {    } \
	-bd 1 -relief sunken -background $ut_system_color

restrict_bindings $UT_F.overhead_box "Label"

label $UT_F.waiting_label -text {Waiting} -foreground $fg_color

restrict_bindings $UT_F.waiting_label "Label"

label $UT_F.waiting_box -text {    } \
	-bd 1 -relief sunken -background $ut_idle_color

restrict_bindings $UT_F.waiting_box "Label"

layout_ut_panel


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Message Queue Toplevel
#

set MQ .msgq

set mq_geom "[ expr $mq_width ]x[ expr $mq_height ]"

toplevel $MQ

wm geometry $MQ $mq_geom

set min_mq_height [ expr (2 * $row_height) + $scroll_width \
	+ (3 * $border_space) ]

set min_mq_width [ expr (4 * $col_width) + (2 * $border_space) ]

wm minsize $MQ $min_mq_width $min_mq_height


#
# Withdraw Window - Not Raised Initially
#

wm withdraw $MQ


#
# Create Message Queue View
#

set MQ_F $MQ.frame

set MQ_C $MQ_F.canvas

set MQ_SBH $MQ_F.horiz_sb

set mq_cheight [ expr $mq_height - (2 * $row_height) - $scroll_width \
	- (3 * $border_space) ]

set mq_cwidth [ expr $mq_width - (4 * $col_width) \
	- (2 * $border_space) ]

frame $MQ_F

restrict_bindings $MQ_F "Frame"

label $MQ_F.title -text {Message Queue Per Task:} -foreground $fg_color

restrict_bindings $MQ_F.title "Label"

label $MQ_F.xN -text {N} -foreground $fg_color
restrict_bindings $MQ_F.xN "Label"

label $MQ_F.xB -text {B} -foreground $fg_color
restrict_bindings $MQ_F.xB "Label"

label $MQ_F.xY -text {Y} -foreground $fg_color
restrict_bindings $MQ_F.xY "Label"

label $MQ_F.xT -text {T} -foreground $fg_color
restrict_bindings $MQ_F.xT "Label"

label $MQ_F.xE -text {E} -foreground $fg_color
restrict_bindings $MQ_F.xE "Label"

label $MQ_F.xS -text {S} -foreground $fg_color
restrict_bindings $MQ_F.xS "Label"

label $MQ_F.nbytes -text {1} -foreground $fg_color

restrict_bindings $MQ_F.nbytes "Label"

label $MQ_F.tick -text {    } -background $fg_color

restrict_bindings $MQ_F.tick "Label"

label $MQ_F.zero -text {0} -foreground $fg_color

restrict_bindings $MQ_F.zero "Label"

canvas $MQ_C -bd $frame_border -relief sunken \
	$XSCROLLINCREMENT $mq_task_wt -confine 0 \
	-scrollregion "0 0 $mq_canvas_width $mq_canvas_height"

restrict_bindings $MQ_C "Canvas"

set mq_tasks_width 0

set cmd [ list scrollCanvas $MQ_C $MQ_SBH \
	-1 mq_tasks_width mq_cwidth mq_xview HORIZ low $mq_task_wt ]

scrollbar $MQ_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $MQ_SBH "Scrollbar"

scrollInit $MQ_C $MQ_SBH -1 $mq_tasks_width $mq_cwidth mq_xview \
	HORIZ low $mq_task_wt

set cmd [ list viewSelect msgq {Message Queue} \
	views_menu_state_Message_Queue TRUE ]

button $MQ_F.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $MQ_F.close ""

label $MQ_F.query_frame -bd $frame_border -relief sunken

restrict_bindings $MQ_F.query_frame "Label"

label $MQ_F.query_frame.query -text {View Info:} \
	-foreground $fg_color -anchor nw

restrict_bindings $MQ_F.query_frame.query "Label"

set mq_query_text_scroll 0

set mq_query_text ""

layout_mq_panel


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Call Trace Toplevel
#

set CT .call_trace

set ct_geom "[ expr $ct_width ]x[ expr $ct_height ]"

toplevel $CT

wm geometry $CT $ct_geom

set min_ct_height [ expr (3 * $scroll_width) + $row_height \
	+ (3 * $border_space) ]

set min_ct_width [ expr $ct_labels_width + (3 * $scroll_width) \
	+ (2 * $border_space) ]

wm minsize $CT $min_ct_width $min_ct_height


#
# Withdraw Window - Not Raised Initially
#

wm withdraw $CT


#
# Create Call Trace View
#

set CT_F $CT.frame

set CT_L $CT_F.labels

set CT_C $CT_F.canvas

set CT_SBH $CT_F.horiz_sb
set CT_SBV $CT_F.vert_sb

set ct_lwidth [ expr $ct_labels_width + $scroll_width ]

set ct_cheight [ expr $ct_height - $scroll_width \
	- $row_height - (3 * $border_space) ]

set ct_cwidth [ expr $ct_width - $ct_lwidth - (2 * $border_space) ]

frame $CT_F

restrict_bindings $CT_F "Frame"

label $CT_F.title -text {Last Event Per Task:} \
	-foreground $fg_color

restrict_bindings $CT_F.title "Label"

label $CT_F.query -text "" -foreground $fg_color

restrict_bindings $CT_F.query "Label"

canvas $CT_L -bd $frame_border -relief sunken \
	$YSCROLLINCREMENT $st_label_ht -confine 0 \
	-scrollregion "0 0 $ct_labels_width $ct_canvas_height"

restrict_bindings $CT_L "Canvas"

canvas $CT_C -bd $frame_border -relief sunken \
	$XSCROLLINCREMENT $st_label_ht $YSCROLLINCREMENT $st_label_ht \
	-confine 0 \
	-scrollregion "0 0 $ct_canvas_width $ct_canvas_height"

restrict_bindings $CT_C "Canvas"

set cmd [ list scrollCanvas $CT_C $CT_SBH \
	-1 ct_canvas_width ct_cwidth ct_xview HORIZ low $st_label_ht ]

scrollbar $CT_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $CT_SBH "Scrollbar"

scrollInit $CT_C $CT_SBH -1 $ct_canvas_width $ct_cwidth ct_xview \
	HORIZ low $st_label_ht

set cmd [ list scrollTwoCanvases $CT_L $CT_C $CT_SBV \
	-1 tasks_height ct_cheight ct_yview ct_yview VERT low \
	$st_label_ht $st_label_ht ]

scrollbar $CT_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $CT_SBV "Scrollbar"

scrollInit $CT_L $CT_SBV -1 $tasks_height $ct_cheight ct_yview \
	VERT low $st_label_ht

scrollInit $CT_C $CT_SBV -1 $tasks_height $ct_cheight ct_yview \
	VERT low $st_label_ht

set cmd [ list viewSelect call_trace {Call Trace} \
	views_menu_state_Call_Trace TRUE ]

button $CT_F.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $CT_F.close ""

layout_ct_panel


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Task Output Toplevel
#

set TO .task_output

set to_geom "[ expr $to_width ]x[ expr $to_height ]"

toplevel $TO

wm geometry $TO $to_geom

set min_to_height [ expr (3 * $scroll_width) + $row_height \
	+ (3 * $border_space) ]

set min_to_width [ expr (3 * $scroll_width) + (2 * $border_space) ]

wm minsize $TO $min_to_width $min_to_height


#
# Withdraw Window - Not Raised Initially
#

wm withdraw $TO


#
# Create Task Output View
#

set TO_F $TO.frame

set TO_C $TO_F.canvas

set TO_SBH $TO_F.horiz_sb
set TO_SBV $TO_F.vert_sb

set to_cheight [ expr $to_height - $scroll_width \
	- $row_height - (5 * $border_space) ]

set to_cwidth [ expr $to_width - $scroll_width - (2 * $border_space) ]

set to_lasty $frame_border

frame $TO_F

restrict_bindings $TO_F "Frame"

label $TO_F.title -text {Task Output:} \
	-foreground $fg_color

restrict_bindings $TO_F.title "Label"

entry $TO_F.file_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $TO_F.file_entry "set_output_file"

restrict_bindings $TO_F.file_entry "Entry"

set output_file ""

label $TO_F.filter_label -text {Filter:} \
	-foreground $fg_color

restrict_bindings $TO_F.filter_label "Label"

entry $TO_F.filter_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $TO_F.filter_entry "set_output_filter"

restrict_bindings $TO_F.filter_entry "Entry"

set output_filter ""

button $TO_F.search_button -text {Search:} \
	-command "set_output_search" \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $TO_F.search_button ""

set cmd [ list toggle_search output $TO_F.search_dir layout_to_panel ]

button $TO_F.search_dir -text {(Forward)} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $TO_F.search_dir ""

entry $TO_F.search_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $TO_F.search_entry "set_output_search"

restrict_bindings $TO_F.search_entry "Entry"

canvas $TO_C -bd $frame_border -relief sunken \
	$XSCROLLINCREMENT $fixed_row_height \
	$YSCROLLINCREMENT $fixed_row_height \
	-confine 0 \
	-scrollregion "0 0 $to_canvas_width $to_canvas_height"

restrict_bindings $TO_C "Canvas"

set cmd [ list scrollCanvas $TO_C $TO_SBH \
	-1 to_canvas_width to_cwidth to_xview HORIZ low $fixed_row_height ]

scrollbar $TO_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $TO_SBH "Scrollbar"

scrollInit $TO_C $TO_SBH -1 $to_canvas_width $to_cwidth to_xview \
	HORIZ low $fixed_row_height

set cmd [ list scrollCanvas $TO_C $TO_SBV \
	-1 to_lasty to_cheight to_yview VERT low $fixed_row_height ]

scrollbar $TO_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $TO_SBV "Scrollbar"

scrollInit $TO_C $TO_SBV -1 $to_lasty $to_cheight to_yview \
	VERT low $fixed_row_height

set cmd [ list viewSelect task_output {Task Output} \
	views_menu_state_Task_Output TRUE ]

button $TO_F.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $TO_F.close ""

layout_to_panel


#
# Create Task Output File Overwrite Check Dialog Box
#

set OF $TO.overwrite_file

frame $OF -bd 4 -relief raised

restrict_bindings $OF "Frame"

label $OF.label -text {File Exists!!  Overwrite Task Output File:} \
	-foreground $fg_color

restrict_bindings $OF.label "Label"

label $OF.file -text {Dummy File} -foreground $fg_color

restrict_bindings $OF.file "Label"

button $OF.yes -text {Yes} \
	-padx 1 -pady 1 \
	-command "place forget $OF ; do_file_overwrite_result TRUE" \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OF.yes ""

button $OF.append -text {Append} \
	-padx 1 -pady 1 \
	-command "place forget $OF ; do_file_overwrite_result APPEND" \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OF.append ""

button $OF.no -text {No} \
	-padx 1 -pady 1 \
	-command "place forget $OF ; do_file_overwrite_result FALSE" \
	-bd $frame_border -relief raised -foreground $fg_color \
	-activeforeground $active_fg_color

restrict_bindings $OF.no ""

set y $border_space

place $OF.label -relx 0.50 -y $y -anchor n

set y [ expr $y + $row_height + $border_space ]

place $OF.file -relx 0.50 -y $y -anchor n

set y [ expr $y + $row_height + $border_space ]

place $OF.yes -relx 0.25 -y $y -anchor n

place $OF.append -relx 0.50 -y $y -anchor n

place $OF.no -relx 0.75 -y $y -anchor n

place forget $OF


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Create Task Trace Event History Toplevel
#

set TV .task_tevhist

set tv_geom "[ expr $tv_width ]x[ expr $tv_height ]"

toplevel $TV

wm geometry $TV $tv_geom

set min_tv_height [ expr (3 * $scroll_width) + $row_height \
	+ (3 * $border_space) ]

set min_tv_width [ expr (3 * $scroll_width) + (2 * $border_space) ]

wm minsize $TV $min_tv_width $min_tv_height


#
# Withdraw Window - Not Raised Initially
#

wm withdraw $TV


#
# Create Task Output View
#

set TV_F $TV.frame

set TV_C $TV_F.canvas

set TV_SBH $TV_F.horiz_sb
set TV_SBV $TV_F.vert_sb

set tv_cheight [ expr $tv_height - $scroll_width \
	- $row_height - (5 * $border_space) ]

set tv_cwidth [ expr $tv_width - $scroll_width - (2 * $border_space) ]

set tv_lasty $frame_border

frame $TV_F

restrict_bindings $TV_F "Frame"

label $TV_F.title -text {Trace Event History:} \
	-foreground $fg_color

restrict_bindings $TV_F.title "Label"

label $TV_F.filter_label -text {Filter:} \
	-foreground $fg_color

restrict_bindings $TV_F.filter_label "Label"

entry $TV_F.filter_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $TV_F.filter_entry "set_tevhist_filter"

restrict_bindings $TV_F.filter_entry "Entry"

set tevhist_filter ""

button $TV_F.search_button -text {Search:} \
	-command "set_tevhist_search" \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $TV_F.search_button ""

set cmd [ list toggle_search tevhist $TV_F.search_dir layout_tv_panel ]

button $TV_F.search_dir -text {(Forward)} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $TV_F.search_dir ""

entry $TV_F.search_entry -font $main_font \
	-bd $frame_border -relief sunken -foreground $fg_color

entry_setup $TV_F.search_entry "set_tevhist_search"

restrict_bindings $TV_F.search_entry "Entry"

canvas $TV_C -bd $frame_border -relief sunken \
	$XSCROLLINCREMENT $fixed_row_height \
	$YSCROLLINCREMENT $fixed_row_height \
	-confine 0 \
	-scrollregion "0 0 $tv_canvas_width $tv_canvas_height"

restrict_bindings $TV_C "Canvas"

set cmd [ list scrollCanvas $TV_C $TV_SBH \
	-1 tv_canvas_width tv_cwidth tv_xview HORIZ low $fixed_row_height ]

scrollbar $TV_SBH -orient horiz -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $TV_SBH "Scrollbar"

scrollInit $TV_C $TV_SBH -1 $tv_canvas_width $tv_cwidth tv_xview \
	HORIZ low $fixed_row_height

set cmd [ list scrollCanvas $TV_C $TV_SBV \
	-1 tv_lasty tv_cheight tv_yview VERT low $fixed_row_height ]

scrollbar $TV_SBV -orient vert -bd $frame_border -relief sunken \
	-width $scroll_width -command $cmd

restrict_bindings $TV_SBV "Scrollbar"

scrollInit $TV_C $TV_SBV -1 $tv_lasty $tv_cheight tv_yview \
	VERT low $fixed_row_height

set cmd [ list viewSelect task_tevhist {Event History} \
	views_menu_state_Event_History TRUE ]

button $TV_F.close -text {Close} -command $cmd \
	-padx 1 -pady 1 \
	-foreground $fg_color -activeforeground $active_fg_color

restrict_bindings $TV_F.close ""

layout_tv_panel


#
# Create Fast Forward Indicators
#

set FF_NET $NET.fastfwd_net
set FF_ST $ST.fastfwd_st
set FF_UT $UT_F.fastfwd_ut
set FF_CT $CT_F.fastfwd_ct
set FF_MQ $MQ_F.fastfwd_mq
set FF_TO $TO_F.fastfwd_to
set FF_TV $TV_F.fastfwd_tv

set FF_LIST \
	[ list \
		"$FF_NET $NET_C" \
		"$FF_ST $ST_C" \
		"$FF_UT $UT_C" \
		"$FF_CT $CT_C" \
		"$FF_MQ $MQ_C" \
		"$FF_TO $TO_C" \
		"$FF_TV $TV_C" \
	]

foreach ff $FF_LIST \
{
	set name [ lindex $ff 0 ]

	frame $name -bd 4 -relief raised

	restrict_bindings $name "Frame"

	label $name.label -text {Fast Forwarding...} -foreground $fg_color

	restrict_bindings $name.label "Label"

	place $name.label -relx 0.50 -y [ expr $border_space / 2 ] -anchor n

	place forget $name
}


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Add Canvas Event Handle Bindings
#

bind $OTF <Map>					"do_otf refresh"
bind $OTF.canvas <1>			"do_otf %y"

bind $KL <Map>					"do_kill refresh"
bind $KL.canvas <1>				"do_kill %y"

bind $SG <Map>					"do_signal refresh"
bind $SG.canvas <1>				"do_signal %y"

bind $NET_C <ButtonPress-1>		"network_handle query_press %x %y"
bind $NET_C <Button1-Motion>	"network_handle query_motion %x %y"
bind $NET_C <ButtonRelease-1>	"network_handle query_release %x %y"

bind $NET_C <Enter>				"network_handle enter %x %y"
bind $NET_C <Leave>				"network_handle leave %x %y"

bind $ST_L <ButtonPress-1>		"canvas_handle ST label_press %x %y"
bind $ST_L <Button1-Motion>		"canvas_handle ST label_motion %x %y"
bind $ST_L <ButtonRelease-1>	"canvas_handle ST label_release %x %y"

bind $ST_C <ButtonPress-1>		"canvas_handle ST query_press %x %y"
bind $ST_C <Button1-Motion>		"canvas_handle ST query_motion %x %y"
bind $ST_C <ButtonRelease-1>	"canvas_handle ST query_release %x %y"

bind $ST_L <Enter>				"canvas_handle ST label_enter %x %y"
bind $ST_L <Leave>				"canvas_handle ST label_leave %x %y"

bind $ST_C <Enter>				"canvas_handle ST enter %x %y"
bind $ST_C <Motion>				"canvas_handle ST time %x %y"
bind $ST_C <Leave>				"canvas_handle ST leave %x %y"

bind $ST_C <ButtonPress-2>		"canvas_handle ST zoom_press %x %y"
bind $ST_C <Button2-Motion>		"canvas_handle ST zoom_motion %x %y"
bind $ST_C <ButtonRelease-2>	"canvas_handle ST zoom_release %x %y"

bind $ST_C <3>					"canvas_handle ST unzoom %x %y"

bind $ST.query_frame <Enter>	"st_info_scroll enter %x"
bind $ST.query_frame <Motion>	"st_info_scroll motion %x"
bind $ST.query_frame <Leave>	"st_info_scroll leave %x"

bind $ST.query_frame.query <Enter>	"st_info_scroll enter %x"
bind $ST.query_frame.query <Motion>	"st_info_scroll motion %x"
bind $ST.query_frame.query <Leave>	"st_info_scroll leave %x"

bind $UT_C <ButtonPress-1>		"canvas_handle UT query_press %x %y"
bind $UT_C <Button1-Motion>		"canvas_handle UT query_motion %x %y"
bind $UT_C <ButtonRelease-1>	"canvas_handle UT query_release %x %y"

bind $UT_C <Enter>				"canvas_handle UT enter %x %y"
bind $UT_C <Motion>				"canvas_handle UT time %x %y"
bind $UT_C <Leave>				"canvas_handle UT leave %x %y"

bind $UT_C <ButtonPress-2>		"canvas_handle UT zoom_press %x %y"
bind $UT_C <Button2-Motion>		"canvas_handle UT zoom_motion %x %y"
bind $UT_C <ButtonRelease-2>	"canvas_handle UT zoom_release %x %y"

bind $UT_C <3>					"canvas_handle UT unzoom %x %y"

bind $MQ_C <ButtonPress-1>		"mq_canvas_handle query_press %x %y"
bind $MQ_C <Button1-Motion>		"mq_canvas_handle query_motion %x %y"
bind $MQ_C <ButtonRelease-1>	"mq_canvas_handle query_release %x %y"

bind $MQ_C <Enter>				"mq_canvas_handle enter %x %y"
bind $MQ_C <Motion>				"mq_canvas_handle move %x %y"
bind $MQ_C <Leave>				"mq_canvas_handle leave %x %y"

bind $MQ_F.query_frame <Enter>	"mq_info_scroll enter %x"
bind $MQ_F.query_frame <Motion>	"mq_info_scroll motion %x"
bind $MQ_F.query_frame <Leave>	"mq_info_scroll leave %x"

bind $MQ_F.query_frame.query <Enter>	"mq_info_scroll enter %x"
bind $MQ_F.query_frame.query <Motion>	"mq_info_scroll motion %x"
bind $MQ_F.query_frame.query <Leave>	"mq_info_scroll leave %x"

bind $CT_L <ButtonPress-1>		"ct_canvas_handle label_press %x %y"
bind $CT_L <Button1-Motion>		"ct_canvas_handle label_motion %x %y"
bind $CT_L <ButtonRelease-1>	"ct_canvas_handle label_release %x %y"

bind $CT_L <Enter>				"ct_canvas_handle label_enter %x %y"
bind $CT_L <Leave>				"ct_canvas_handle label_leave %x %y"

bind $CT_C <ButtonPress-1>		"ct_canvas_handle query_press %x %y"
bind $CT_C <Button1-Motion>		"ct_canvas_handle query_motion %x %y"
bind $CT_C <ButtonRelease-1>	"ct_canvas_handle query_release %x %y"

bind $CT_C <Enter>				"ct_canvas_handle enter %x %y"
bind $CT_C <Leave>				"ct_canvas_handle leave %x %y"

bind $ADJ <ButtonPress-1>		"adjust_main_panel press %y"
bind $ADJ <Button1-Motion>		"adjust_main_panel motion %y"
bind $ADJ <ButtonRelease-1>		"adjust_main_panel release %y"

bind .xpvm <Configure>			"resize_main_panel"

if { $depth != 1 } \
{
	bind $NK <Configure>		"resize_nk_panel"
}

bind $UT <Configure>			"resize_ut_panel"

bind $MQ <Configure>			"resize_mq_panel"

bind $CT <Configure>			"resize_ct_panel"

bind $TO <Configure>			"resize_to_panel"

bind $TV <Configure>			"resize_tv_panel"

set win_map_list ""

bind .xpvm <Unmap>				"unmap_all"

bind .xpvm <Map>				"remap_all"

bind $UT <Map>					"remap_all"
bind $MQ <Map>					"remap_all"
bind $CT <Map>					"remap_all"
bind $TO <Map>					"remap_all"
bind $TV <Map>					"remap_all"

bind .xpvm <Destroy>			"exit"

bind $UT <Destroy>				"exit"
bind $MQ <Destroy>				"exit"
bind $CT <Destroy>				"exit"
bind $TO <Destroy>				"exit"
bind $TV <Destroy>				"exit"


#
# Tick
#

puts -nonewline "."
flush stdout


#
# Button Help Bindings
#


#
# Main Panel
#

butt_help .xpvm.active label \
"Trace Status - a * Indicates that XPVM is Busy Collecting Trace Events"

butt_help .xpvm.file button \
	"Raise / Lower File Menu for Exiting XPVM and Halting PVM"

butt_help .xpvm.hosts button \
	"Raise / Lower Hosts Menu for Adding & Deleting Hosts"

butt_help .xpvm.tasks button \
	"Raise / Lower Tasks Menu for Spawning & Controlling Tasks"

butt_help .xpvm.views button \
	"Raise / Lower Views Menu for Turning Views On & Off"

butt_help .xpvm.options button \
	"Raise / Lower Options Menu for Setting Modes and Task Sorting"

butt_help .xpvm.reset button \
	"Raise / Lower Reset Menu for Resetting PVM, Views, or Trace File"

butt_help .xpvm.help button \
	"Raise / Lower Help Menu for More Detailed Help Information"


#
# Network View
#

butt_help $NET.active_label label \
	"Host is Active - At Least One Computing Task Present"

butt_help $NET.active_box label \
	"Host is Active - At Least One Computing Task Present"

butt_help $NET.system_label label \
	"Host is in System Overhead - Tasks Executing PVM System Calls"

butt_help $NET.system_box label \
	"Host is in System Overhead - Tasks Executing PVM System Calls"

butt_help $NET.empty_label label \
	"No Tasks Executing on Host"

butt_help $NET.empty_box label \
	"No Tasks Executing on Host"

if { $depth != 1 } \
{
	butt_help $NET.netkey button \
		"Toggle Network Volume Color Key Pop-Up"

	butt_help $NK_F.close button \
		"Remove Network Volume Color Key Pop-Up"
}

butt_help $NET.close close \
	"Turn Off Network View, Remove From Window"

set net_help_enter \
	"Click Left Mouse Button on Hosts or Network Links for Details"


#
# Control Panel
#

butt_help $CTRL.adjust button \
	"Adjust Space Allocation Between Network and Space-Time Views"

butt_help $CTRL.rewind button \
	"Rewind - Reset Trace File to Beginning (Set to Playback Mode)"

butt_help $CTRL.stop button \
	"Stop - Temporarily Pause Trace Playback or Monitoring"

butt_help $CTRL.fwdstep button \
	"Single-Step - Show Next Single Trace Event"

butt_help $CTRL.fwd button \
	"Play - Continuous Trace Playback or Monitoring"

butt_help $CTRL.fastfwd button \
	"Fast Forward - Efficient Trace Play Advance, No Drawing"

butt_help $CTRL.time label \
	"Elapsed Trace Time in Seconds"

butt_help $CTRL.file label \
	"Current Trace File Name for Playback or Monitoring"

butt_help $CTRL.file_entry entry_nf \
	{ set_trace_file \
		"Current Trace File Name for Playback or Monitoring" }

butt_help $CTRL.playback button \
	"Set to Trace Playback Mode, Close Any Trace File, Reset Views"

butt_help $CTRL.overwrite button \
	"Set to Monitor Mode, Overwrite Current Trace File, Reset Views"


#
# Space-Time View
#

butt_help $ST.shrink button \
	"Shrink Height of Space-Time Tasks, Show More Tasks in Window"

butt_help $ST.grow button \
	"Grow Height of Space-Time Tasks, Show Fewer Tasks in Window"

butt_help $ST.task_height_entry entry \
	{ update "Explicitly Set Height of Space-Time Tasks" }

butt_help $ST.computing_label label \
	"Task is Busy Executing User Computation"

butt_help $ST.user_defined_label label \
	"Task is Marking User Defined Event (Virtual Event)"

butt_help $ST.computing_box label \
	"Task is Busy Executing User Computation"

butt_help $ST.user_defined_box label \
	"Task is Marking User Defined Event (Virtual Event)"

butt_help $ST.overhead_label label \
	"Task is in Overhead State - Executing a PVM System Call"

butt_help $ST.overhead_box label \
	"Task is in Overhead State - Executing a PVM System Call"

butt_help $ST.waiting_label label \
	"Task is Idle Waiting for a Message"

butt_help $ST.waiting_box label \
	"Task is Idle Waiting for a Message"

butt_help $ST.message_label label \
	"Animates Messages Transferred Among Tasks"

butt_help $ST.message_box label \
	"Animates Messages Transferred Among Tasks"

butt_help $ST.close close \
	"Turn Off Space-Time View, Remove From Window"

set st_help_enter \
	"Left Mouse Button Query, Middle Button Zoom, Right Button Unzoom"

set st_help_query \
	"Move Pointer to Select Query, Double Click Button to Hold Info"

set st_help_zoom \
"Hold Button and Swipe Out Area to Left or Right, Release to Apply Zoom"

set st_help_label \
	"Click Left Mouse Button to View Task ID Number (Or Custom Sort)"

set st_help_label_query \
	"Move Mouse Pointer to Select Desired Task (Or Custom Arrange)"

set correlate_help \
	"Move Pointer to Adjust Correlate Time, Double Click Button to Hold"


#
# Utilization View
#

butt_help $UT_F.computing_label label \
	"Number of Tasks Busy Executing User Computation"

butt_help $UT_F.computing_box label \
	"Number of Tasks Busy Executing User Computation"

butt_help $UT_F.overhead_label label \
	"Number of Tasks in Overhead State - Executing PVM System Calls"

butt_help $UT_F.overhead_box label \
	"Number of Tasks in Overhead State - Executing PVM System Calls"

butt_help $UT_F.waiting_label label \
	"Number of Tasks Idle Waiting for Messages"

butt_help $UT_F.waiting_box label \
	"Number of Tasks Idle Waiting for Messages"

butt_help $UT_F.close close \
	"Turn Off Utilization View, Lower Window"

set ut_help_enter \
	"Middle Mouse Button to Zoom, Right Button to Unzoom"

set ut_help_zoom \
"Hold Button and Swipe Out Area to Left or Right, Release to Apply Zoom"


#
# Message Queue View
#

butt_help $MQ_F.close close \
	"Turn Off Message Queue View, Lower Window"

set mq_help_enter \
	"Click Left Mouse Button for Message Details"

set mq_help_query \
	"Move Pointer to Select Message, Double Click to Hold Info"


#
# Call Trace View
#

butt_help $CT_F.close close \
	"Turn Off Call Trace View, Lower Window"

set ct_help_enter \
	"Click Left Mouse Button for Event Time Stamp"

set ct_help_label \
	"Click Left Mouse Button to View Task ID Number (Or Custom Sort)"

set ct_help_label_query \
	"Move Mouse Pointer to Select Desired Task (Or Custom Arrange)"


#
# Task Output View
#

butt_help $TO_F.title label \
	"Enter File Name for Saving Task Output"

butt_help $TO_F.file_entry entry \
	{ set_output_file "Enter File Name for Saving Task Output" }

butt_help $TO_F.filter_label label \
	"Enter Filter Text for Reducing Task Output"

butt_help $TO_F.filter_entry entry \
	{ set_output_filter "Enter Filter Text for Reducing Task Output" }

butt_help $TO_F.search_button button \
	"Click to Search for Next Occurrence of Target Text"

butt_help $TO_F.search_dir button \
	"Click to Toggle Searching Direction"

butt_help $TO_F.search_entry entry \
	{ "focus .xpvm" "Enter Target Text for Searching Task Output" }

butt_help $TO_F.close close \
	"Turn Off Task Output View, Lower Window"


#
# Trace Event History View
#

butt_help $TV_F.filter_label label \
	"Enter Filter Text for Reducing Trace Event History"

butt_help $TV_F.filter_entry entry \
	{ set_tevhist_filter \
		"Enter Filter Text for Reducing Trace Event History" }

butt_help $TV_F.search_button button \
	"Click to Search for Next Occurrence of Target Text"

butt_help $TV_F.search_dir button \
	"Click to Toggle Searching Direction"

butt_help $TV_F.search_entry entry \
	{ "focus .xpvm" \
		"Enter Target Text for Searching Trace Event History" }

butt_help $TV_F.close close \
	"Turn Off Trace Event History View, Lower Window"


#
# Other Host Add Dialog
#

butt_help $OH.host_entry entry \
	{ update "Enter Name of Other New Host to be Added" }

butt_help $OH.cancel button \
	"Lower Dialog, Do Not Add Host"

butt_help $OH.accept button \
	"Accept Host Name and Add Host Now"


#
# Task Spawn Dialog
#

butt_help $SP.cmd label \
	"Enter Executable Command Line for Spawn"

butt_help $SP.cmd_entry entry \
	{ update "Enter Executable Command Line for Spawn" }

butt_help $SP.buf label \
	"Enter Trace Event Buffer Size in Bytes for Spawn (0 = NONE)"

butt_help $SP.buf_entry entry \
	{ do_trace_buffer \
		"Enter Trace Event Buffer Size in Bytes for Spawn (0 = NONE)" }

butt_help $SP.trace_mask button \
	"Raise / Lower PVM Trace Mask Menus for Selecting Routines to Trace"

butt_help $SP.flags label \
	"Set PVM Spawn Command Options"

butt_help $SP.flags_debug button \
	"Toggle PvmTaskDebug Spawn Flag Setting"

butt_help $SP.flags_trace button \
	"Toggle PvmTaskTrace Spawn Flag Setting (Required for XPVM Tracing)"

butt_help $SP.flags_front button \
	"Toggle PvmMppFront Spawn Flag Setting"

butt_help $SP.flags_compl button \
	"Toggle PvmHostCompl Spawn Flag Setting"

butt_help $SP.where label \
	"Choose a Location for Spawning the Tasks"

butt_help $SP.where_host button \
	"Select a Specific Host on Which to Spawn"

butt_help $SP.where_arch button \
	"Select a Specific PVM Architecture on Which to Spawn"

butt_help $SP.where_default button \
	"Select the PVM Default Location for Spawning"

butt_help $SP.loc label \
	"Enter the Specific Host / Architecture for Spawning the Tasks"

butt_help $SP.loc_entry entry \
	{ update \
	"Enter the Specific Host / Architecture for Spawning the Tasks" }

butt_help $SP.ntasks label \
	"Enter the Number of Tasks to be Spawned"

butt_help $SP.ntasks_entry entry \
	{ update "Enter the Number of Tasks to be Spawned" }

butt_help $SP.close_on_start button \
	"Toggle Whether Spawn Dialog Automatically Lowers on Spawn"

butt_help $SP.close button \
	"Lower the Spawn Dialog, Don't Spawn Any Tasks"

butt_help $SP.start_append button \
"Spawn Tasks Now, Do Not Reset Trace File or Views - Append to Current"

butt_help $SP.start button \
	"Spawn Tasks Now, Reset Trace File and Views - Fresh Start"


#
# On-The-Fly Task Dialog
#

butt_help $OTF.trace_on button \
	"Set Tracing to On, (Click on Task or <Adjust All Tasks> to Apply)"

butt_help $OTF.trace_off button \
	"Set Tracing to Off, (Click on Task or <Adjust All Tasks> to Apply)"

butt_help $OTF.trace_mask button \
	"Raise / Lower PVM Trace Mask Menus for Selecting Routines to Trace"

butt_help $OTF.close button \
	"Lower the On-The-Fly Dialog, Don't Adjust Tracing for Any Tasks"

butt_help $OTF.refresh button \
	"Refresh List of Tasks - Check for New Tasks"

butt_help $OTF.otf_all button \
	"Adjust Tracing for All Tasks on List Now"


#
# Task Kill Dialog
#

butt_help $KL.close button \
	"Lower the Kill Dialog, Don't Kill Any Tasks"

butt_help $KL.refresh button \
	"Refresh List of Tasks - Check for New Tasks"

butt_help $KL.kill_all button \
	"Kill All Tasks on List Now"


#
# Task Signal Dialog
#

butt_help $SG.close button \
	"Lower the Signal Dialog, Don't Signal Any Tasks"

butt_help $SG.refresh button \
	"Refresh List of Tasks - Check for New Tasks"

butt_help $SG.signal_all button \
	"Signal All Tasks on List Now With the Selected Signal"


#
# Trace File Overwrite Check Dialog
#

butt_help $OV.yes button \
	"YES - Overwrite Contents of Current Trace File in Monitor Mode"

butt_help $OV.no button \
	"NO - Do Not Overwrite Trace File, Switch to Playback Mode"


#
# General Output File Overwrite Check Dialog
#

butt_help $OF.yes button \
	"YES - Overwrite Contents of Existing Task Output File"

butt_help $OF.append button \
	"APPEND - Append to Contents of Existing Task Output File"

butt_help $OF.no button \
	"NO - Do Not Overwrite Existing Task Output File"


#
# End of Button Help
#

puts " done."


#
# Actually Bring Up Main XPVM Window
#

wm deiconify .xpvm


#
# Initialize C TCL Globals
#

initialize_tcl_globals


#
# Initialize Trace Status
#

handleTraceFileStatus

trace_file_handle $trace_file

if { $trace_file_status == "OverWrite" } \
{
	trace_fwd
} \
else \
{
	trace_stop
}


#
# Reset Welcome Message
#

setMsgSafe { Welcome to XPVM }

update

