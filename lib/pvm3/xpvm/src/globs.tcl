#
# $Id: globs.tcl,v 4.50 1998/04/09 21:12:36 kohl Exp $
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

puts -nonewline "\[globs.tcl\]"
flush stdout

#
# Compatibility Globals
#

if { $tcl_major_version > 7 || $tcl_minor_version >= 4 } \
{
	set XSCROLLINCREMENT -xscrollincrement
	set YSCROLLINCREMENT -yscrollincrement

	set FRAME_OFFSET 1

	set SELECTOR -selectcolor
} \
\
else \
{
	set XSCROLLINCREMENT -scrollincrement
	set YSCROLLINCREMENT -scrollincrement

	set FRAME_OFFSET 0

	set SELECTOR -selector
}

#
# Compatibility Procs
#

if { $tcl_major_version > 7 || $tcl_minor_version >= 4 } \
{
	proc restrict_bindings { win extra } \
	{
		#debug_proc restrict_bindings entry

		bindtags $win "$win $extra"

		#debug_proc restrict_bindings exit
	}
} \
\
else \
{
	proc restrict_bindings { win extra } \
	{ }
}

if { $tcl_major_version > 7 || $tcl_minor_version >= 4 } \
{
	proc do_xview { canvas location } \
	{
		#debug_proc do_xview entry

		$canvas xview moveto 0.0

		$canvas xview scroll $location units

		#debug_proc do_xview exit
	}
} \
\
else \
{
	proc do_xview { canvas location } \
	{
		#debug_proc do_xview entry

		$canvas xview $location

		#debug_proc do_xview exit
	}
}

if { $tcl_major_version > 7 || $tcl_minor_version >= 4 } \
{
	proc do_yview { canvas location } \
	{
		#debug_proc do_yview entry

		$canvas yview moveto 0.0

		$canvas yview scroll $location units

		#debug_proc do_yview exit
	}
} \
\
else \
{
	proc do_yview { canvas location } \
	{
		#debug_proc do_yview entry

		$canvas yview $location

		#debug_proc do_yview exit
	}
}

if { $tcl_major_version > 7 || $tcl_minor_version >= 4 } \
{
	proc entry_setup { entry cmd } \
	{
		bind $entry <Return> $cmd
	}
} \
\
else \
{
	proc entry_setup { entry cmd } \
	{
		bind $entry <Return> $cmd

		bind $entry <ButtonPress-2> "%W insert insert [selection get]"

		bind $entry <3> [ list %W scan mark %x ]

		bind $entry <Button3-Motion> [ list %W scan dragto %x ]
	}
}

