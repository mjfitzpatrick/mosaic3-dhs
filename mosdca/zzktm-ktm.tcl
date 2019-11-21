# ZZKTM dummy keyword translation module script.
# This script expects a keyword database to have been loaded into the
# Tcl variable "kwdb" by the time it is executed.

print [format "kwdb = 0x%x\n" $kwdb]
set output_kwdb [kwdb_Open output_kwdb]

dprintf 2 ktm.tcl "test dprintf: kwdb=%s, level=%s\n" $kwdb 2

for {set ep [kwdb_Head $kwdb]} \
    {$ep > 0} \
    {set ep [kwdb_Next $kwdb $ep]} \
    { kwdb_CopyEntry $output_kwdb $kwdb $ep }
