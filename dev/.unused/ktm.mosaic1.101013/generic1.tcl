# Generic DCA Keyword Translation Module
#
# Input:
#     inkwdbs:     A list of KWDB pointers
#     imageid:     A name to use in dprintf calls for the image being processed.
#     nextensions: Number of extensions
#
# Output:
#     outkwdbs: A list of KWDB pointers with names:
#             phu, imN
#     dprintf diagnostic messages
#
# This script takes all of the raw input KWDBs and writes them to the PHU,
# without modification, for diagnostic purposes.

# Initialize.
set outkwdbs ""
set nextensions 4
set tcl_precision 8

## Optionally dump DCA context.
#dprintf 1 $imageid "imagename=`$imagename' imagetype=`$imagetype'\n"
#dprintf 1 $imageid "nimages=$nimages $pixtype(1) $axlen1(1)x$axlen2(1)\n"
#dprintf 2 $imageid "seqno=$seqno iostat=$iostat nkwdb=$nkwdb\n"
#dprintf 2 $imageid "maxgkw=$maxgkw maxikw=$maxikw dsim=$dsim\n"
#dprintf 2 $imageid "imagefile=`$imagefile'\n"
#dprintf 2 $imageid "ktmfile=`$ktmfile'\n"

# Send initial messages.
dprintf 1 $imageid "begin generic keyword translation\n"
dprintf 1 $imageid "WARNING: generic KTM is for testing only\n"

# Copy all groups to PHU organized by group name.
set okwdb [kwdb_Open "phu"]
foreach kwdb $inkwdbs {
    set name [kwdb_Name $kwdb]
    if {$name != "NOCS_Post"} continue
    kwdb_AddEntry $okwdb ""
    kwdb_AddEntry $okwdb KWDBNAME "$name" S "Group Name"
    for {set ep [kwdb_Head $kwdb]} {$ep>0} {set ep [kwdb_Next $kwdb $ep]} {
	kwdb_CopyEntry $okwdb $kwdb $ep
    }
}
set nkw [kwdb_Len $okwdb]
lappend outkwdbs $okwdb

# Set up empty output extension databases.
for {set i 1} {$i <= $nextensions} {incr i} {
    set okwdb [kwdb_Open im$i]
    kwdb_AddEntry $okwdb ""
    lappend outkwdbs $okwdb
}

# Finished.
dprintf 1 $imageid "adding $nkw keywords to PHU\n"
dprintf 1 $imageid "generic keyword translation done\n"
