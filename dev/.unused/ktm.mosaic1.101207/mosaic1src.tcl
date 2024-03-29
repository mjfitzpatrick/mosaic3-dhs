# 		DCA Keyword Translation Module for MOSAIC
# 
# Input:
#     inkwdbs:  A list of KWDB pointers.
# 	The expected databases have names NOCS_Pre, NOCS_Post, PanA_Pre,
# 	PanA_Post.  Only the "post" database are used.
# 	The keywords from any additional unexpected databases will be copied
# 	to the PHU.
#     imageid:  A name to use in dprintf calls for the image being processed.
# 	This is typically the output image file name.
#     nimages: The number of images.
#     srcdir: Directory for source files.
#     homedir: Home directory for working files.
#     ngui: NOCS GUI file with proposal information.
#     filter: NOCS filter configuration file.
#     dbname: Filename for saving all keywords.  May be undefined to skip.
#     imname: Filename for output image.
#     datafile: Filename of data file.
#     mapfiles: Array of mapping files.
# 	The expected mapping files are NOCS and Pan.
#     mapunknown: Include keywords not in mapping?
# 	If "yes" then the mapping is much slower so this should be used
# 	during development and testing, such as when there have been changes
# 	to the upstream metadata suppliers.  Once all possible metadata have
# 	been identified they should be included in the mapping and
# 	mapunknown should be set to "no" for operational use.
#     maplevel: Level of mapping to use.
# 	0 - Include only important metadata
# 	1 - Include possibly interesting metadata
# 	2 - Include all metadata
# 
# 	For development and testing use level 1 and 2 to identify metadata
# 	that should either be explicitly handled by the KTM or added to the
# 	end of the header.  For operational use level 0 should be used.
# 
# Output:
#     outkwdbs: A list of KWDB pointers with names:
# 	The expected names are phu, im[1-N]
# 	If a dependent input database is not found then no extension will
# 	be created.
#     Database file for all keywords with name given by the dbname input.
#     Pipeline trigger file.
# 
#     Diagnostic messages using the dprintf routine.
# 
# Mapping files:
#     Mapping files consists of lines with three or four whitespace delimited
#     words.  The first word is the name of keyword in the database being
#     mapped.  The second word is the mapped name of the keyword or '-' to use
#     the same name.  The purpose of this mapping is to map long keywords to
#     unique FITS keywords since there is no requirement that the metadata
#     suppliers use FITS-style keywords.  Note that further keyword mapping of
#     level 0 keywords may occur in the KTM.  The third column is a level
#     number to be used as described previously.  Lower numbers are more
#     important and likely to be included and higher numbers are more likely
#     to be ignored.  Only level 0 keywords are explicitly processed by the
#     KTM.  The fourth column is a comment string.  This needs to be quoted if
#     it contains blanks.  This comment, if present, will override the comment
#     in the input database.


# MOSAIC1.1 -- Main routine.

proc mosaic1 {} {
    global inkwdbs imageid
    global mapfiles mapunknown maplevel
    global kwdbs
    global extname

    set outkwdbs ""
    set	NOCSkwdbs ""

    # Set the input kwdbs array to the database pointers indexed by name.
    catch {unset kwdbs}
    foreach kwdb $inkwdbs {
	set name [kwdb_Name $kwdb]
	set kwdbs($name) $kwdb
    }

    # Save keywords if desired.
    if {[catch { MosSave } err]} {dprintf 1 "MosSave Error: $err\n"}

    # Translate keywords using mapping files.  For now we only use the
    # post readout databases.  Because the NOCS database is small we
    # always map unkown keywords.
    MapNOCS NOCS_Post $mapfiles(NOCS) "" yes $maplevel
    MapKeywords PanA_Post $mapfiles(Pan) PanA $mapunknown $maplevel

    # Add detector information from files.
    SetDets

    # Set database with defaults.
    DefHeader def

    # Create PHU database.
    PhuHeader phu

    # Create extension headers.
    foreach det [lsort [array names extname]] {
	ExtHeader $det
    }
    ExtClean
    MoveUnknown phu

    # Verify important keywords.
    PhuVerify
    ExtVerify

    # Create the pipeline trigger file.
    #PLTrigger

    # Finish up.
    dprintf 1 $imageid "Cleaning up\n"
    foreach kwdb [list def $mapfiles(NOCS) $mapfiles(Pan)] {
	kwdb_Close $kwdbs($kwdb)
    }
    foreach kwdb $NOCSkwdbs {
	kwdb_Close $kwdbs($kwdb)
    }
    foreach kwdb { PanA } {
	kwdb_Close $kwdbs($kwdb)
    }
}


# DefHeader -- Create a keyword database of default values.
# This database contains default entries to be used by FindEntry if
# the keyword is not in any other database and computed keywords.

proc DefHeader name {
    global kwdbs nimages

    set kwdbs($name) [kwdb_Open $name]

    # Default values
    AddEntry $name NEXTEND  $nimages N "Number of extensions"
    AddEntry $name TIMESYS  "UTC approximate" S "Time system"
    AddEntry $name RADECSYS "FK5" S "Coordinate system"
    AddEntry $name RADECEQ  2000. N "Default Equinox"
    AddEntry $name OBSERVER "UNKNOWN" S "Observer(s)"
    AddEntry $name PROPOSER "UNKNOWN" S "Proposer(s)"
    AddEntry $name PROPOSAL "UNKNOWN" S "Proposal title"
    AddEntry $name PROPID   "UNKNOWN" S "Proposal identification"
    AddEntry $name INSTRUME "Mosaic 1.1" S "Science Instrument"

    # Map important keywords.
    MoveEntry $name "" "NOCOBJ   OBJECT"   "Observation title"
    CopyEntry $name "" "NOCTYP   OBSTYPE"  "Observation type"
    MoveEntry $name "" EXPTIME "Exposure time (sec)"
    #MoveEntry $name "" "NOCFIL   FILTER"   "Filter"
    MoveEntry $name "" "NOCTEL   TELESCOP"
    MoveEntry $name "" "NOCAO    OBSERVER"
    MoveEntry $name "" "NOCOA    OBSASST"
    MoveEntry $name "" "NOCPI    PROPOSER"
    MoveEntry $name "" "NOCPROP  PROPID"
    MoveEntry $name "" "NOCINST  INSTRUME"

    # Computed values
    SetExptime $name ""
    SetTel     $name "" TELESCOP
    SetDet     $name "" 
    SetEquin   $name "" RA DEC EQUINOX
    SetObstype $name "" OBSTYPE S "Observation type"
    SetFilter  $name ""
    SetDate    $name "" UTDATE UT DATE-OBS S "Date of observation start (UTC)"
    SetObsid   $name $name DATE-OBS OBSID S "Observation ID"
    SetMJD     $name $name DATE-OBS MJD-OBS N "MJD of observation start"
    SetFocus   $name NOCS_Post
    #SetSeqid   $name SEQID SEQNUM NOCNO NOCTOT S "Sequence ID"
}


# PhuHeader -- Set the PHU Header.

proc PhuHeader name {
    global imageid kwdbs NOCSkwdbs outkwdbs detname detval

    dprintf 1 $imageid "creating global header\n"

    set kwdbs($name) [kwdb_Open $name]
    lappend outkwdbs $kwdbs($name)

    AddEntry phu SIMPLE T N
    AddEntry phu BITPIX 32 N
    AddEntry phu NAXIS 0 N

    MoveEntry phu ""   OBJECT   "Observation title"
    MoveEntry phu ""   OBSTYPE  "Observation type"
    MoveEntry phu def   EXPTIME
    #MoveEntry phu def  EXPCOADD
    #MoveEntry phu ""   NCOADD
    MoveEntry phu ""   RADECSYS "Default coordinate system"
    CopyEntry phu ""   RADECEQ  "Default equinox"
    CopyEntry phu def  RA       "RA of observation (hr)"
    CopyEntry phu def  DEC      "DEC of observation (deg)"
    MoveEntry phu ""  "RA OBJRA"
    MoveEntry phu ""  "DEC OBJDEC"
    MoveEntry phu ""  "EPOCH OBJEPOCH"

    AddEntry  phu      ""
    MoveEntry phu ""   TIMESYS
    CopyEntry phu ""   DATE-OBS
    CopyEntry phu ""   "UT TIME-OBS"
    MoveEntry phu ""   MJD-OBS
    MoveEntry phu ""   ST

    AddEntry  phu      ""
    MoveEntry phu ""   OBSERVAT "Observatory"
    MoveEntry phu ""   TELESCOP "Telescope"
    CopyEntry phu ""   "RADECSYS TELRADEC" "Telescope coordinate system"
    CopyEntry phu ""   TELEQUIN "Equinox of tel coords"
    CopyEntry phu ""   TELRA	"RA of telescope (hr)"
    CopyEntry phu ""   TELDEC	"DEC of telescope (deg)"
    MoveEntry phu ""   TELFOCUS  "Telescope focus"
    MoveEntry phu ""   "TCPHA HA"
    MoveEntry phu ""   ZD        "Zenith distance"
    MoveEntry phu ""   AIRMASS   "Airmass" "" N

    # The following keywords are defined in SetTel.
    MoveEntry phu ""   ADC
    if {![catch {GetValue def ADCSTAT}]} {MoveEntry phu def ADCSTAT}
    if {![catch {GetValue def ADCPAN1}]} {MoveEntry phu def ADCPAN1}
    if {![catch {GetValue def ADCPAN2}]} {MoveEntry phu def ADCPAN2}

    MoveEntry phu ""   CORRCTOR

    AddEntry  phu      ""
    CopyEntry phu def  INSTRUME
    CopyEntry phu def  "INSTRUME DETECTOR" "Instrument detector"
    MoveEntry phu def  DETSIZE
    MoveEntry phu def  NCCDS
    MoveEntry phu def  NAMPS
    MoveEntry phu def  NEXTEND
    MoveEntry phu ""   PIXSIZE1
    MoveEntry phu ""   PIXSIZE2
    MoveEntry phu ""   PIXSCAL1
    MoveEntry phu ""   PIXSCAL2
    MoveEntry phu ""   RAPANGL
    MoveEntry phu ""   DECPANGL
    MoveEntry phu ""   FILTER   "Filter name"
    MoveEntry phu ""   FILTPOS
    MoveEntry phu ""   FILTERSN
    if {[info exists detval(gain,nocs)]} {
        AddEntry phu NOCGAIN "$detval(gain,nocs)" S "NOCS gain setting"}

    MoveEntry phu ""  "MSESHUTR  SHUTSTAT" "Shutter status"
    MoveEntry phu ""  "MSECAMF1  TV1FOC"
    MoveEntry phu ""  "MSECAMF2  TV2FOC"
    MoveEntry phu ""  "MSETEMP7  ENVTEM"

    MoveEntry phu ""   DEWAR "Dewar identification" "$detname(mos) Dewar" S
    MoveEntry phu ""  "MSETEMP1  DEWTEM"
    MoveEntry phu ""  "MSETEMP3  DEWTEM2"
    MoveEntry phu ""  "MSETEMP2  CCDTEM"

    AddEntry  phu      ""
    MoveEntry phu ""   OBSERVER "Observer(s)"
    MoveEntry phu ""   PROPOSER "Proposer(s)"
#    MoveEntry phu ""   PROPOSAL "Proposal title"
    MoveEntry phu ""   PROPID   "Proposal identification"
    MoveEntry phu ""   OBSID
#    MoveEntry phu ""   SEQID
#    MoveEntry phu ""   SEQNUM
    CopyEntry phu ""   EXPID "Monsoon exposure ID"
    MoveEntry phu ""   NOCID "NOCS exposure ID"

    # Delete the emails.
    DeleteEntry ngui NOCPIE
    DeleteEntry ngui NOCAOE
    DeleteEntry ngui NOCOAE

    # Monsoon information.
    AddEntry  phu      ""
    MoveEntry phu "" CONTROLR
    MoveEntry phu "" DHEFILE "Sequencer file"

    foreach kwdb $NOCSkwdbs {
	if {[kwdb_Len $kwdbs($kwdb)] > 0} {
	    AddEntry  phu ""
	    MoveKwdb  phu $kwdb
	}
    }
}


# ExtHeader -- Extension header.

proc ExtHeader {det} {
    global imageid kwdbs outkwdbs
    global extname detsize detname detval atm

    set ext $extname($det)
    scan $ext "im%d" extnum
    scan $det "%4s,%1s" ccd amp
    scan $ccd "ccd%d" ccdnum

    dprintf 1 $imageid "creating extension header `$ext'\n"

    set kwdbs($ext) [kwdb_Open $ext]
    lappend outkwdbs $kwdbs($ext)

    AddEntry $ext BITPIX 32 N
    AddEntry $ext NAXIS 2 N
    AddEntry $ext NAXIS1 [expr $detsize(amp,1)+$detsize(bias,1)] N
    AddEntry $ext NAXIS2 $detsize(amp,2) N

    AddEntry  $ext      INHERIT  T        L "Inherits global header"
    AddEntry  $ext      EXTNAME  $ext     S "Extension name"
    AddEntry  $ext      EXTVER   $extnum N "Extension version"
    AddEntry  $ext      IMAGEID  $extnum N "Image identification"
    CopyEntry $ext phu  OBSID
    #CopyEntry $ext phu  EXPID
    #CopyEntry $ext phu  NOCID

    AddEntry  $ext      ""
    CopyEntry $ext phu  OBJECT
    CopyEntry $ext phu  OBSTYPE
    CopyEntry $ext phu	EXPTIME
    #MoveEntry $ext phu {EXPTIME EXPCOADD}
    #MoveEntry $ext ""  {NCOADDS NCOADD}
    CopyEntry $ext phu  FILTER

    AddEntry  $ext      ""
    CopyEntry $ext phu  RA
    CopyEntry $ext phu  DEC
    CopyEntry $ext phu  RADECEQ
    CopyEntry $ext phu  DATE-OBS
    CopyEntry $ext phu  TIME-OBS
    CopyEntry $ext phu  MJD-OBS
    if {![catch {set mjd [GetValue $det JDEXPS]}]} {
        set mjd [expr $mjd - 2400000.5]
	AddEntry $ext MJDSTART $mjd N "MJD of observation start"
	DeleteEntry $det JDEXPS
    }
    if {![catch {set mjd [GetValue $det JDEXPE]}]} {
        set mjd [expr $mjd - 2400000.5]
	AddEntry $ext MJDEND $mjd N "MJD of observation end"
	DeleteEntry $det JDEXPE
    }
    CopyEntry $ext phu  ST

    AddEntry $ext ""
    AddEntry $ext CCDNAME $detname($ccd) S "CCD name"
    AddEntry $ext AMPNAME "$detname($ccd):$amp" S "Amplifier name"
    if {[info exists detval(gain,$det)]} {
        AddEntry $ext GAIN $detval(gain,$det) N "Gain (e/ADU)"
    } elseif {[info exists detval(gain,approx)]} {
        AddEntry $ext GAIN $detval(gain,approx) N "Approx Gain (e/ADU)"
    }
    if {[info exists detval(rdnoise,$det)]} {
        AddEntry $ext RDNOISE $detval(rdnoise,$det) N "Readout noise (e)"
    } elseif {[info exists detval(rdnoise,approx)]} {
        AddEntry $ext RDNOISE $detval(rdnoise,approx) N "Approx readout noise (e)"
    }
    if {[info exists detval(saturate,$det)]} {
        AddEntry $ext SATURATE $detval(saturate,$det) N "Saturation (ADU)"
    } elseif {[info exists detval(saturate,approx)]} {
        AddEntry $ext SATURATE $detval(saturate,approx) N "Approx saturation (ADU)"
    } elseif {[info exists detval(saturate,adc)]} {
        AddEntry $ext SATURATE $detval(saturate,adc) N "ADC saturation (ADU)"
    }

    CcdGeom $ext $ccd $amp $ext
    SetWcs $ext $ccd

    if {![catch {set focshift [GetValue def FOCSHIFT]}]} {
	AddEntry  $ext ""
	CopyEntry $ext def FOCSTART
	CopyEntry $ext def FOCSTEP
	CopyEntry $ext def FOCNEXPO
        if {$atm($amp,2) < 0} {set focshift [expr -($focshift)]}
	AddEntry $ext FOCSHIFT $focshift N \
	    "Line shift between focus exposures (pixels)"
    }

    # Delete keywords.
    foreach kw {EXPID} {
	DeleteEntry $det  $kw
    }
}


# ExtClean -- Move selected common extension keywords to the PHU header
# and remove unnecessary keywords.

proc ExtClean {} {
#    ExtCleanKw CCDSUM
}

# PhuVerify/ExtVerify -- Verify critical keywords.
# This routine is to be expanded as needed.

proc PhuVerify {} {
    VerifyExists phu OBJECT   warn
    VerifyExists phu OBSTYPE  warn
    VerifyExists phu OBSID    warn
    VerifyExists phu EXPTIME  warn
    VerifyExists phu FILTER   warn
    VerifyExists phu DATE-OBS warn
    VerifyExists phu TIME-OBS warn
    VerifyExists phu RA       warn
    VerifyExists phu DEC      warn
}

proc ExtVerify {} {
    global outkwdbs

    foreach kwdb $outkwdbs {
	set ext [kwdb_Name $kwdb]
	if {$ext == "phu"} continue
	VerifyExists $ext EXTNAME  warn
	VerifyExists $ext IMAGEID  warn
	VerifyExists $ext GAIN     warn
	VerifyExists $ext RDNOISE  warn
	VerifyExists $ext DETSEC   warn
	VerifyExists $ext CCDSUM   warn
	VerifyExists $ext CCDSEC   warn
	VerifyExists $ext DATASEC  warn
	VerifyExists $ext BIASSEC  warn
	VerifyExists $ext TRIMSEC  warn
    }
}

# MosSave -- Save all NOCS and Pan keywords in a compact format.
# This assumes that all keywords are in NOCS_Post or PanA_Post so
# any keywords not in those are not recorded.

proc MosSave {} {
    global imageid dbname kwdbs

    if {![info exists dbname]} return

    dprintf 1 $imageid "Save keywords to $dbname\n"
    if {[catch {set f [open $dbname w]} err]} {
	dprintf 2 $imageid "$err\n"
    }

    # Dump NOCS
    if {[info exists kwdbs(NOCS_Post)]} {

        set kwdb1 $kwdbs(NOCS_Post) 
        set kwdb2 $kwdbs(NOCS_Pre) 
        for {set ep1 [kwdb_Head $kwdb1]} {$ep1>0} {set ep1 [kwdb_Next $kwdb1 $ep1]} {
	    set comment ""
	    kwdb_GetEntry $kwdb1 $ep1 keyword val1 type comment
	    if {$val1 != "" && [string is double $val1]} {
	        set val1 [expr $val1]
	        if {![catch {set ival [expr int($val1)]}]} {
		    if {$val1 == $ival} {set val1 $ival}
	        }
	    } else {
	    	set val1 "'$val1'"
	    }
	    if {[catch {set val2 [kwdb_GetValue $kwdb2 $keyword]}]} {
		set val2 "-"
	    }
	    if {$val1 != "" && [string is double $val2]} {
	        set val2 [expr $val2]
	        if {![catch {set ival [expr int($val2)]}]} {
		    if {$val2 == $ival} {set val2 $ival}
	        }
	    } else {
		set val2 "'$val2'"
	    }
	    if {$val2 == $val1} {set val2 ""}

	    if {[string match "- *" $comment]} {
	        set comment [string range $comment 2 end]
	    }

	    puts $f "'$keyword',$val1,$val2,,,'$comment'"
	        
      }

    } else {
        dprintf 1 $imageid "MosSave: No NOCS keywords available, skipping....\n"
    }

    # Save or replace a few values from the Pre database.
    DeleteEntry NOCS_Post RA
    DeleteEntry NOCS_Post DEC
    DeleteEntry NOCS_Post TCPHA
    DeleteEntry NOCS_Post ZD
    MoveEntry NOCS_Post NOCS_Pre RA
    MoveEntry NOCS_Post NOCS_Pre DEC
    MoveEntry NOCS_Post NOCS_Pre TCPHA
    MoveEntry NOCS_Post NOCS_Pre ZD
    CopyEntry NOCS_Post NOCS_Pre "MSEPEDF MSEPEDF1"

    # Dump Pans
    if {[info exists kwdbs(PanA_Post)] } {

        set kwdb1 $kwdbs(PanA_Post) 
        set kwdb2 $kwdbs(PanA_Pre) 
        for {set ep1 [kwdb_Head $kwdb1]} {$ep1>0} {set ep1 [kwdb_Next $kwdb1 $ep1]} {
	    set comment ""
	    kwdb_GetEntry $kwdb1 $ep1 keyword val1 type comment
	    if {$val1 !="" && [string is double $val1]} {
	        set val1 [expr $val1]
	        if {![catch {set ival [expr int($val1)]}]} {
		    if {$val1 == $ival} {set val1 $ival}
	        }
	    } else {set val1 "'$val1'"}
	    if {[catch {set val2 [kwdb_GetValue $kwdb2 $keyword]}]} {set val2 "-"}
	    if {$val2 !="" && [string is double $val2]} {
	        set val2 [expr $val2]
	        if {![catch {set ival [expr int($val2)]}]} {
		    if {$val2 == $ival} {set val2 $ival}
	        }
	    } else {set val2 "'$val2'"}
	    if {[catch {set val3 [kwdb_GetValue $kwdb2 $keyword]}]} {set val3 "-"}
	    if {$val3 !="" && [string is double $val3]} {
	        set val3 [expr $val3]
	        if {![catch {set ival [expr int($val3)]}]} {
		    if {$val3 == $ival} {set val3 $ival}
	        }
	    } else {set val3 "'$val3'"}
	    if {[catch {set val4 [kwdb_GetValue $kwdb2 $keyword]}]} {set val4 "-"}
	    if {$val4 !="" && [string is double $val4]} {
	        set val4 [expr $val4]
	        if {![catch {set ival [expr int($val4)]}]} {
		    if {$val4 == $ival} {set val4 $ival}
	        }
	    } else {set val4 "'$val4'"}
	    if {$val4 == $val3} {set val4 ""}
	    if {$val3 == $val1} {set val3 ""}
	    if {$val2 == $val1} {set val2 ""}

	    if {[string match "- *" $comment]} {
	        set comment [string range $comment 2 end]
	    }

	    puts $f "'$keyword',$val1,$val2,$val3,$val4,'$comment'"
        }
    } else {
        dprintf 1 $imageid "MosSave: No PAN keywords available, skipping....\n"
    }


    close $f

    dprintf 1 $imageid "Done saving keywords to $dbname\n"
}


# SetSeqid -- Generate a sequence ID and sequence number.
# The data acquisition may have these but this provides values from the data
# capture side.  It is used by the pipeline and data reduction software.

proc SetSeqid {outkwdb seqidkw seqnumkw seqnokw seqtotkw type comment} {
    global imageid imname kwdbs telid lastseqid seqid seqflag

    if {[catch {
	# Set components of sequence ID.
	set dataset [join [split [FindValue PROPID] "-_.:&*?=#@"] ""]
	set expid [join [split [FindValue OBSID] "-_.:"] ""]
	set seqno [FindValue $seqnokw]
	set seqtot [FindValue $seqtotkw]

	# Get/set the last information.
	set queue [format "%s%s" "pl" $telid]
	set lastdataset "unknown"
	set lastseqnum 0
	set lastexpid "unknown"
	set lastseqno 0
	set lastseqtot $seqtot
	set lastseqflag "last"
	set lastseqid "unknown"
	if {[file exists .ktm.dat]} {
	    set f [open .ktm.dat r]
	    if {[gets $f line] > 0} {
		set queue [string trim [lindex $line 0]]
		set lastdataset [string trim [lindex $line 1]]
		set lastseqnum [string trim [lindex $line 2]]
		set lastexpid [string trim [lindex $line 3]]
		set lastseqno [string trim [lindex $line 4]]
		set lastseqtot [string trim [lindex $line 5]]
		set lastseqflag [string trim [lindex $line 6]]
		set lastseqid [format "%s_%s_%04d-%s" $queue $lastdataset $lastseqnum $lastexpid]
	    }
	    close $f
	}

	# Apply heuristics to determine when to change the sequence ID.
	set seqflag "none"

	# If the dataset name changes trigger the last dataset.
	if {$dataset != $lastdataset} {
	    set seqflag "last"
	}

	# If the sequence total has been reached trigger the dataset(s).
	if {$seqno == $seqtot} {
	    if {($seqtot == 1 || $seqtot != $lastseqtot) &&
	        $lastseqflag == "none"} {
		set seqflag "both"
	    } else {
	        set seqflag "current"
	    }

	#If there is a break in the sequence number trigger the last dataset.
	} elseif {$seqno <= $lastseqno || $lastseqno > [expr $lastseqno + 1]} {
	    set seqflag "last"
	}

	# If the last dataset has already been triggered don't trigger again.
	if {$lastseqflag == "current" || $lastseqflag == "both"} {
	    if {$seqflag == "last"} {
	        set seqflag "none"
	    } elseif {$seqflag == "both"} {
	        set seqflag "current"
	    }
	} elseif {$lastseqflag == "last" && $seqflag == "current"} {
	    set seqflag "both"
	}

	# Advance the sequence number and reset the last sequence number.
	set seqnum $lastseqnum
	if {$seqflag == "both"} {
	    incr lastseqnum 1
	    set seqnum $lastseqnum
	    set seqno 0
	} elseif {$seqflag == "current" && $lastseqflag == "last"} {
	    incr lastseqnum 1
	    set seqnum $lastseqnum
	    set seqno 0
	} elseif {$seqflag == "last"} {
	    incr lastseqnum 1
	    set seqnum $lastseqnum
	} elseif {$lastseqflag == "current" || $lastseqflag == "both"} {
	    incr lastseqnum 1
	    set seqnum $lastseqnum
	}

	# Format the sequence ID string.
	set seqid [format "%s_%s_%04d-%s" $queue $dataset $lastseqnum $expid]

	# Update/create data file.
	set f [open .ktm.dat w]
	puts $f "$queue $dataset $seqnum $expid $seqno $seqtot $seqflag"
	close $f

	# Set header.
	set seqid1 [format "%s_%s_%04d" $queue $dataset $lastseqnum]
	AddEntry $outkwdb $seqidkw $seqid1 $type $comment
	AddEntry $outkwdb $seqnumkw $seqnum N "Sequence Number"

    } err]} {
	dprintf 1 $imageid "can't compute sequence ID\n"
	dprintf 2 $imageid "$err\n"
    }
}


# PLTrigger -- Write pipeline trigger file containing the image name.

proc PLTrigger {} {
    global imageid imname kwdbs lastseqid seqid seqflag

    if {[catch {
	# Write file with pipeline trigger information.
	regsub ".fits" $imname "" name
	set fname [join [concat $name .trig] ""]
	dprintf 1 $imageid "Create pipeline trigger file $fname\n"
	set f [open $fname w]

	# Create end of sequence for last sequence.
	if {$lastseqid != "unknown" &&
	    ($seqflag == "last" || $seqflag == "both")} {
	    puts $f [format "%s.mtdend" $lastseqid]
	}

	# Create trigger file for current exposure.
	puts $f [format "%s.mtd" $seqid]

	# Create end of sequence for current sequence.
	if {$seqflag == "current" || $seqflag == "both"} {
	    puts $f [format "%s.mtdend" $seqid]
	}

	close $f
    } err]} {
	dprintf 1 $imageid "failed to generate pipeline trigger\n"
	dprintf 2 $imageid "$err\n"
    }
}

##### The following are utility routines #####

# MapKeywords -- Map keywords in input database to output database using a
# mapping database.  If an input keyword is in the mapping database then it is
# moved from the input database to the output database with the mapped keyword
# name or deleted from the input database if the mapped name is IGNORE.
# After the mapping the input database will contain only those keywords not
# in the mapping database.

proc MapKeywords {inkwdb mapkwdb outkwdb mapunknown maplevel} {
    global srcdir imageid kwdbs

    dprintf 1 $imageid "Map keywords $inkwdb to $outkwdb with $mapkwdb\n"
    dprintf 2 $imageid "  mapunknown = $mapunknown, maplevel = $maplevel\n"

    if {![info exists kwdbs($inkwdb)]} {
        dprintf 1 $imageid "No $inkwdb keywords available, returning....\n"
	return
    }

    if {![info exists kwdbs($outkwdb)]} {
        set kwdbs($outkwdb) [kwdb_Open $outkwdb]
    }
    if {![info exists kwdbs($mapkwdb)]} {
	set f [open ${srcdir}/$mapkwdb r]
	set mkwdb [kwdb_Open $mapkwdb]
	set kwdbs($mapkwdb) $mkwdb
	while {[gets $f line] >= 0} {
	    set keyword [string trim [lindex $line 0]]
	    set value [string trim [lindex $line 1]]
	    set level [string trim [lindex $line 2]]
	    set comment [string trim [lindex $line 3]]
	    if {$value == "-"} {set value $keyword}
	    if {$level > $maplevel} {
	        if {$mapunknown == "no"} continue
		set value IGNORE
	    }
	    kwdb_AddEntry $mkwdb $keyword $value S $comment
	}
	close $f
    }

    set ikwdb $kwdbs($inkwdb)
    set okwdb $kwdbs($outkwdb)
    set mkwdb $kwdbs($mapkwdb)

    if {$mapunknown == "no"} {
	for {set mep [kwdb_Head $mkwdb]} {$mep>0} {set mep [kwdb_Next $mkwdb $mep]} {
	    set mcomment ""
	    kwdb_GetEntry $mkwdb $mep mkeyword mvalue mtype mcomment

	    if {![catch {set ep [kwdb_Lookup $ikwdb $mkeyword]} err]} {
		set comment ""
		kwdb_GetEntry $ikwdb $ep keyword value type comment

		# Fix format.
		if {$value == ""} {
		    set value " "
		    set type S
		} elseif {[string is double $value]} {
		    set value [expr $value]
		    set type N
		    if {![catch {set ivalue [expr int($value)]}]} {
			if {$value == $ivalue} {
			    set value $ivalue
			}
		    }
		} else {
		    set type S
		}
		if {[string match "- *" $comment]} {
		    set comment [string range $comment 2 end]
		}

		if {$mcomment == ""} {
		    kwdb_AddEntry $okwdb $mvalue $value $type $comment
		} else {
		    if {[string match "- *" $mcomment]} {
			set mcomment [string range $mcomment 2 end]
		    }
		    kwdb_AddEntry $okwdb $mvalue $value $type $mcomment
		}
	    }
	}
	return
    }

    for {set ep [kwdb_Head $ikwdb]} {$ep>0} {set ep [kwdb_Next $ikwdb $ep]} {
	set comment ""
	kwdb_GetEntry $ikwdb $ep keyword value type comment

	# Fix format.
	if {$value == ""} {
	    set value " "
	    set type S
	    kwdb_SetValue $ikwdb $keyword $value
	} elseif {[string is double $value]} {
	    set value [expr $value]
	    set type N
	    if {![catch {set ivalue [expr int($value)]}]} {
		if {$value == $ivalue} {
		    set value $ivalue
		}
	    }
	    kwdb_SetValue $ikwdb $keyword $value
	    kwdb_SetType $ikwdb $keyword $type
	} else {
	    set type S
	    kwdb_SetType $ikwdb $keyword $type
	}
	if {[string match "- *" $comment]} {
	    set comment [string range $comment 2 end]
	    kwdb_SetComment $ikwdb $keyword $comment
	}

	# Check mapping database.
	if {![catch {set mep [kwdb_Lookup $mkwdb $keyword]} err]} {
	    set mcomment ""
	    kwdb_GetEntry $mkwdb $mep mkeyword mvalue mtype mcomment
	    if {$mvalue != "IGNORE"} {
	        if {$mcomment == ""} {
		    kwdb_AddEntry $okwdb $mvalue $value $type $comment
		} else {
		    if {[string match "- *" $mcomment]} {
			set mcomment [string range $mcomment 2 end]
		    }
		    kwdb_AddEntry $okwdb $mvalue $value $type $mcomment
		}
	    }
	    kwdb_DeleteEntry $ikwdb $ep
	}
    }

    MoveKwdb $okwdb $ikwdb
}

# MapNOCS -- Map keywords in the NOCS input database to output databases using
# a mapping database.  If an input keyword is in the mapping database then it
# is moved from the input database to the a specified output database.  After
# the mapping the input database will contain only those keywords not in the
# mapping database.
#
# In addition we add the NOCS specific .ngui file for getting the
# proposal, PI, observer, and operator values.  These override any
# supplied by NOCS.

proc MapNOCS {inkwdb mapkwdb outroot mapunknown maplevel} {
    global imageid kwdbs NOCSkwdbs srcdir homedir ngui

    # Read the NGUI information.
    # We do this first so that the keywords have precedence.
    set outkwdb "ngui"
    lappend NOCSkwdbs $outkwdb
    set kwdbs($outkwdb) [kwdb_Open $outkwdb]
    set okwdb $kwdbs($outkwdb)
    if {![catch {set f [open ${homedir}/${ngui} r]} err]} {
	while {[gets $f line] >= 0} {
	    set keyword [string trim [string range $line 0 7]]
	    set comment [string trim [string range $line 10 32]]
	    set value [string trim [string range $line 35 end]]
	    kwdb_AddEntry $okwdb $keyword $value S $comment
	}
    } else { 
	dprintf 1 $imageid "can't open NGUI file\n"
	dprintf 2 $imageid "$err\n"

	AddEntry $outkwdb NOCPI   "4meter"           S "Principal Investigator"
	AddEntry $outkwdb NOCPIE  "4meter@noao.edu"  S "PIs E-mail Address"
	AddEntry $outkwdb NOCAO   "4meter"           S "Actual Observer(s)"
	AddEntry $outkwdb NOCAOE  "4meter@noao.edu"  S "AOs E-mail Address"
	AddEntry $outkwdb NOCOA   "KPNO OA"          S "Observing Assistant"
	AddEntry $outkwdb NOCOAE  "4meter@noao.edu"  S "OAs E-mail Address"
	AddEntry $outkwdb NOCPROP "2010A-000"        S "Proposal Identifier"
	AddEntry $outkwdb NOCTEL  "KPNO Mayall 4m"   S "Telescope System"
	AddEntry $outkwdb NOCINST "Mosaic 1.1"       S "Science Instrument"
    }

    dprintf 1 $imageid "Map keywords $inkwdb with $mapkwdb\n"
    dprintf 2 $imageid "  mapunknown = $mapunknown, maplevel = $maplevel\n"

    if {![info exists kwdbs($inkwdb)]} {
        dprintf 1 $imageid "No NOCS keywords available, returning....\n"
	return
    }

    if {![info exists kwdbs($mapkwdb)]} {
	set f [open ${srcdir}/$mapkwdb r]
	set mkwdb [kwdb_Open $mapkwdb]
	set kwdbs($mapkwdb) $mkwdb
	while {[gets $f line] >= 0} {
	    set level [string trim [string range $line 0 1]]
	    set keyword [string trim [string range $line 2 9]]
	    set outkwdb [string trim [string range $line 11 41]]
	    set comment [string trim [string range $line 43 103]]
	    set dummy [string trim [string range $line 105 end]]
	    if {$level > $maplevel} {
	        if {$mapunknown == "no"} continue
		set outkwdb IGNORE
	    } else {
		set outkwdb [string range $outkwdb 0 [expr {[string last . $outkwdb]-1}]]
	    }
	    kwdb_AddEntry $mkwdb $keyword $outkwdb S $comment
	}
	close $f
    }

    set ikwdb $kwdbs($inkwdb)
    set mkwdb $kwdbs($mapkwdb)

    if {$mapunknown == "no"} {
	for {set mep [kwdb_Head $mkwdb]} {$mep>0} {set mep [kwdb_Next $mkwdb $mep]} {
	    set mcomment ""
	    kwdb_GetEntry $mkwdb $mep mkeyword outkwdb mtype mcomment
	    if {$outkwdb == "IGNORE"} continue

	    if {![catch {set ep [kwdb_Lookup $ikwdb $mkeyword]} err]} {
		set comment ""
		kwdb_GetEntry $ikwdb $ep keyword value type comment

		# Fix format.
		if {$value == ""} {
		    set value " "
		    set type S
		} elseif {[string is double $value]} {
		    set value [expr $value]
		    set type N
		    if {![catch {set ivalue [expr int($value)]}]} {
			if {$value == $ivalue} {
			    set value $ivalue
			}
		    }
		} else {
		    set type S
		    if {$keyword == "NOCOBJ"} {
			set value [nocsDecodeVerbotenChars $value]
		    } elseif {$keyword != "NOCSCR"} {
			regsub -all "_" $value " " value
		    }
		}

		# Write to output keyword database.
		set outkwdb ${outroot}${outkwdb}
		if {![info exists kwdbs($outkwdb)]} {
		    lappend NOCSkwdbs $outkwdb
		    set kwdbs($outkwdb) [kwdb_Open $outkwdb]
		}
		set okwdb $kwdbs($outkwdb)
		if {$mcomment == ""} {
		    kwdb_AddEntry $okwdb $keyword $value $type $comment
		} else {
		    kwdb_AddEntry $okwdb $keyword $value $type $mcomment
		}
	    }
	}
	return
    }

    for {set ep [kwdb_Head $ikwdb]} {$ep>0} {set ep [kwdb_Next $ikwdb $ep]} {
	set comment ""
	kwdb_GetEntry $ikwdb $ep keyword value type comment

	# Fix format.
	if {$value == ""} {
	    set value " "
	    set type S
	    kwdb_SetValue $ikwdb $keyword $value
	} elseif {[string is double $value]} {
	    set value [expr $value]
	    set type N
	    if {![catch {set ivalue [expr int($value)]}]} {
		if {$value == $ivalue} {
		    set value $ivalue
		}
	    }
	    kwdb_SetValue $ikwdb $keyword $value
	    kwdb_SetType $ikwdb $keyword $type
	} else {
	    set type S
	    if {$keyword == "NOCOBJ"} {
	        set value [nocsDecodeVerbotenChars $value]
	    } elseif {$keyword != "NOCNAME" && $keyword != "NOCSCR"} {
		regsub -all "_" $value " " value
	    }
	    kwdb_SetType $ikwdb $keyword $type
	}

	# Check mapping database.
	if {![catch {set mep [kwdb_Lookup $mkwdb $keyword]} err]} {
	    set mcomment ""
	    kwdb_GetEntry $mkwdb $mep mkeyword outkwdb mtype mcomment
	    if {$outkwdb != "IGNORE"} {
		set outkwdb ${outroot}${outkwdb}
		if {![info exists kwdbs($outkwdb)]} {
		    lappend NOCSkwdbs $outkwdb
		    set kwdbs($outkwdb) [kwdb_Open $outkwdb]
		}
		set okwdb $kwdbs($outkwdb)
	        if {$mcomment == ""} {
		    kwdb_AddEntry $okwdb $keyword $value $type $comment
		} else {
		    kwdb_AddEntry $okwdb $keyword $value $type $mcomment
		}
	    }
	    kwdb_DeleteEntry $ikwdb $ep
	}
    }

    set outkwdb ${outroot}unknown
    if {![info exists kwdbs($outkwdb)]} {
	lappend NOCSkwdbs $outkwdb
	set kwdbs($outkwdb) [kwdb_Open $outkwdb]
    }
    set okwdb $kwdbs($outkwdb)
    MoveKwdb $okwdb $ikwdb
}


# nocsDecodeVerbotenChars -- Decode strings encoded by NOCS.

proc nocsDecodeVerbotenChars { D } {

    # replace .ws. with single-space
    set I [regsub -all {\.ws\.} ${D} "\ " D]
    # replace .dq. with double-quote
    set I [regsub -all {\.dq\.} ${D} "\"" D]
    # replace .sq. with single-quote
    set I [regsub -all {\.sq\.} ${D} "\'" D]
    # replace .eq. with equal-sign
    set I [regsub -all {\.eq\.} ${D} "\=" D]
    # replace .us. with underscore
    set I [regsub -all {\.us\.} ${D} "\_" D]
    # replace .bs. with backslash - NOT!
    #set I [regsub -all {\.bs\.} ${D} {\\} D]
    return ${D}
}


# MoveKwdb -- Move keywords from one database to another.

proc MoveKwdb {outkwdb inkwdb} {
    global kwdbs

    if {![info exists kwdbs($inkwdb)]} return
    if {![info exists kwdbs($outkwdb)]} {
        set kwdbs($outkwdb) [kwdb_Open $outkwdb]
    }

    set kwdb $kwdbs($inkwdb)
    set okwdb $kwdbs($outkwdb)

    if {[kwdb_Len $kwdb] == 0} return

    for {set ep [kwdb_Head $kwdb]} {$ep>0} {set ep [kwdb_Next $kwdb $ep]} {
	kwdb_CopyEntry $okwdb $kwdb $ep
	kwdb_DeleteEntry $kwdb $ep
    }
}


# CopyKwdb -- Copy keywords from one database to another.

proc CopyKwdb {outkwdb inkwdb} {
    global kwdbs

    if {![info exists kwdbs($inkwdb)]} return
    if {![info exists kwdbs($outkwdb)]} {
        set kwdbs($outkwdb) [kwdb_Open $outkwdb]
    }

    set kwdb $kwdbs($inkwdb)
    set okwdb $kwdbs($outkwdb)

    for {set ep [kwdb_Head $kwdb]} {$ep>0} {set ep [kwdb_Next $kwdb $ep]} {
	kwdb_CopyEntry $okwdb $kwdb $ep
    }
}


# MoveUnknown -- Move unknown keywords to the specified database.
# There is no checking whether the keywords already exist or make sense.

proc MoveUnknown {outkwdb} {
    global kwdbs NOCSkwdbs outkwdbs imageid mapfiles

#    # Remove known but unused keywords.
#    foreach kw {SIMPLE BITPIX NAXIS ORIGPIC ACEB001
#	ACEB002 ACEB003 ACEB004 HDR_REV} {
#	DeleteEntry NULL $kw
#    }
#    foreach kw {NAXIS1 NAXIS2 CCDSUM GAIN RDNOISE OPICNUM UT HDR_REV
#	DATE-OBS DETECTOR UT} {
#	DeleteEntry PRIMARY $kw
#    }
#    foreach kw {TELID} {
#	DeleteEntry TELESCOPE $kw
#    }
#    foreach kw {UT ICSSEQ ICSDATE ADCMODE DATE-OBS EPOCH} {
#	DeleteEntry NOCS $kw
#    }
#    foreach kw {FOCSTART FOCSTEP FOCSHIFT FOCNEXPO} {
#	DeleteEntry FOCUS $kw
#    }

    # Set lists of databases to exclude.
    lappend known def $NOCSkwdbs PanA
    lappend known $mapfiles(NOCS) $mapfiles(Pan)
    lappend known NOCS_Pre NOCS_Post PanA_Pre PanA_Post
    foreach kwdb $outkwdbs {
	lappend known [kwdb_Name $kwdb]
    }

    # Move unknown keywords.
    foreach name [array names kwdbs] {
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
	if {[lindex $entry 0] == $outkwdb && $inkw == $outkw} {
	    if {$value != ""} {
	        kwdb_SetValue $kwdbs($outkwdb) $outkw $value
		set entry [GetEntry $outkwdb $inkw]
	    }
	    if {$type != ""} {
	        kwdb_SetType $kwdbs($outkwdb) $outkw $type
		set entry [GetEntry $outkwdb $inkw]
	    }
	    if {$comment != ""} {
	        kwdb_SetComment $kwdbs($outkwdb) $outkw $comment
		set entry [GetEntry $outkwdb $inkw]
	    }
	} else {
	    set value [lindex $entry 3]
	    if {$type == ""} {
		set type [lindex $entry 4]
	    }
	    if {$type != "S"} {
		set value [string trim $value]
	    }
	    if {$comment == ""} {
		set comment [lindex $entry 5]
	    }
	    kwdb_AddEntry $kwdbs($outkwdb) $outkw $value $type $comment
	}
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

    set inkw  [lindex $kws 0]
    set outkw [lindex $kws 1]
    if {$outkw == ""} {set outkw $inkw}

    set comment [join [lindex $args 0]]
    set value [lindex $args 1]
    set type [lindex $args 2]
    set entry [CopyEntry $outkwdb $inkwdb $kws $comment $value $type]
    set inkwdb [lindex $entry 0]
    set inkw [lindex $entry 2]
    if {$outkwdb != $inkwdb || $outkw != $inkw} {
	if {$inkwdb != ""} {DeleteEntry $inkwdb $inkw}
    }
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
    global imageid kwdbs NOCSkwdbs

    set search $NOCSkwdbs
    lappend search PanA phu def
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

proc SetObstype {outkwdb inkwdb kws args} {
    global imageid kwdbs

    set inkw  [lindex $kws 0]
    set outkw [lindex $kws 1]
    if {$outkw == ""} {set outkw $inkw}

    if {[catch {
	MoveEntry $outkwdb $inkwdb $kws $args
	set obstype [string trim [GetValue $outkwdb $outkw]]
	set obstype [string tolower $obstype]
	switch -regexp $obstype {
	dark {
	    if {[GetValue $outkwdb EXPTIME] < 1.} {
	        set obstype zero
	    } else {
	        set obstype dark
	    } }
	dflat {set obstype "dome flat"}
	tflat {set obstype "sky flat"}
	focus {set obstype focus}
	sky {set obstype object}
	test {set obstype test}
	zero {set obstype zero}
	default {set obstype object}
	}
	kwdb_SetValue $kwdbs($outkwdb) $outkw $obstype
    } err]} {
	dprintf 1 $imageid "can't set value for keyword `$outkw'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetDet -- Set detector dependent keywords.

proc SetDet {outkwdb inkwdb} {
    global imageid nimages
    global detname detsize

    if {[catch {
        set ndets [expr [array size detname]-1]
	set size [format "\[1:%d,1:%d\]" $detsize(mos,1) $detsize(mos,2)]

	AddEntry $outkwdb INSTRUME $detname(mos) S "Science Instrument"
	AddEntry $outkwdb DETSIZE  $size S "Mosaic detector size"
	AddEntry $outkwdb NCCDS  $ndets N "Number of CCDs in mosaic"
	AddEntry $outkwdb NAMPS  $nimages N "Number of amplifiers in mosaic"
	AddEntry $outkwdb PIXSIZE1 15. N "Pixel size for axis 1 (microns)"
	AddEntry $outkwdb PIXSIZE2 15. N "Pixel size for axis 2 (microns)"
	MoveEntry $outkwdb $inkwdb "MSEIDENT CONTROLR"
	#AddEntry $outkwdb CONHWV "Monsoon ??" S "Controller software version"
	#AddEntry $outkwdb CONSWV "Monsoon ??" S "Controller software version"
    } err]} {
	dprintf 1 $imageid \
	    "can't determine detector for detector parameters\n"
	dprintf 2 $imageid "$err\n"
	if {[info exists err1]} {dprintf 2 $imageid "and $err1\n"}
    }
}


# SetEquin -- Set default equinox and precess coordinates.

proc SetEquin {outkwdb inkwdb rakw deckw eqkw} {
    global imageid

    if {[catch {
	# Get the RA/DEC.
	CopyEntry $outkwdb $inkwdb "$rakw  TELRA"    "RA of telescope (hr)"
	CopyEntry $outkwdb $inkwdb "$deckw TELDEC"   "DEC of telescope (deg)"
	MoveEntry $outkwdb $inkwdb "$eqkw  TELEQUIN" "Equinox of tel coords" "" N

	set deg 0
	set min 0
	set sec 0
	scan [GetValue $outkwdb TELRA] "%d:%d:%f" deg min sec
	set ra [expr 15*($deg+$min/60.+$sec/3600.)]

	set deg 0
	set min 0
	set sec 0
	set sgn [regsub -- "-" [GetValue $outkwdb TELDEC] "" dec]
	scan $dec "%d:%d:%f" deg min sec
	if {$sgn} {
	    set dec [expr -($deg+$min/60.+$sec/3600.)]
	} else {
	    set dec [expr ($deg+$min/60.+$sec/3600.)]
	}

	if {abs($ra)<0.000001 && abs($dec) < 0.000001} {
	    # Get the RA/DEC.
	    CopyEntry $outkwdb $inkwdb "RA  TELRA"    "RA of telescope (hr)"
	    CopyEntry $outkwdb $inkwdb "DEC TELDEC"   "DEC of telescope (deg)"

	    set deg 0
	    set min 0
	    set sec 0
	    scan [GetValue $outkwdb TELRA] "%d:%d:%f" deg min sec
	    set ra [expr 15*($deg+$min/60.+$sec/3600.)]

	    set deg 0
	    set min 0
	    set sec 0
	    set sgn [regsub -- "-" [GetValue $outkwdb TELDEC] "" dec]
	    scan $dec "%d:%d:%f" deg min sec
	    if {$sgn} {
		set dec [expr -($deg+$min/60.+$sec/3600.)]
	    } else {
		set dec [expr ($deg+$min/60.+$sec/3600.)]
	    }
	}

	# Precess.
	set telequin [GetValue $outkwdb TELEQUIN]
	set defequin [GetValue $outkwdb RADECEQ]
	set result [precess $ra $dec $telequin $defequin]
	set ra [lindex $result 0]
	set dec [lindex $result 1]

	set ra [expr $ra/15.]
	set deg [expr int($ra)]
	set dmin [expr ($ra-$deg)*60.]
	set min [expr int($dmin)]
	set dsec [expr ($dmin-$min)*60.]
	set sec [expr int($dsec)]
	set secdeci [expr int(($dsec-$sec)*100)]

	if {$secdeci > 99} {set secdeci 99}
	if {$sec >= 60} {
	    set sec 0
	    set min [expr ($min+1)]
	}
	if {$min >= 60} {
	    set min 0
	    set deg [expr ($deg+1)]
	}

	set ra [format "%02d:%02d:%02d.%02d" $deg $min $sec $secdeci]

	if {$sgn} {set dec [expr -($dec)]}

	set deg [expr int($dec)]
	set dmin [expr ($dec-$deg)*60.]
	set min [expr int($dmin)]
	set dsec [expr ($dmin-$min)*60.]
	set sec [expr int($dsec)]
	set secdeci [expr int(($dsec-$sec)*100)]

	if {$secdeci > 99} {set secdeci 99}
	if {$sec >= 60} {
	    set sec 0
	    set min [expr ($min+1)]
	}
	if {$min >= 60} {
	    set min 0
	    set deg [expr ($deg+1)]
	}

	if {$sgn} {
	    set dec [format "-%02d:%02d:%02d.%02d" $deg $min $sec $secdeci]
	} else {
	    set dec [format "%02d:%02d:%02d.%02d" $deg $min $sec $secdeci]
	}

	AddEntry $outkwdb RA  $ra S "Default RA (hr)"
	AddEntry $outkwdb DEC $dec S "Default DEC (deg)"
    } err]} {
	dprintf 1 $imageid "can't set default equinox\n"
	dprintf 2 $imageid "$err\n"
	if {[info exists err1]} {dprintf 2 $imageid "and $err1\n"}
    }
}


# SetTel -- Set Telescope dependent keywords.

proc SetTel {outkwdb inkwdb kws args} {
    global kwdbs imageid telid

    set inkw  [lindex $kws 0]
    set outkw [lindex $kws 1]
    if {$outkw == ""} {set outkw $inkw}

    if {[catch {
	MoveEntry $outkwdb $inkwdb $kws $args

	set tel [string trim [GetValue $outkwdb $outkw]]
	switch $tel {
	"KPNO Mayall 4m" {set telid kp4m}
	"WIYN 0.9m" {set telid kp09m}
	default {set telid kp4m}
	}

	# Set telescope specific keywords.
	switch $telid {
	kp4m {
	    kwdb_SetValue $kwdbs($outkwdb) $outkw "KPNO 4.0 meter telescope"
	    AddEntry  $outkwdb OBSERVAT KPNO S "Observatory"
	    AddEntry  $outkwdb TELID $telid S "Telescope ID"
	    AddEntry  $outkwdb RAPANGL 0. N "Position angle of RA axis (deg)"
	    AddEntry  $outkwdb DECPANGL 90. N "Position angle of DEC axis (deg)"
	    AddEntry  $outkwdb PIXSCAL1 0.258 N "Pixel scale for axis 1 (arcsec/pixel)"
	    AddEntry  $outkwdb PIXSCAL2 0.258 N "Pixel scale for axis 2 (arcsec/pixel)"
	    AddEntry  $outkwdb ADC "Mayall ADC" S "ADC Identification"
	    AddEntry  $outkwdb CORRCTOR \
		"Mayall Corrector" S "Corrector Identification"
            MoveEntry $outkwdb $inkwdb  "ADCMODE ADCSTAT"
            MoveEntry $outkwdb $inkwdb  "MSEADCA1 ADCPAN1"
            MoveEntry $outkwdb $inkwdb  "MSEADCA2 ADCPAN2"
	    }
	kp09m {
	    kwdb_SetValue $kwdbs($outkwdb) $outkw "KPNO 0.9 meter telescope"
	    AddEntry  $outkwdb OBSERVAT KPNO S "Observatory"
	    AddEntry  $outkwdb TELID $telid S "Telescope ID"
	    AddEntry  $outkwdb RAPANGL 0. N "Position angle of RA axis (deg)"
	    AddEntry  $outkwdb DECPANGL 270. N "Position angle of DEC axis (deg)"
	    AddEntry  $outkwdb PIXSCAL1 0.423 N "Pixel scale for axis 1 (arcsec/pixel)"
	    AddEntry  $outkwdb PIXSCAL2 0.423 N "Pixel scale for axis 2 (arcsec/pixel)"
	    AddEntry  $outkwdb CORRCTOR \
		"KPNO 0.9m Corrector" S "Corrector Identification"
	    AddEntry  $outkwdb ADC "none" S "ADC Identification"
	    }
	}
    } err]} {
	dprintf 1 $imageid \
	    "can't determine telescope for telescope parameters\n"
	dprintf 2 $imageid "$err\n"
	if {[info exists err1]} {dprintf 2 $imageid "and $err1\n"}
    }
}


# SetFocus -- Set focus information including focus sequence.

proc SetFocus {outkwdb inkwdb} {
    global kwdbs imageid

    if {[catch {

	# Make sure the focus is set by doing this first.
	MoveEntry $outkwdb "" "MSEPEDF TELFOCUS"

	# Do focus sequence.
	if {[GetValue $outkwdb OBSTYPE] == "focus"} {
	    MoveEntry $outkwdb "" "MSEPEDF1 FOCSTART"
	    MoveEntry $outkwdb "" "NOCGPXPS FOCSHIFT"
	    if {![catch {FindValue NOCFITER}]} {
		MoveEntry $outkwdb "" "NOCFITER FOCNEXPO"
	    } else {
		CopyEntry $outkwdb "" "NOCTOT FOCNEXPO" "Number of focus values"
	    }
	    set nfocus [GetValue $outkwdb FOCNEXPO]
	    if {![catch {FindValue NOCFSTEP}]} {
		MoveEntry $outkwdb "" "NOCFSTEP FOCSTEP"
	    } else {
		set focus1 [GetValue $outkwdb FOCSTART]
		set focus2 [GetValue $outkwdb TELFOCUS]
		if {$nfocus > 0} {
		    set focstep [expr ($focus2 - $focus1) / $nfocus]
		} else {
		    set focstep 0
		}
		AddEntry $outkwdb FOCSTEP $focstep N "Focus step"
	    }

	    if {$nfocus <= 1} {
		DeleteEntry $outkwdb FOCSTART
		DeleteEntry $outkwdb FOCSHIFT
		DeleteEntry $outkwdb FOCNEXPO
		DeleteEntry $outkwdb FOCSTEP
	    }
	}

    } err]} {
	dprintf 1 $imageid \
	    "can't determine focus information\n"
	dprintf 2 $imageid "$err\n"
	if {[info exists err1]} {dprintf 2 $imageid "and $err1\n"}
    }
}

# SetExptime -- Set exposure time keywords.

proc SetExptime {outkwdb inkwdb} {
    global kwdbs imageid telid
    #global ncoadds

    if {[catch {
	set exptime1 [FindValue EXPTIME]
	if {[catch {set ncoadds [FindValue NCOADDS]}]} {set ncoadds 1}
	if {$ncoadds < 1} {set ncoadds 1}
	set ncoadds 1

	set exptime [expr $ncoadds * $exptime1]
	AddEntry $outkwdb EXPTIME $exptime N "Exposure time (sec)"
	#AddEntry $outkwdb EXPCOADD $exptime1 N "Single coadd exp time (sec)"
	#AddEntry $outkwdb NCOADD $ncoadds N "Number of coadds"
    } err]} {
	dprintf 1 $imageid \
	    "can't determine exposure times\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetDate -- Set the date and time.

proc SetDate {outkwdb inkwdb datekw timekw kw type comment} {
    global imageid

    if {[catch {
	MoveEntry $outkwdb $inkwdb $datekw
	MoveEntry $outkwdb $inkwdb $timekw
	set date [GetValue $outkwdb $datekw]
        set time [GetValue $outkwdb $timekw]]
	DeleteEntry $outkwdb $datekw
	if {[scan $date "%d/%d/%d" mm dd yy] == 3} {
	    set yy [expr int(fmod($yy,100))]
	    if {$yy > 96} {set century 1900} else {set century 2000}
	    set yy [expr $century+$yy]
	} elseif {[scan $date "%d-%d-%d" yy mm dd] != 3} {
	    error "unknown format for `$date'"
	}
	scan $time "%d:%d:%f" h m s
	set dateobs [format "%04d-%02d-%02dT%02d:%02d:%04.1f" \
	    $yy $mm $dd $h $m $s]
	set timesys [string trim [FindValue TIMESYS]]
	AddEntry $outkwdb $kw $dateobs $type $comment
    } err]} {
	dprintf 1 $imageid "can't compute value for keyword `$kw'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetObsid -- Set the observation identification.

proc SetObsid {outkwdb inkwdb date kw type comment} {
     global imageid telid

    if {[catch {
	scan [GetValue $inkwdb $date] "%d-%d-%dT%d:%d:%d" yy mm dd h m s
	set obsid [format "%s.%04d%02d%02dT%02d%02d%02d" \
	    $telid $yy $mm $dd $h $m $s]
	AddEntry $outkwdb $kw $obsid $type $comment
    } err]} {
	dprintf 1 $imageid "can't compute value for keyword `$kw'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetFilter -- Set filter.

proc SetFilter {outkwdb inkwdb} {
    global imageid kwdbs filters filter filtersn

    if {[catch {
	MoveEntry $outkwdb $inkwdb "NOCFIL FILTER" "Filter name"
	MoveEntry $outkwdb $inkwdb "NOCFSN FILTERSN" "Filter serial number"
	MoveEntry $outkwdb $inkwdb "MSEFILTR FILTPOS" "Instrument filter position"

	# Since some filters start with a number force the type.
	kwdb_SetType $kwdbs($outkwdb) FILTER S
	kwdb_SetType $kwdbs($outkwdb) FILTERSN S

	# Read the filter configuration information.
	if {[catch { 
	    set f [open $filters]
	    set filtpos [GetValue $outkwdb FILTPOS]
	    while {[gets $f line] >= 0} {
		set filtinfo [split [string trim $line] " "]
		if {[lindex $filtinfo 0] != $filtpos} continue
		set filter [join [lrange $filtinfo 2 end]]
		kwdb_SetValue $kwdbs($outkwdb) FILTER $filter
		set sn [lrange $filtinfo end end]
		if {[regexp {[ckCK#][ ]*[0-9][0-9][0-9][0-9]$} $sn]} {
		    set filtersn $sn
		    kwdb_SetValue $kwdbs($outkwdb) FILTERSN $filtersn
		}
		break
	    }
	    close $f
	} err1]} {
	    dprintf 1 $imageid "error reading filter configuration file\n"
	    dprintf 2 $imageid "$err1\n"
	}

	# These variables are for array indexing.
	set filter [string trim [GetValue $outkwdb FILTER]]
	set filtersn [string trim [GetValue $outkwdb FILTERSN]]
	regsub -all {[^0-9a-zA-Z]} $filter "_" filter
    } err]} {
	dprintf 1 $imageid "can't set value for keyword FILTER\n"
	dprintf 2 $imageid "$err\n"
    }
}


# ExtCleanKw -- Move a common extension keyword to the PHU.

proc ExtCleanKw {kw} {
    global outkwdbs

    foreach kwdb $outkwdbs {
	set ext [kwdb_Name $kwdb]
	if {$ext == "phu"} continue

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
#
# In this version we handle binning but not ROI.

proc CcdGeom {outkwdb ccd amp ext} {
    global imageid
    global detsize ltm ltv atm atv dtm dtv

    if {[catch {
	# Set the binning.
    	set colbin [FindValue colBin]
	set rowbin [FindValue rowBin]

	# Set the CCD region.
	switch $amp {
	A {set cx1 1; set cx2 $detsize(amp,1)
	   set cy1 1; set cy2 $detsize(amp,2)}
	B {set cx1 [expr $detsize(ccd,1)-$detsize(amp,1)+1]
	   set cx2 [expr $cx1+$detsize(amp,1)-1]
	   set cy1 1; set cy2 $detsize(amp,2)}
	C {set cx1 1; set cx2 $detsize(amp,1)
	   set cy1 [expr $detsize(ccd,2)-$detsize(amp,2)+1]
	   set cy2 [expr $cy1+$detsize(amp,2)-1]}
	D {set cx1 [expr $detsize(ccd,1)-$detsize(amp,1)+1]
	   set cx2 [expr $cx1+$detsize(amp,1)-1]
	   set cy1 [expr $detsize(ccd,2)-$detsize(amp,2)+1]
	   set cy2 [expr $cy1+$detsize(amp,2)-1]}
	}

	# Adjust transformation for binning.
	set ltmbin(1) [expr double($ltm(1))/$colbin]
	set ltvbin(1) [expr 1.-$ltmbin(1)*$cx1-(1.-$ltmbin(1))/2.]
	set ltmbin(2) [expr double($ltm(2))/$rowbin]
	set ltvbin(2) [expr 1.-$ltmbin(2)*$cy1-(1.-$ltmbin(2))/2.]

	# Set the sections.
	set ccdsec [format "\[%d:%d,%d:%d\]" $cx1 $cx2 $cy1 $cy2]

	set ic1 [expr int($ltmbin(1)*$cx1+$ltvbin(1)+0.5)]
	set ic2 [expr int($ltmbin(1)*$cx2+$ltvbin(1)+0.5)]
	set il1 [expr int($ltmbin(2)*$cy1+$ltvbin(2)+0.5)]
	set il2 [expr int($ltmbin(2)*$cy2+$ltvbin(2)+0.5)]
	set datasec [format "\[%d:%d,%d:%d\]" $ic1 $ic2 $il1 $il2]

	set bc1 [expr $ic2+1]
	set bc2 [expr $ic2+$detsize(bias,1)]
	set bl1 $il1
	set bl2 $il2
	set biassec [format "\[%d:%d,%d:%d\]" $bc1 $bc2 $bl1 $bl2]

	set dx1 [expr int($dtm($ccd,1)*$cx1+$dtv($ccd,1)+0.5)]
	set dx2 [expr int($dtm($ccd,1)*$cx2+$dtv($ccd,1)+0.5)]
	set dy1 [expr int($dtm($ccd,2)*$cy1+$dtv($ccd,2)+0.5)]
	set dy2 [expr int($dtm($ccd,2)*$cy2+$dtv($ccd,2)+0.5)]
	set detsec [format "\[%d:%d,%d:%d\]" $dx1 $dx2 $dy1 $dy2]

	set as1 [expr int($atm($amp,1)*$cx1+$atv($amp,1)+0.5)]
	set as2 [expr int($atm($amp,1)*$cx2+$atv($amp,1)+0.5)]
	set ap1 [expr int($atm($amp,2)*$cy1+$atv($amp,2)+0.5)]
	set ap2 [expr int($atm($amp,2)*$cy2+$atv($amp,2)+0.5)]
	set ampsec [format "\[%d:%d,%d:%d\]" $as1 $as2 $ap1 $ap2]

	# Output the geometry keywords.
	AddEntry $outkwdb CCDSUM  "$colbin $rowbin"  S "CCD pixel summing"
	AddEntry $outkwdb CCDSEC  $ccdsec     S "CCD section"
	AddEntry $outkwdb AMPSEC  $ampsec     S "Amplifier section"
	AddEntry $outkwdb DATASEC $datasec    S "Data section"
	AddEntry $outkwdb DETSEC  $detsec     S "Detector section"
	AddEntry $outkwdb BIASSEC $biassec    S "Bias section"
	AddEntry $outkwdb TRIMSEC $datasec    S "Trim section"

	AddEntry $outkwdb ""
	AddEntry $outkwdb ATM1_1 $atm($amp,1) N "CCD to amp transformation"
	AddEntry $outkwdb ATM2_2 $atm($amp,2) N "CCD to amp transformation"
	AddEntry $outkwdb ATV1   $atv($amp,1) N "CCD to amp transformation"
	AddEntry $outkwdb ATV2   $atv($amp,2) N "CCD to amp transformation"
	AddEntry $outkwdb LTM1_1 $ltmbin(1)   N "CCD to image transformation"
	AddEntry $outkwdb LTM2_2 $ltmbin(2)   N "CCD to image transformation"
	AddEntry $outkwdb LTV1   $ltvbin(1)   N "CCD to image transformation"
	AddEntry $outkwdb LTV2   $ltvbin(2)   N "CCD to image transformation"
	AddEntry $outkwdb DTM1_1 $dtm($ccd,1) N "CCD to detector transformation"
	AddEntry $outkwdb DTM2_2 $dtm($ccd,2) N "CCD to detector transformation"
	AddEntry $outkwdb DTV1   $dtv($ccd,1) N "CCD to detector transformation"
	AddEntry $outkwdb DTV2   $dtv($ccd,2) N "CCD to detector transformation"
    } err]} {
	dprintf 1 $imageid "can't set geometry keywords for `$outkwdb'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetWcs -- Set the world coordinate system.

proc SetWcs {ext ccd} {
    global imageid telid filter filtersn
    global wcsfilter WCSASTRM CTYPE1 CTYPE2
    global CRVAL1 CRVAL2 CRPIX1 CRPIX2 CD1_1 CD2_2 CD1_2 CD2_1
    global WAT01
    global WAT11 WAT12 WAT13 WAT14 WAT15 WAT16 WAT17
    global WAT21 WAT22 WAT23 WAT24 WAT25 WAT26 WAT27

    set tel ""
    set filt ""
    if {[info exists telid]} {set tel "$telid,"}
    if {[info exists filtersn]} {
	set filt $filtersn,
	if {[info exists wcsfilter($filtersn)]} {
	    set filt "$wcsfilter($filtersn),"
	} elseif {[info exists $filter]} {
	    if {[info exists wcsfilter($filter)]} {
		set filt "$wcsfilter($filter),"
	    }
	}
    }

    set key $tel$filt$ccd
    if {![info exists WCSASTRM($key)]} {set key $filt$ccd}
    if {![info exists WCSASTRM($key)]} {set key $tel$ccd}
    if {![info exists WCSASTRM($key)]} {set key $ccd}

    if {![info exists WCSASTRM($key)]} {
	dprintf 1 $imageid "can't set WCS ($key) for `$ext'\n"
	return
    }

    if {$WCSASTRM($key) == "None available"} {
	set crval1 $CRVAL1($key)
	set crval2 $CRVAL2($key)

    } else {
	if {[catch {GetValue $ext TELRA}] || [catch {GetValue $ext TELDEC}]} {
	    if {[catch {GetValue $ext RA}] || [catch {GetValue $ext DEC}]} {
		dprintf 1 $imageid "can't set WCS ($key) for `$ext'\n"
		return
	    }
	}

	# Set the reference point to the RA/DEC keyword values.
	set deg 0
	set min 0
	set sec 0
	scan [GetValue $ext RA] "%d:%d:%f" deg min sec
	set crval1 [expr 15*($deg+$min/60.+$sec/3600.)]

	set deg 0
	set min 0
	set sec 0
	set sgn [regsub -- "-" [GetValue $ext DEC] "" dec]
	scan $dec "%d:%d:%f" deg min sec
	if {$sgn} {
	    set crval2 [expr -($deg+$min/60.+$sec/3600.)]
	} else {
	    set crval2 [expr ($deg+$min/60.+$sec/3600.)]
	}

	# Precess.
	set equinox [GetValue $ext RADECEQ]
	set wcsequin 2000.
	set result [precess $crval1 $crval2 $equinox $wcsequin]
	set crval1 [lindex $result 0]
	set crval2 [lindex $result 1]
    }

    # Adjust for binning and ROI.
    set ltv1   [GetValue $ext LTV1]
    set ltv2   [GetValue $ext LTV2]
    set ltm1_1 [GetValue $ext LTM1_1]
    set ltm2_2 [GetValue $ext LTM2_2]
    set crpix1 [expr ($CRPIX1($key)*$ltm1_1+$ltv1)]
    set crpix2 [expr ($CRPIX2($key)*$ltm2_2+$ltv2)]
    set cd1_1  [expr ($CD1_1($key)/$ltm1_1)]
    set cd2_1  [expr ($CD2_1($key)/$ltm1_1)]
    set cd1_2  [expr ($CD1_2($key)/$ltm2_2)]
    set cd2_2  [expr ($CD2_2($key)/$ltm2_2)]

    # Set WCS.
    AddEntry $ext ""
    AddEntry $ext WCSASTRM $WCSASTRM($key) S "WCS Source"
    if {[info exists wcsequin]} {
	AddEntry $ext EQUINOX  $wcsequin       N "Equinox of WCS"}
    AddEntry $ext WCSDIM   2               N "WCS dimensionality"
    AddEntry $ext CTYPE1   $CTYPE1($key)   S "Coordinate type"
    AddEntry $ext CTYPE2   $CTYPE2($key)   S "Coordinate type"
    AddEntry $ext CRVAL1   $crval1         N "Coordinate reference value"
    AddEntry $ext CRVAL2   $crval2         N "Coordinate reference value"
    AddEntry $ext CRPIX1   $crpix1         N "Coordinate reference pixel"
    AddEntry $ext CRPIX2   $crpix2         N "Coordinate reference pixel"
    AddEntry $ext CD1_1    $cd1_1          N "Coordinate matrix"
    AddEntry $ext CD2_1    $cd2_1          N "Coordinate matrix"
    AddEntry $ext CD1_2    $cd1_2          N "Coordinate matrix"
    AddEntry $ext CD2_2    $cd2_2          N "Coordinate matrix"

    if {[info exists WAT01($key)]} {
	AddEntry $ext WAT0_001 $WAT01($key)    S "Coordinate system"}
    if {[info exists WAT11($key)]} {
	AddEntry $ext WAT1_001 $WAT11($key)    S}
    if {[info exists WAT12($key)]} {
	AddEntry $ext WAT1_002 $WAT12($key)    S}
    if {[info exists WAT13($key)]} {
	AddEntry $ext WAT1_003 $WAT13($key)    S}
    if {[info exists WAT14($key)]} {
	AddEntry $ext WAT1_004 $WAT14($key)    S}
    if {[info exists WAT15($key)]} {
	AddEntry $ext WAT1_005 $WAT15($key)    S}
    if {[info exists WAT16($key)]} {
	AddEntry $ext WAT1_006 $WAT16($key)    S}
    if {[info exists WAT17($key)]} {
	AddEntry $ext WAT1_007 $WAT17($key)    S}
    if {[info exists WAT21($key)]} {
	AddEntry $ext WAT2_001 $WAT21($key)    S}
    if {[info exists WAT22($key)]} {
	AddEntry $ext WAT2_002 $WAT22($key)    S}
    if {[info exists WAT23($key)]} {
	AddEntry $ext WAT2_003 $WAT23($key)    S}
    if {[info exists WAT24($key)]} {
	AddEntry $ext WAT2_004 $WAT24($key)    S}
    if {[info exists WAT25($key)]} {
	AddEntry $ext WAT2_005 $WAT25($key)    S}
    if {[info exists WAT26($key)]} {
	AddEntry $ext WAT2_006 $WAT26($key)    S}
    if {[info exists WAT27($key)]} {
	AddEntry $ext WAT2_007 $WAT27($key)    S}
}


# PRECESS -- Precess to epoch 2000.  This code is transcribed from the
# IRAF procedure astutil/asttools/precessgj.x.  This is accurate enough
# for our purpose here.  The input and output ra and dec are in degrees.

proc precess {ra1 dec1 equinox1 equinox2} {
    global tcl_precision

    set old_precision $tcl_precision
    set tcl_precision 17

    set t [expr ($equinox2-$equinox1)/100.]
    if {abs($t) < 0.001} {
	set tcl_precision $old_precision
	return [list $ra1 $dec1]
    }

    set pi 3.1415926535897932385
    set radian 57.295779513082320877

    set tau [expr ($equinox1-1850.)/100.]

    set theta [expr $t*((2005.11-0.85*$tau)-$t*(0.43+$t*0.041))/3600.]
    set zeta [expr  $t*((2303.55+1.40*$tau)+$t*(0.30+$t*0.017))/3600.]
    set z [expr $zeta+$t*$t*0.79/3600.]

    set ra [expr $ra1/$radian]
    set dec [expr $dec1/$radian]
    set theta [expr $theta/$radian]
    set zeta [expr $zeta/$radian]
    set z [expr $z/$radian]

    set a [expr $ra+$zeta]
    set dec2 [expr asin(cos($dec)*cos($a)*sin($theta)+sin($dec)*cos($theta))]
    set ap [expr asin(cos($dec)*sin($a)/cos($dec2))]
    set test [expr (cos($dec)*cos($a)*cos($theta)-sin($dec)*sin($theta))/cos($dec2)]

    if {$test < 0.} {set ap [expr $pi-$ap]}
    set ra2 [expr $ap+$z]
    if {$ra2 < 0.} {set ra2 [expr $ra2+2*$pi]}

    set ra2 [expr $ra2*$radian]
    set dec2 [expr $dec2*$radian]

    set tcl_precision $old_precision
    return [list $ra2 $dec2]
}


# SetDets -- Set number of detectors and source the data file.
# This version is very "hardwired".  In future this can be generalized to
# figure things out based on the number of Pan databases and the information
# in them.

proc SetDets {} {
    global imageid nimages kwdbs srcdir datafile
    global extname detname detsize detval
    global ltm ltv atm atv dtm dtv
    global wcsfilter
    global WCSASTRM CTYPE1 CTYPE2
    global CRVAL1 CRVAL2 CRPIX1 CRPIX2 CD1_1 CD2_2 CD1_2 CD2_1
    global WAT01
    global WAT11 WAT12 WAT13 WAT14 WAT15 WAT16 WAT17
    global WAT21 WAT22 WAT23 WAT24 WAT25 WAT26 WAT27

    # Source data files.
    source ${srcdir}/${datafile}

    if {[catch {
	set dhefile [FindValue DHEFILE]
	source ${srcdir}/${dhefile}
    } err]} {
	dprintf 1 $imageid "can't set detector characteristics\n"
	dprintf 2 $imageid "$err\n"
    }
}


# SetMJD - Set MJD keyword from date and time keywords.

proc SetMJD {outkwdb inkwdb date kw type comment} {
    global imageid

    if {[catch {
	set date [GetValue $inkwdb $date]
	set MJD [ComputeMJD $date]
	AddEntry $outkwdb $kw $MJD $type $comment
    } err]} {
	dprintf 1 $imageid "can't compute value for keyword `$kw'\n"
	dprintf 2 $imageid "$err\n"
    }
}


# ComputeMJD -- Compute Modified Julian date from date.

proc ComputeMJD {date} {

    scan $date "%d-%d-%dT%d:%d:%f" y m d hr min sec
    if {$m > 2} {
	incr m
    } else {
	incr m 13
	incr y -1
    }
    set mjd [expr {
	int($y*365.25) + $y/100/4 - $y/100 + int($m*30.6001) + $d - 679004
    }]

    set t [format "%0.8f" [expr ($hr + $min / 60. + $sec / 3600.) / 24.]]
    set t [string trimleft $t 0]

    return $mjd$t
}
