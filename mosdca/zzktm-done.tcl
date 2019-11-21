# ZZKTM done script.

print [format "kwdb %s has %d entries\n" \
    [kwdb_Name $output_kwdb] [kwdb_Len $output_kwdb]]

kwdb_UpdateFITS $output_kwdb zzktm.fits
