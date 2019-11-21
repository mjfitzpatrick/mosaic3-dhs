set nimages 4
set imageid TEST
set tcl_precision 16

foreach name {PanA_Pre PanA_Post PanB_Pre PanB_Post NOCS_Pre NOCS_Post} {
    set kwdb [kwdb_Open $name]
    lappend inkwdbs $kwdb
    set f [open ${name}.dat r]
    while {[gets $f line] >= 0} {
	set keyword [string trim [lindex $line 0]]
	set value [string trim [lindex $line 1]]
	set comment [string trim [lindex $line 2]]
	if {[string is double $value]} {
	    kwdb_AddEntry $kwdb $keyword $value N $comment
	} else {
	    kwdb_AddEntry $kwdb $keyword $value S $comment
	}
    }
    close $f
}
