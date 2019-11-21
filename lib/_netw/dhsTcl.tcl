load /usr/local/lib/libfitstcl.so
load $env(MONSOON_LIB)/libdhsTcl.so
load $env(MONSOON_LIB)/libmsdTcl.so

proc dhsTest { } {

 # define some data
 set expID [msd::msd]
 set obsetID "Observation Set #1"
 set nelms 100
 set nlines 5
 set nitems 4
 set arry [list {0 1 2 3 4 5 6 7 8 9} {0 1 2 3 4 5 6 7 8 9} {0 1 2 3 4 5 6 7 8 9} {0 1 2 3 4 5 6 7 8 9} {0 1 2 3 4 5 6 7 8 9} {9 8 7 6 5 4 3 2 1 0} {9 8 7 6 5 4 3 2 1 0} {9 8 7 6 5 4 3 2 1 0} {9 8 7 6 5 4 3 2 1 0} {9 8 7 6 5 4 3 2 1 0} ]
 set fits [list TELESCOP "KPNO Mayall 4m" "Kitt Peak National Observatory" ASTRONOM "Joe Bloggs" "MONSOON Test System Username" INSTRUME "MONSOON+NEWFIRM" "MONSOON Image Acquisition" DETECTOR "aladdin_III" "aladdin_III 4096x4096" ]
 set avp [list  intTime 1.09365 "Integration time in milliseconds" biasVoltage -5.0982521 "Bias voltage for array" dataDir /MNSN/soft_dev/data/ "Current data directory" fileName 2453036.123645.fits "Current output filename" creg 0x02000000 "Control register in HEX representation" ]

 # show version
 dhs::version

 # SysOpen (0xFACE=OCS, 0xCAFE=MSL)
 set sID [dhs::SysOpen 0xFACE]
 puts "dhs::SysOpen ... (sID=$sID) [expr {$sID >= 0 ? "OK" : "FAILED"}]"

 # OpenConnect (0xABCD=PAN 0xFEED=WHO)
 set cID [dhs::OpenConnect 0xFEED {10 10 0 0 8 1 1 1 1 1 1}]
 puts "dhs::OpenConnect ... (cID=$cID) [expr {$cID >= 0 ? "OK" : "FAILED"}]"

 # OpenExp
 set eID [dhs::OpenExp $cID {10 10 0 0 8 1 1 1 1 1 1} $expID $obsetID]
 puts "dhs::OpenExp ... (eID=$eID) [expr {$eID >= 0 ? "OK" : "FAILED"}]"

 # IOctl (0x1000=debug)
 set stat [dhs::IOctl $eID 0x1000 $expID $obsetID]
 puts "dhs::IOctl ... (eID=$eID) [expr {$stat == 0 ? "OK" : "FAILED"}]"

 # PixelData
 set stat [dhs::PixelData $eID $arry $nelms {10 10 0 0 8 1 1 1 1 1 1} $expID $obsetID]
 puts "dhs::PixelData ... (eID=$eID) [expr {$stat == 0 ? "OK" : "FAILED"}]"

 # MetaData (FITS)
 set stat [dhs::MetaData $eID $fits $nitems {1 3 {8 20 46} {1 1 1}} $expID $obsetID]
 puts "dhs::MetaData (FITS) ... (eID=$eID) [expr {$stat == 0 ? "OK" : "FAILED"}]"

 # MetaData (AVP)
 set stat [dhs::MetaData $eID $avp $nlines {2 3 {32 32 64} {1 1 1}} $expID $obsetID]
 puts "dhs::MetaData (AVP) ... (eID=$eID) [expr {$stat == 0 ? "OK" : "FAILED"}]"

 # CloseExp
 set stat [dhs::CloseExp $eID $expID]
 puts "dhs::CloseExp ... (eID=$eID) [expr {$stat == 0 ? "OK" : "FAILED"}]"

 # CloseConnect
 set stat [dhs::CloseConnect $cID]
 puts "dhs::CloseConnect ... (cID=$cID) [expr {$stat == 0 ? "OK" : "FAILED"}]"

 # SysClose
 set stat [dhs::SysClose $sID]
 puts "dhs::SysClose ... (sID=$sID) [expr {$stat == 0 ? "OK" : "FAILED"}]"
}
