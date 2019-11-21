# ZZKTM done script.

if {[catch {
    foreach kwdb $outkwdbs {
	set name [kwdb_Name $kwdb]
	set nentries [kwdb_Len $kwdb]
	set fitsfile ${imageid}_${name}.fits
	dprintf 1 $imageid "kwdb $name has $nentries entries -> $fitsfile\n"
	kwdb_UpdateFITS $kwdb $fitsfile
    }
} err]} {
    dprintf 1 $imageid "$err\n"
}
