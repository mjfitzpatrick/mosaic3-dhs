# ZZKTM initialize script.

set fitsfile /tmp/pix.fits

print [format "loading %s\n" $fitsfile]
set kwdb [kwdb_OpenFITS /tmp/pix.fits]

print [format "kwdb = 0x%x\n" $kwdb]
print [format "kwdb %s has %d entries\n" \
    [kwdb_Name $kwdb] [kwdb_Len $kwdb]]
