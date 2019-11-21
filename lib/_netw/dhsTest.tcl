load /usr/local/lib/libfitstcl.so
load $env(MONSOON_LIB)/libdhsTcl.so
load $env(MONSOON_LIB)/libmsdTcl.so

proc dhsSysOpenTest { } {
 global sID
 set sID [dhs::SysOpen 0xFACE]
 puts "dhs::SysOpen ... (sID=$sID) [expr {$sID >= 0 ? "OK" : "FAILED"}]"
}

proc dhsSysCloseTest { } {
 global sID
 set stat [dhs::SysClose $sID]
 puts "dhs::SysClose ... (sID=$sID) [expr {$stat == 0 ? "OK" : "FAILED"}]"
}

proc dhsOpenConnectTest { } {
 global cID
 set cID [dhs::OpenConnect 0xFEED {4096 4096 0 0 8 1 1 1 1 1 1}]
 puts "dhs::OpenConnect ... (cID=$cID) [expr {$cID >= 0 ? "OK" : "FAILED"}]"
}

proc dhsCloseConnectTest { } {
 global cID
 set stat [dhs::CloseConnect $cID]
 puts "dhs::CloseConnect ... (cID=$cID) [expr {$stat == 0 ? "OK" : "FAILED"}]"
}

proc dhsOpenExpTest { } {
 global cID eID expID obsetID
 set expID [msd::msd]
 set obsetID "Phil Daly"
 set eID [dhs::OpenExp $cID {10 10 0 0 8 1 1 1 1 1 1} $expID $obsetID]
 puts "dhs::OpenExp ... (cID=$cID) [expr {$cID >= 0 ? "OK" : "FAILED"}]"
}

proc dhsCloseExpTest { } {
 global eID expID
 set stat [dhs::CloseConnect $eID $expID]
 puts "dhs::CloseExp ... (cID=$cID) [expr {$stat == 0 ? "OK" : "FAILED"}]"
}
