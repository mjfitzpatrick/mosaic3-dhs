# DCA Keyword Translation Module for the NOAO Mosaic: Version 1
#
# Input:
#     inkwdbs:  A list of KWDB pointers with names:
#	      ics, OBSERVATORY, TELESCOPE, NULL, PRIMARY,
#             ACEB001, ACEB002, ACEB003, ACEB004,
#	      ACEB001_AMP11, ACEB001_AMP13, ACEB002_AMP11, ACEB002_AMP13,
#	      ACEB003_AMP22, ACEB002_AMP24, ACEB004_AMP22, ACEB004_AMP24,
#          The keywords from unknown databases will be copied to the PHU.
#     imageid:  A name to use in dprintf calls for the image being processed.
#          This might be the output image file name.
#
# Output:
#     outkwdbs: A list of KWDB pointers with names:
#             phu, im1, im2, im3, im4, im5, im6, im7, im8
#         If a dependent input database is not found then no extension will
#         be created.
#
#     dprintf diagnostic messages


# MOSAIC -- Main routine.

proc mosaic {} {
    global inkwdbs outkwdbs kwdbs

    # Set the kwdbs array to the database pointers indexed by database name.
    catch {unset kwdbs}
    foreach kwdb $inkwdbs {
	set name [kwdb_Name $kwdb]
	set kwdbs($name) $kwdb
    }

    # Merge controller database into amplifier database.
    # This is done so we can delete keywords as they are used.
    CopyKwdb  ACEB001_AMP11 ACEB001
    MoveKwdb  ACEB001_AMP13 ACEB001
    CopyKwdb  ACEB002_AMP11 ACEB002
    MoveKwdb  ACEB002_AMP13 ACEB002
    CopyKwdb  ACEB003_AMP22 ACEB003
    MoveKwdb  ACEB003_AMP24 ACEB003
    CopyKwdb  ACEB004_AMP22 ACEB004
    MoveKwdb  ACEB004_AMP24 ACEB004

    # Create the output keyword databases.
    DefHeader def
    PhuHeader phu
#    ExtHeader im1 1 ACEB001_AMP11 1 111    0    0    0    0
#    ExtHeader im2 2 ACEB001_AMP13 2 113 2044    0 2044    0
#    ExtHeader im3 3 ACEB002_AMP11 3 211 4227    0 4088    0
#    ExtHeader im4 4 ACEB002_AMP13 4 213 6271    0 6132    0
#    ExtHeader im5 5 ACEB003_AMP22 5 322    0 4124    0 4096
#    ExtHeader im6 6 ACEB003_AMP24 6 324 2044 4124 2044 4096
#    ExtHeader im7 7 ACEB004_AMP22 7 422 4227 4124 4098 4096
#    ExtHeader im8 8 ACEB004_AMP24 8 424 6271 4124 6132 4096
    ExtHeader im1 1 ACEB001_AMP11 1 111    0    0    0    0
    ExtHeader im2 2 ACEB001_AMP13 2 113 2044    0 2044    0
    ExtHeader im3 3 ACEB002_AMP11 3 211 4088    0 4088    0
    ExtHeader im4 4 ACEB002_AMP13 4 213 6132    0 6132    0
    ExtHeader im5 5 ACEB003_AMP22 5 322    0 4096    0 4096
    ExtHeader im6 6 ACEB003_AMP24 6 324 2044 4096 2044 4096
    ExtHeader im7 7 ACEB004_AMP22 7 422 4098 4096 4098 4096
    ExtHeader im8 8 ACEB004_AMP24 8 424 6132 4096 6132 4096
    ExtClean  im1 im2 im3 im4 im5 im6 im7 im8
    MoveUnknown phu

    # Verify important keywords.
    PhuVerify phu
    ExtVerify im1 im2 im3 im4 im5 im6 im7 im8

    # Create the output list.
    set outkwdbs ""
    foreach name {phu im1 im2 im3 im4 im5 im6 im7 im8} {
	if {[info exists kwdbs($name)]} {lappend outkwdbs $kwdbs($name)}
    }

    # Finish up.
    kwdb_Close $kwdbs(def)
}


# DefHeader -- Create a keyword database of default values.
# This database contains default entries to be used by FindEntry if
# the keyword in not in any other database and computed keywords.

proc DefHeader name {
    global imageid kwdbs

    set kwdbs($name) [kwdb_Open $name]

    # Default values
    AddEntry $name TIMESYS  "UTC approximate" S "Default time system"
    AddEntry $name RADECSYS "FK5" S "Default coordinate system"
    AddEntry $name OBSERVAT "KPNO" S "Observatory"
    AddEntry $name DETECTOR "Mosaic 1" S "Detector"
    AddEntry $name DETSIZE  "\[1:8176,1:8192\]" S "Detector size for DETSEC"
    AddEntry $name PIXSIZE1 0.15 N "Pixel size for axis 1 (microns)"
    AddEntry $name PIXSIZE2 0.15 N "Pixel size for axis 2 (microns)"
    AddEntry $name OBSERVER "<unknown>" S "Observer(s)"
    AddEntry $name PROPOSER "<unknown>" S "Proposer(s)"
    AddEntry $name PROPOSAL "<unknown>" S "Proposal title"
    AddEntry $name PROPID   "<unknown>" S "Proposal identification"

    # Computed values
    SetObstype $name
    SetTel   $name
    SetObsid $name OBSID
    SetMJD   $name DATE-OBS UTSHUT MJD-OBS "MJD of observation start"
    SetMJD   $name DATE-OBS UT MJDHDR "MJD of header creation"
    SetDate  $name DATE-NEW
    SetNccds $name NCCDS
    SetNamps $name NAMPS
}


# PhuHeader -- Set the PHU Header.

proc PhuHeader name {
    global imageid kwdbs

    dprintf 1 $imageid "creating global header\n"

    set kwdbs(phu) [kwdb_Open $name]

    MoveEntry phu ""   OBJECT   "Observation title"
    MoveEntry phu ""   OBSTYPE  "Observation type"
    CopyEntry phu ""   EXPTIME  "Exposure time (sec)"
    CopyEntry phu ""   DARKTIME "Dark time (sec)"
    MoveEntry phu ""   PREFLASH "Preflash time (sec)"
    MoveEntry phu ""   RADECSYS "Default coordinate system"
    MoveEntry phu ics  RA       "RA of observation (hr)"
    MoveEntry phu ics  DEC      "DEC of observation (deg)"
    MoveEntry phu ics  EQUINOX  "Equinox of coordinate system"

    AddEntry  phu      ""
    MoveEntry phu ""   TIMESYS
    CopyEntry phu ""   DATE-OBS
    CopyEntry phu ""   "UTSHUT TIME-OBS" "Time of observation start"
    MoveEntry phu ""   MJD-OBS
    MoveEntry phu ""   MJDHDR
    MoveEntry phu ics  "ST LSTHDR" "LST of header creation"
    MoveEntry phu ""   DATE-NEW

    AddEntry  phu      ""
    MoveEntry phu ""   OBSERVAT "Observatory"
    MoveEntry phu ""   TELESCOP "Telescope"
    MoveEntry phu ics  ZD        "Zenith distance at MJDHDR"
    MoveEntry phu ics  AIRMASS   "Airmass at MJDHDR"
    MoveEntry phu ics  "FOCUS TELFOCUS" "Telescope focus"
    MoveEntry phu ""   CORRCTOR
    MoveEntry phu ""   ADC
    if {[GetValue phu ADC] != "none"} {
	MoveEntry phu ""   ADCSTAT
	MoveEntry phu ""   ADCPAN1
	MoveEntry phu ""   ADCPAN2
    }

    AddEntry  phu      ""
    MoveEntry phu ""   DETECTOR
    MoveEntry phu ""   DETSIZE
    MoveEntry phu ""   NCCDS
    MoveEntry phu ""   NAMPS
    MoveEntry phu ""   PIXSIZE1
    MoveEntry phu ""   PIXSIZE2
    MoveEntry phu ""   PIXSCAL1
    MoveEntry phu ""   PIXSCAL2
    MoveEntry phu ""   RAPANGL
    MoveEntry phu ""   DECPANGL
    MoveEntry phu ics  "FILTNUM FILPOS"   "Filter position"
    MoveEntry phu ics  "FILTERS FILTER"   "Filter name(s)"
    MoveEntry phu ics  "MSEREADY  SHUTSTAT" "Shutter status"
    MoveEntry phu ics  "TVCAMNF   TV1FOC"
    MoveEntry phu ics  "TVCAMSF   TV2FOC"
    MoveEntry phu ics  "MSETEMP7  ENVTEM"
    MoveEntry phu ""   DEWAR "Dewar identification" "Mosaic dewar 1" S
    MoveEntry phu ics  "MSETEMP1  DEWTEM1"
    MoveEntry phu ics  "MSETEMP3  DEWTEM2"
    MoveEntry phu ics  "MSETEMP2  CCDTEM"

    AddEntry  phu      ""
    CopyEntry phu ""   CONTROLR "Controller identification" "Mosaic Arcon" S
    CopyEntry phu ""   CONHWV   "Controller hardware version" \
				"Mosaic/Arcon V?" S
    CopyEntry phu ""   "HDR_REV  CONSWV"   "Controller software version"
    CopyEntry phu ""   "WAVEFILE ARCONWD" "Date waveforms last compiled"
    CopyEntry phu ""   "WAVEMODE ARCONWM"
    CopyEntry phu ""   "DCS_TIME ARCONDCS"
    CopyEntry phu ""   "GTINDEX  ARCONGI"

    AddEntry  phu      ""
    MoveEntry phu ""   OBSERVER "Observer(s)"
    MoveEntry phu ""   PROPOSER "Proposer(s)"
    MoveEntry phu ""   PROPOSAL "Proposal title"
    MoveEntry phu ""   PROPID   "Proposal identification"
    MoveEntry phu ""   OBSID
    AddEntry  phu      KWDICT   "NOAO Keyword Dictionary: V0.0" S \
				"Keyword dictionary"

    AddEntry  phu      ""
    AddEntry  phu      CHECKSUM "<unknown>" S "Header checksum"
    AddEntry  phu      DATASUM  "<unknown>" S "Data checksum"
    AddEntry  phu      CHECKVER "<unknown>" S "Checksum version"
}


# ExtHeader -- Extension header.

proc ExtHeader {ext extver aceb ccd amp xoffin yoffin xoffout yoffout} {
    global imageid kwdbs

    if {![info exists kwdbs($aceb)]} {
	dprintf 3 $imageid "extension header `$ext' not created\n"
	return
    }

    dprintf 1 $imageid "creating extension header `$ext'\n"

    set kwdbs($ext) [kwdb_Open $ext]

#   AddEntry  $ext       BSCALE   1       N "Scale for unsigned short integers"
#   AddEntry  $ext       BZERO    32768   N "Zero for unsigned short integers"
    AddEntry  $ext       INHERIT  T       L "Inherits global header"
    AddEntry  $ext       EXTNAME  $ext S "Extension name"
    AddEntry  $ext       EXTVER   $extver N "Extension version"
    AddEntry  $ext       IMAGEID  $extver N "Image identification"
    CopyEntry $ext phu   OBSID

    AddEntry  $ext       ""
    CopyEntry $ext phu   OBJECT
    CopyEntry $ext phu   OBSTYPE
    MoveEntry $ext $aceb EXPTIME
    MoveEntry $ext $aceb DARKTIME
    CopyEntry $ext phu   PREFLASH
    CopyEntry $ext phu   FILTER

    AddEntry  $ext       ""
    CopyEntry $ext phu   RA
    CopyEntry $ext phu   DEC
    CopyEntry $ext phu   EQUINOX
    CopyEntry $ext phu   DATE-OBS
    CopyEntry $ext phu   TIME-OBS
    CopyEntry $ext phu   MJD-OBS
    CopyEntry $ext phu   LSTHDR

    AddEntry  $ext       ""
    switch $ccd {
    1 {set ccdname "Mosiac CCD $ccd \[1,1\]"}
    2 {set ccdname "Mosiac CCD $ccd \[2,1\]"}
    3 {set ccdname "Mosiac CCD $ccd \[3,1\]"}
    4 {set ccdname "Mosiac CCD $ccd \[4,1\]"}
    5 {set ccdname "Mosiac CCD $ccd \[1,2\]"}
    6 {set ccdname "Mosiac CCD $ccd \[2,2\]"}
    7 {set ccdname "Mosiac CCD $ccd \[3,2\]"}
    8 {set ccdname "Mosiac CCD $ccd \[4,2\]"}
    }
    AddEntry  $ext          CCDNAME  $ccdname S "CCD name"
    switch [string range $amp 1 end] {
    11 {set ampname "lower left (Amp$amp)"}
    12 {set ampname "lower right (Amp$amp)"}
    13 {set ampname "lower left (Amp$amp)"}
    14 {set ampname "lower right (Amp$amp)"}
    21 {set ampname "upper left (Amp$amp)"}
    22 {set ampname "upper right (Amp$amp)"}
    23 {set ampname "upper left (Amp$amp)"}
    24 {set ampname "upper right (Amp$amp)"}
    }
    AddEntry  $ext       AMPNAME  "$ccdname, $ampname" S "Amplifier name"
    MoveEntry $ext $aceb GAIN
    MoveEntry $ext $aceb RDNOISE
    CopyEntry $ext  ""   "DATAMAX  SATURATE"

    MoveEntry $ext $aceb CONTROLR "Controller identification" "Mosaic Arcon" S
    MoveEntry $ext $aceb CONHWV   "Controller hardware version" $aceb S
    #MoveEntry $ext $aceb "HDR_REV  CONSWV" "Controller software version"
    MoveEntry $ext $aceb "WAVEFILE ARCONWD" "Date waveforms last compiled"
    MoveEntry $ext $aceb "WAVEMODE ARCONWM"
    MoveEntry $ext $aceb "DCS_TIME ARCONDCS"
    MoveEntry $ext $aceb "GTINDEX  ARCONGI"
    MoveEntry $ext $aceb "GTGAIN ARCONG"
    MoveEntry $ext $aceb "GTRON ARCONRN"
    AddEntry  $ext       BPM "mscdb\$noao/mosaic3/bpm_$ext" S "Bad pixel mask"

    CcdGeom   $ext $aceb $xoffin $yoffin $xoffout $yoffout
    SetWcs    $ext

    if {![catch {set focshift [FindValue FOCSHIFT]}]} {
	AddEntry  $ext       ""
	CopyEntry $ext "" FOCSTART
	CopyEntry $ext "" FOCSTEP
	CopyEntry $ext "" FOCNEXPO
	if {$amp < 300} {
	    AddEntry $ext FOCSHIFT $focshift N "Focus sequence shift"
	} else {
	    set focshift [expr -($focshift)]
	    AddEntry $ext FOCSHIFT $focshift N "Focus sequence shift"
	}
    }

    AddEntry  $ext       ""
    AddEntry  $ext       CHECKSUM "<unknown>" S "Header checksum"
    AddEntry  $ext       DATASUM  "<unknown>" S "Data checksum"
    AddEntry  $ext       CHECKVER "<unknown>" S "Checksum version"

    # Delete keywords.
    foreach kw {DATE-OBS UT UTSHUT CCDSEC DATASEC TRIMSEC BIASSEC
	NAMPSYX AMPLIST WAVEFILE WAVEMODE DATAMAX DCS_TIME GTINDEX
	GTRON GTGAIN DEBUG0 DEBUG2 DEBUGX DEBUGY} {
	DeleteEntry $aceb $kw
    }

    if {[kwdb_Len $kwdbs($aceb)] > 0} {
	dprintf 2 $imageid \
	    "copying unknown keywords from `$aceb' to `$ext'\n"
	AddEntry  $ext ""
	MoveKwdb  $ext $aceb
    }
}


# ExtClean -- Move selected common extension keywords to the PHU header
# and remove unnecessary keywords.

proc ExtClean {args} {
    global kwdbs

    if {[llength $args] == 0} return

    set exts ""
    foreach name $args {
	if {[info exists kwdbs($name)]} {lappend exts $name}
    }
    if {$exts == ""} return

    ExtCleanKw $exts CONTROLR
    ExtCleanKw $exts CONHWV
    ExtCleanKw $exts CONSWV
    ExtCleanKw $exts ARCONWD
    ExtCleanKw $exts ARCONWM
    ExtCleanKw $exts ARCONDCS
    ExtCleanKw $exts ARCONGI
}


# MoveKwdb -- Move keywords from one database to another.

proc MoveKwdb {outkwdb inkwdb} {
    global kwdbs

    if {![info exists kwdbs($inkwdb)]} return
    if {![info exists kwdbs($outkwdb)]} return

    set kwdb $kwdbs($inkwdb)
    set okwdb $kwdbs($outkwdb)

    for {set ep [kwdb_Head $kwdb]} {$ep>0} {set ep [kwdb_Next $kwdb $ep]} {
	kwdb_CopyEntry $okwdb $kwdb $ep
	kwdb_DeleteEntry $kwdb $ep
    }
}


# CopyKwdb -- Copy keywords from one database to another.

proc CopyKwdb {outkwdb inkwdb} {
    global kwdbs

    if {![info exists kwdbs($inkwdb)]} return
    if {![info exists kwdbs($outkwdb)]} return

    set kwdb $kwdbs($inkwdb)
    set okwdb $kwdbs($outkwdb)

    for {set ep [kwdb_Head $kwdb]} {$ep>0} {set ep [kwdb_Next $kwdb $ep]} {
	kwdb_CopyEntry $okwdb $kwdb $ep
    }
}


# MoveUnknown -- Move unknown keywords to the specified database.
# There is no checking whether the keywords already exist or make sense.

proc MoveUnknown {outkwdb} {
    global kwdbs imageid

    # Remove known but unused keywords.
    foreach kw {SIMPLE BITPIX NAXIS ORIGPIC ACEB001
	ACEB002 ACEB003 ACEB004 HDR_REV} {
	DeleteEntry NULL $kw
    }
    foreach kw {NAXIS1 NAXIS2 CCDSUM GAIN RDNOISE OPICNUM UTSHUT
	FOCSTART FOCSTEP FOCNEXPO FOCSHIFT HDR_REV} {
	DeleteEntry PRIMARY $kw
    }
    foreach kw {TELID} {
	DeleteEntry TELESCOPE $kw
    }
    foreach kw {UT ICSSEQ ICSDATE ADCMODE} {
	DeleteEntry ics $kw
    }

    # Set lists of input databases and databases to exclude.
    set names [array names kwdbs]
    lappend known phu im1 im2 im3 im4 im5 im6 im7 im8 def

    # Move unknown keywords.
    foreach name $names {
	if {[lsearch $known $name] >= 0} continue
	if {[kwdb_Len $kwdbs($name)] == 0} continue

	dprintf 2 $imageid \
	    "copying unknown keywords from `$name' to `$outkwdb'\n"

	AddEntry $outkwdb ""
	MoveKwdb $outkwdb $name
    }
}


# PrintKwdb -- Print keywords from a database.
# This is used for development.

proc PrintKwdb {kwdbname} {
    global kwdbs

    if {![info exists kwdbs($kwdbname)]} return

    set kwdb $kwdbs($kwdbname)
    if {[kwdb_Len $kwdb] == 0} return

    print "Database $kwdbname -----------------------------------------------\n"
    for {set ep [kwdb_Head $kwdb]} {$ep>0} {set ep [kwdb_Next $kwdb $ep]} {
	set keyword ""
	set value ""
	set type ""
	set comment ""
	kwdb_GetEntry $kwdb $ep keyword value type comment
	if {$value == ""} {
	    print "\n"
	} else {
	    print "[format "%-8s= %20s / %s\n" $keyword $value $comment]"
	}
    }
}


# PhuVerify/ExtVerify -- Verify critical keywords.
# This routine is to be expanded as needed.

proc PhuVerify {name} {
    global kwdbs

    if {![info exists kwdbs($name)]} return

    VerifyExists $name OBJECT   warn
    VerifyExists $name OBSTYPE  warn
    VerifyExists $name OBSID    warn
    VerifyExists $name EXPTIME  warn
    VerifyExists $name FILTER   warn
    VerifyExists $name DATE-OBS warn
    VerifyExists $name TIME-OBS warn
    VerifyExists $name RA       warn
    VerifyExists $name DEC      warn
}

proc ExtVerify {args} {
    global kwdbs

    foreach name $args {
	if {![info exists kwdbs($name)]} continue

	VerifyExists $name GAIN     warn
	VerifyExists $name RDNOISE  warn
	VerifyExists $name DETSEC   warn
	VerifyExists $name CCDSUM   warn
	VerifyExists $name CCDSEC   warn
	VerifyExists $name DATASEC  warn
	VerifyExists $name BIASSEC  warn
	VerifyExists $name TRIMSEC  warn
    }
}

##### The following are utility routines #####

# CopyEntry -- Copy an entry.
#
#	entry = CopyEntry outkwdb inkwdb kws [comment [value type]]
#
# outkwdb is the name of the output keyword database and inkwdb is the name
# of the input keyword database.  If inkwdb is not specified then all the
# standard databases will be searched.  kws is a list of keyword names.  If the
# list has only one value the input and output keyword names are the same
# otherwise the first value is the input name and the second value is the
# output name.  If a comment is given then it overrides the comment from the
# input database.  If the entry is not found and no value is given a warning
# is printed otherwise the output entry is created from the value, type, and
# comment.  The return value is entry information from GetEntry if the
# keyword is found and 0 if the keyword is not found.

proc CopyEntry {outkwdb inkwdb kws args} {
    global imageid kwdbs

    set inkw  [lindex $kws 0]
    set outkw [lindex $kws 1]
    if {$outkw == ""} {set outkw $inkw}

    set comment [join [lindex $args 0]]
    set value [lindex $args 1]
    set type [lindex $args 2]

    if {[catch {
	if {$inkwdb == ""} {
	    set entry [FindEntry $inkw]
	} else {
	    set entry [GetEntry $inkwdb $inkw]
	}
	set value [lindex $entry 3]
	set type [lindex $entry 4]
	if {$comment == ""} {
	    set comment [lindex $entry 5]
	}
	kwdb_AddEntry $kwdbs($outkwdb) $outkw $value $type $comment
    } err]} {
	set entry ""
	if {$value != ""} {
	    kwdb_AddEntry $kwdbs($outkwdb) $outkw $value $type $comment
	} else {
	    dprintf 1 $imageid "can't set value for keyword `$outkw'\n"
	    dprintf 2 $imageid "$err\n"
	}
    }

    return $entry
}


# MoveEntry -- Move an entry.
#
#	entry = MoveEntry outkwdb inkwdb kws [comment [value type]]
#
# outkwdb is the name of the output keyword database and inkwdb is the name
# of the input keyword database.  If inkwdb is not specified then all the
# standard databases will be searched.  kws is a list of keyword names.  If the
# list has only one value the input and output keyword names are the same
# otherwise the first value is the input name and the second value is the
# output name.  If a comment is given then it overrides the comment from the
# input database.  If the entry is not found and no value is given a warning
# is printed otherwise the output entry is created from the value, type, and
# comment.  If an input keyword is found then after it is copied to the
# output database it is deleted from the input database.  The return value
# is entry information from GetEntry if the keyword is found and 0 if the
# keyword is not found.

proc MoveEntry {outkwdb inkwdb kws args} {

    set comment [join [lindex $args 0]]
    set value [lindex $args 1]
    set type [lindex $args 2]
    set entry [CopyEntry $outkwdb $inkwdb $kws $comment $value $type]
    set kwdb [lindex $entry 0]
    set kw [lindex $entry 2]
    if {$kwdb != ""} {DeleteEntry $kwdb $kw}
    return $entry
}


# DeleteEntry -- Delete an entry from the specified database.
# This deletes all occurences of the keyword.

proc DeleteEntry {kwdbname kw} {
    global kwdbs

    if {![info exists kwdbs($kwdbname)]} return

    set kwdb $kwdbs($kwdbname)
    while {![catch {set ep [kwdb_Lookup $kwdb $kw]}]} {
        kwdb_DeleteEntry $kwdb $ep
    }
}


# AddEntry -- Add an entry.
# Do nothing if the database is undefined.

proc AddEntry {kwdb kw args} {
    global imageid kwdbs

    if {[catch {
	if {![info exists kwdbs($kwdb)]} {
	    error "database `$kwdb' does not exist"
	}
	set value [lindex $args 0]
	set type  [lindex $args 1]
	set comment [join [lindex $args 2]]
	kwdb_AddEntry $kwdbs($kwdb) $kw $value $type $comment
    } err]} {
	dprintf 2 $imageid "can't add keyword `$kw' to database `$kwdb'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# GetEntry -- Get an entry from a keyword database with error checking.
# The return value is a list of {kwdb ep keyword value type comment}.

proc GetEntry {kwdbname kw} {
    global imageid kwdbs

    if {![info exists kwdbs($kwdbname)]} {
	return -code error \
	    "keyword `$kw' from `$kwdbname' not found"
    }
    set kwdb $kwdbs($kwdbname)
    if {[catch {
	set ep [kwdb_Lookup $kwdb $kw]
	kwdb_GetEntry $kwdb $ep keyword value type comment
	set entry [list $kwdbname $ep $keyword $value $type $comment]
    }]} {
	return -code error \
	    "keyword `$kw' from `$kwdbname' not found"
    }

    return $entry
}


# FindEntry -- Find keyword from a set of keywords database with error checking.
# The return value is a list of {kwdb ep keyword value type comment}.

proc FindEntry {kw} {
    global imageid kwdbs

    lappend search NULL PRIMARY TELESCOPE OBSERVATORY ics
    lappend search ACEB001_AMP11 ACEB001_AMP13
    lappend search ACEB002_AMP11 ACEB002_AMP13
    lappend search ACEB003_AMP22 ACEB003_AMP24
    lappend search ACEB004_AMP22 ACEB004_AMP24
    lappend search def
    foreach kwdb $search {
	if {![info exists kwdbs($kwdb)]} continue
	if {[catch {set entry [GetEntry $kwdb $kw]}]} continue
	return $entry
    }
    return -code error "keyword `$kw' not found"
}


# GetValue -- Get a value from a keyword database with error checking.

proc GetValue {kwdbname kw} {
    global imageid kwdbs

    return [lindex [GetEntry $kwdbname $kw] 3]
}


# FindValue -- Find a value from a set of keywords database with error checking.

proc FindValue {kw} {
    global imageid kwdbs

    return [lindex [FindEntry $kw] 3]
}


# VerifyExists -- Verify if a keyword exists and take error action.

proc VerifyExists {kwdb kw erract} {
    global imageid 

    if {[catch {GetEntry $kwdb $kw} err]} {
	switch $erract {
	warn  {dprintf 0 $imageid "WARNING: $err\n"}
	fatal {error $err}
	}
    }
}


# SetObstype -- Set observation type.

proc SetObstype {kwdb} {
    global kwdbs

    MoveEntry $kwdb "" "IMAGETYP OBSTYPE" "Observation type"
    if {![catch {set obstype [GetValue $kwdb OBSTYPE]}]} {
	set obstype [string tolower [string trim $obstype]]
	switch $obstype {
	bias {set obstype zero}
	}
	kwdb_SetValue $kwdbs($kwdb) OBSTYPE $obstype
    }
}


# SetTel -- Set Telescope dependent keywords.

proc SetTel {kwdb} {
    global imageid

    if {[catch {
	if {[catch {set telid [string trim [FindValue TELID]]} err1]} {
	    set tel [string trim [FindValue TELESCOP]]
	    switch $tel {
	    "KPNO 4.0 meter telescope" {set telid kp4m}
	    "KPNO 0.9 meter telescope" {set telid kp09m}
	    }
	}

	# Set telescope specific keywords.
	switch $telid {
	kp4m {
	    AddEntry  $kwdb TELID $telid S "Telescope ID"
	    AddEntry  $kwdb TELESCOP "KPNO 4.0 meter telescope" S Telescope
	    AddEntry  $kwdb RAPANGL 0. N "Position angle of RA axis (deg)"
	    AddEntry  $kwdb DECPANGL 90. N "Position angle of DEC axis (deg)"
	    AddEntry  $kwdb PIXSCAL1 0.258 N "Pixel scale for axis 1 (arcsec/pixel)"
	    AddEntry  $kwdb PIXSCAL2 0.258 N "Pixel scale for axis 2 (arcsec/pixel)"
	    AddEntry  $kwdb CORRCTOR \
		"Mayall Corrector" S "Corrector Identification"
	    AddEntry  $kwdb ADC "Mayall ADC" S "ADC Identification"
	    MoveEntry $kwdb ics "ADCMODE ADCSTAT"
            MoveEntry $kwdb ics  "MSEADCA1 ADCPAN1"
            MoveEntry $kwdb ics  "MSEADCA2 ADCPAN2"
	    }
	kp09m {
	    AddEntry  $kwdb TELID $telid S "Telescope ID"
	    AddEntry  $kwdb TELESCOP "KPNO 0.9 meter telescope" S Telescope
	    AddEntry  $kwdb RAPANGL 0. S "Position angle of RA axis (deg)"
	    AddEntry  $kwdb DECPANGL 270. S "Position angle of DEC axis (deg)"
	    AddEntry  $kwdb PIXSCAL1 0.423 N "Pixel scale for axis 1 (arcsec/pixel)"
	    AddEntry  $kwdb PIXSCAL2 0.423 N "Pixel scale for axis 2 (arcsec/pixel)"
	    AddEntry  $kwdb CORRCTOR \
		"KPNO 0.9m Corrector" S "Corrector Identification"
	    AddEntry  $kwdb ADC "none" S "ADC Identification"
	    }
	}
    } err]} {
	dprintf 1 $imageid \
	    "can't determine telescope for telescope parameters\n"
	dprintf 2 $imageid "$err\n"
	if {[info exists err1]} {dprintf 2 $imageid "and $err1\n"}
    }
}


# SetTel1 -- Set Telescope dependent keywords.

proc SetTel1 {kwdb} {
    global imageid

    if {[catch {
	if {[catch {set tel [FindValue TELESCOP]} err1]} {
	    set telid [FindValue TELID]
	    set telid [string trim $telid]
	    set tel ""
	    switch $telid {
	    kp4m {set tel "KPNO 4.0 meter telescope"}
	    }
	    set tel [string trim $tel]
	    if {tel != ""} {AddEntry  $kwdb TELESCOP $tel S Telescope}
	}

	# Set telescope specific keywords.
	set tel [string trim $tel]
	switch $tel {
	"KPNO 4.0 meter telescope" {
	    AddEntry  $kwdb TELID kp4m S "Telescope ID"
	    AddEntry  $kwdb RAPANGL 180. N "Position angle of RA axis (deg)"
	    AddEntry  $kwdb DECPANGL 270. N "Position angle of DEC axis (deg)"
	    AddEntry  $kwdb PIXSCAL1 0.26 N "Pixel scale for axis 1 (arcsec/pixel)"
	    AddEntry  $kwdb PIXSCAL2 0.26 N "Pixel scale for axis 2 (arcsec/pixel)"
	    AddEntry  $kwdb CORRCTOR \
		"Mayall Corrector" S "Corrector Identification"
	    AddEntry  $kwdb ADC "Mayall ADC" S "ADC Identification"
	    MoveEntry $kwdb ics "ADCMODE ADCSTAT"
	    }
	}
    } err]} {
	dprintf 1 $imageid \
	    "can't determine telescope for telescope parameters\n"
	dprintf 2 $imageid "$err\n"
	if {[info exists err1]} {dprintf 2 $imageid "and $err1\n"}
    }
}


# SetDate -- Set the new date and time format.
# This will eventually become DATE-OBS.

proc SetDate {outkwdb outkw} {
    global imageid

    if {[catch {
	set dateobs [FindValue DATE-OBS]
        set ut [string trim [FindValue UTSHUT]]
	scan $dateobs "%2s/%2s/%2s" dd mm yy
	set dateobs [format "19%2s-%2s-%2sT%s" $yy $mm $dd $ut]
	set timesys [string trim [FindValue TIMESYS]]
	if {$timesys == "UTC approximate"} {set dateobs [append dateobs Z]}
	AddEntry $outkwdb $outkw $dateobs S "Future DATE-OBS keyword ($timesys)"
    } err]} {
	dprintf 1 $imageid "can't compute value for keyword `$outkw'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetObsid -- Set the observation identification.

proc SetObsid {outkwdb outkw} {
     global imageid

    if {[catch {
	set telid [FindValue TELID]
        set dateobs [FindValue DATE-OBS]
        set ut [string trim [FindValue UTSHUT]]
	scan $dateobs "%d/%d/%d" dd mm yy
	scan $ut "%d:%d:%d" h m s
	set telid [string trim $telid]
	set obsid [format "%s.%02d%02d%02d.%02d%02d%02d" \
	    $telid $yy $mm $dd $h $m $s]
	AddEntry $outkwdb $outkw $obsid S "Observation ID"
    } err]} {
	dprintf 1 $imageid "can't compute value for keyword `$outkw'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# ExtCleanKw -- Move a common extension keyword to the PHU.

proc ExtCleanKw {exts kw} {

    if {[llength $exts] == 0} return
    foreach ext $exts {
	if {[catch {set value [GetValue $ext $kw]} err]} return
	if {[info exists refval]} {
	    if {$value != $refval} {
		DeleteEntry phu $kw
		return
	    }
	} else {
	    set refval $value
	}
    }
    catch {
	if {[catch {set value [GetValue phu $kw]}]} {
	    set value ""
	}
	if {$value != $refval} {
	    CopyEntry phu $ext $kw
	}
	foreach ext $exts {
	    DeleteEntry $ext $kw
	}
    }
}


# CcdGeom -- Set the CCD geometry keywords.

proc CcdGeom {outkwdb inkwdb xoffin yoffin xoffout yoffout} {
    global imageid

    if {[catch {
	# Get the required section keywords.
	set ccdsum  [GetValue PRIMARY CCDSUM]
	set ccdsec  [GetValue $inkwdb CCDSEC]
	set datasec [GetValue $inkwdb DATASEC]
	set trimsec [GetValue $inkwdb TRIMSEC]
	set biassec [GetValue $inkwdb BIASSEC]

	# Convert CCDSEC to physical CCD coordinates.
	if {[scan $ccdsum "%d %d" xsum ysum] != 2} {
	    return -code error "keyword `CCDSUM' has bad value `$ccdsum'"
	}
	if {[scan $ccdsec "\[%d:%d,%d:%d\]" cx1 cx2 cy1 cy2] != 4} {
	    return -code error "keyword `CCDSEC' has bad value `$ccdsec'"
	}
	if {[scan $datasec "\[%d:%d,%d:%d\]" dx1 dx2 dy1 dy2] != 4} {
	    return -code error "keyword `DATASEC' has bad value `$datasec'"
	}
	set xoff [expr int($xoffin/$xsum)]
	set yoff [expr int($yoffin/$ysum)]
	set cx1  [expr ($cx1-$xoff-1)*$xsum+1]
	set cx2  [expr ($cx2-$xoff-1)*$xsum+$xsum]
	set cy1  [expr ($cy1-$yoff-1)*$ysum+1]
	set cy2  [expr ($cy2-$yoff-1)*$ysum+$ysum]
	set ccdsec [format "\[%d:%d,%d:%d\]" $cx1 $cx2 $cy1 $cy2]

	# Set DETSEC.
	set ex1 [expr $cx1+$xoffout]
	set ex2 [expr $cx2+$xoffout]
	set ey1 [expr $cy1+$yoffout]
	set ey2 [expr $cy2+$yoffout]
	set detsec [format "\[%d:%d,%d:%d\]" $ex1 $ex2 $ey1 $ey2]

	# Set physical WCS to CCD coordinates.
	set ltm1 [expr 1./$xsum]
	set ltv1 [expr $dx1-($cx1+($xsum-1)/2)*$ltm1]
	set ltm2 [expr 1./$ysum]
	set ltv2 [expr $dy1-($cy1+($ysum-1)/2)*$ltm2]

	# Output the geometry keywords.
	AddEntry $outkwdb DETSEC  $detsec  S "Detector section"
	AddEntry $outkwdb CCDSIZE "\[1:2044,1:4096\]" S "CCD size"
	AddEntry $outkwdb CCDSUM  $ccdsum  S "CCD pixel summing"
	AddEntry $outkwdb CCDSEC  $ccdsec  S "CCD section"
	AddEntry $outkwdb DATASEC $datasec S "Data section"
	AddEntry $outkwdb BIASSEC $biassec S "Bias section"
#	AddEntry $outkwdb TRIMSEC $trimsec S "Trim section"
	AddEntry $outkwdb TRIMSEC $datasec S "Trim section"

	AddEntry $outkwdb ""
	AddEntry $outkwdb LTM1_1  $ltm1    N "Mapping to CCD WCS"
	AddEntry $outkwdb LTM2_2  $ltm2    N "Mapping to CCD WCS"
	AddEntry $outkwdb LTV1    $ltv1    N "Mapping to CCD WCS"
	AddEntry $outkwdb LTV2    $ltv2    N "Mapping to CCD WCS"
    } err]} {
	dprintf 1 $imageid "can't set geometry keywords for `$outkwdb'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetWcs -- Set the world coordinate system.

proc SetWcs {ext} {
    global imageid

    # Set telescope and extension dependent values.
    if {[catch {
	set telid [string trim [FindValue TELID]]
	switch [string trim [FindValue TELID]] {
	    kp4m {
	        set wcssol "mscdb\$noao/mosaic3/4meter/wcs.dat $ext"
		switch $ext {
		    im1 {  
			set crpix1  4547.7944
			set crpix2  3676.6199
			set cd1_1   3.0444971E-7
			set cd2_1  -7.1500083E-5
			set cd1_2  -7.1847942E-5
			set cd2_2   6.9822690E-7
		    } im2 {
			set crpix1  2392.4799
			set crpix2  3653.6695
			set cd1_1   5.8018553E-8
			set cd2_1  -7.2447533E-5
			set cd1_2  -7.2178412E-5
			set cd2_2   3.6159369E-7
		    } im3 {
			set crpix1   270.2529
			set crpix2  3645.4233
			set cd1_1  -2.7795839E-7
			set cd2_1  -7.2430296E-5
			set cd1_2  -7.2201956E-5
			set cd2_2   7.5022606E-8
		    } im4 {
			set crpix1 -1883.8376
			set crpix2  3668.3418
			set cd1_1  -8.3822454E-7
			set cd2_1  -7.1293379E-5
			set cd1_2  -7.1890717E-5
			set cd2_2  -1.8976123E-7
		    } im5 {
			set crpix1  4564.8096
			set crpix2  -522.7591
			set cd1_1  -7.8919705E-7
			set cd2_1  -7.1499746E-5
			set cd1_2  -7.1844082E-5
			set cd2_2  -1.2717281E-7
		    } im6 {
			set crpix1  2397.8479
			set crpix2  -503.5898
			set cd1_1  -5.4934757E-7
			set cd2_1  -7.2513027E-5
			set cd1_2  -7.2199603E-5
			set cd2_2   3.2439893E-7
		    } im7 {
			set crpix1   273.8358
			set crpix2  -502.1922
			set cd1_1  -1.8027144E-7
			set cd2_1  -7.2450952E-5
			set cd1_2  -7.2115146E-5
			set cd2_2   6.2899919E-7
		    } im8 {
			set crpix1 -1885.0409
			set crpix2  -533.7724
			set cd1_1   5.5929076E-7
			set cd2_1  -7.1192392E-5
			set cd1_2  -7.1652934E-5
			set cd2_2   8.3625615E-7
		    }
		}
	    }
	    kp09m {
	        set wcssol "mscdb\$noao/mosaic3/36inch/wcs.dat $ext"
		switch $ext {
		    im1 {  
			set crpix1  4268.3691
			set crpix2  4159.4971
			set cd1_1  -2.2321744E-6
			set cd2_1  -1.1752722E-4
			set cd1_2   1.1774019E-4
			set cd2_2  -1.7416765E-6
		    } im2 {
			set crpix1  2142.1994
			set crpix2  4144.9953
			set cd1_1  -1.9723743E-6
			set cd2_1  -1.1802055E-4
			set cd1_2   1.1791767E-4
			set cd2_2  -1.8064284E-6
		    } im3 {
			set crpix1    25.4446
			set crpix2  4137.4438
			set cd1_1  -1.8696863E-6
			set cd2_1  -1.1802422E-4
			set cd1_2   1.1790317E-4
			set cd2_2  -2.0461683E-6
		    } im4 {
			set crpix1 -2101.6228
			set crpix2  4146.3663
			set cd1_1  -1.6532483E-6
			set cd2_1  -1.1752602E-4
			set cd1_2   1.1777419E-4
			set cd2_2  -2.0864079E-6
		    } im5 {
			set crpix1  4280.5125
			set crpix2    -3.3819
			set cd1_1  -1.4632785E-6
			set cd2_1  -1.1753595E-4
			set cd1_2   1.1775296E-4
			set cd2_2  -1.9661691E-6
		    } im6 {
			set crpix1  2147.9988
			set crpix2     0.1794
			set cd1_1  -1.5382470E-6
			set cd2_1  -1.1806350E-4
			set cd1_2   1.1794949E-4
			set cd2_2  -1.6482971E-6
		    } im7 {
			set crpix1    33.0828
			set crpix2     3.3368
			set cd1_1  -1.7526105E-6
			set cd2_1  -1.1803377E-4
			set cd1_2   1.1793607E-4
			set cd2_2  -1.5718461E-6
		    } im8 {
			set crpix1 -2090.3337
			set crpix2    -3.2520
			set cd1_1  -2.2401473E-6
			set cd2_1  -1.1751998E-4
			set cd1_2   1.1777714E-4
			set cd2_2  -1.7477722E-6
		    }
		}
	    }
	}

	if {[info exists crpix1]} {
	    # Set the reference point to the RA/DEC keyword values.
	    set deg 0
	    set min 0
	    set sec 0
	    scan [GetValue $ext RA] "%d:%d:%f" deg min sec
	    set crval1 [expr 15*($deg+$min/60.+$sec/3600.)]

	    set deg 0
	    set min 0
	    set sec 0
	    scan [GetValue $ext DEC] "%d:%d:%f" deg min sec
	    set crval2 [expr ($deg+$min/60.+$sec/3600.)]

	    set ltv1   [GetValue $ext LTV1]
	    set ltv2   [GetValue $ext LTV2]
	    set ltm1_1 [GetValue $ext LTM1_1]
	    set ltm2_2 [GetValue $ext LTM2_2]
	    set crpix1 [expr ($crpix1*$ltm1_1+$ltv1)]
	    set crpix2 [expr ($crpix2*$ltm2_2+$ltv2)]
	    set cd1_1  [expr ($cd1_1/$ltm1_1)]
	    set cd2_1  [expr ($cd2_1/$ltm1_1)]
	    set cd1_2  [expr ($cd1_2/$ltm2_2)]
	    set cd2_2  [expr ($cd2_2/$ltm2_2)]

	    AddEntry $ext CTYPE1   RA---TAN S "Coordinate type"
	    AddEntry $ext CTYPE2   DEC--TAN S "Coordinate type"
	    AddEntry $ext CRVAL1   $crval1  N "Coordinate reference value"
	    AddEntry $ext CRVAL2   $crval2  N "Coordinate reference value"
	    AddEntry $ext CRPIX1   $crpix1  N "Coordinate reference pixel"
	    AddEntry $ext CRPIX2   $crpix2  N "Coordinate reference pixel"
	    AddEntry $ext CD1_1    $cd1_1   N "Coordinate transformation matrix"
	    AddEntry $ext CD2_1    $cd2_1   N "Coordinate transformation matrix"
	    AddEntry $ext CD1_2    $cd1_2   N "Coordinate transformation matrix"
	    AddEntry $ext CD2_2    $cd2_2   N "Coordinate transformation matrix"
	    AddEntry $ext WCSDIM         2  N "Coordinate system dimensionality"
	    AddEntry $ext WAT0_001 "system=image" S "Coordinate system"
	    AddEntry $ext WAT1_001 "wtype=tan axtype=ra" S "Coordinate type"
	    AddEntry $ext WAT2_001 "wtype=tan axtype=ra" S "Coordinate type"
	    AddEntry $ext WCSSOL   $wcssol  S "Plate solution"
	}
    } err]} {
	dprintf 1 $imageid "can't set WCS for `$ext'\n"
	dprintf 2 $imageid "$err\n"
    }
}



# SetNccds -- Set the number of CCDs.
# This assumes a particular amplifier identification.

proc SetNccds {kwdb kw} {
    global kwdbs

    set nccds 0
    foreach name {ACEB001_AMP11 ACEB002_AMP11 ACEB003_AMP22 ACEB004_AMP22} {
	set nccd1 0
	set nccd2 0
        if {[info exists kwdbs($name)]} {
	    set amplist [GetValue $name AMPLIST]
	    foreach amp $amplist {
		switch $amp {
		    11 {set nccd1 1}
		    12 {set nccd1 1}
		    21 {set nccd1 1}
		    22 {set nccd1 1}
		    13 {set nccd2 1}
		    14 {set nccd2 1}
		    23 {set nccd2 1}
		    24 {set nccd2 1}
		}
	    }
	    incr nccds $nccd1
	    incr nccds $nccd2
	}
    }

    AddEntry $kwdb $kw $nccds  N "Number of CCDs"
}


# SetNamps -- Set the number of amps in a list of databases.

proc SetNamps {kwdb kw} {
    global kwdbs

    set namps 0
    foreach name {ACEB001_AMP11 ACEB002_AMP11 ACEB003_AMP22 ACEB004_AMP22} {
        if {[info exists kwdbs($name)]} {
	    incr namps [llength [GetValue $name AMPLIST]]
	}
    }
    
    AddEntry $kwdb $kw $namps N "Number of Amplifiers"
}


# SetMJD - Set MJD keyword from date and time keywords.

proc SetMJD {kwdb kw_date kw_time kw comment} {
    global imageid

    if {[catch {
	set date [FindValue $kw_date]
        set time [FindValue $kw_time]]
	set MJD [ComputeMJD $date $time]
	AddEntry $kwdb $kw $MJD N $comment
    } err]} {
	dprintf 1 $imageid "can't compute value for keyword `$kw'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# ComputeMJD -- Compute Modified Julian date from date and time.
# This routine is only for the 20th century.

proc ComputeMJD {date time} {

    scan $date "%d/%d/%d" d m y
    if {$m > 2} {
	incr m
    } else {
	incr m 13
	incr y -1
    }
    set mjd [expr int($y*365.25) + int($m*30.6001) + $d + 14955]

    scan $time "%d:%d:%f" hr min sec
    set t [format "%0.8f" [expr ($hr + $min / 60. + $sec / 3600.) / 24.]]
    set t [string trimleft $t 0]

    return $mjd$t
}

########################## Execute ###########################################

dprintf 1 $imageid "begin keyword translation\n"
set tcl_precision 8
mosaic
dprintf 1 $imageid "keyword translation done\n"
