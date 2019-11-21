load /ndhs/test/libfitstcl.so
load /ndhs/test/libdhsTcl.so

proc dhsSysOpenTest { } {

  # SysOpen (0xFACE=OCS, 0xCAFE=MSL)
  set dhsID [dhs::SysOpen 0xFACE]
  puts "dhs::SysOpen ... (dhsID=$dhsID) [expr {$dhsID >= 0 ? "OK" : "FAILED"}]"

  # SysClose
  #set dhsStat [dhs::SysClose $dhsID]
  #puts "dhs::SysClose ... (dhsID=$dhsID) [expr {$dhsStat == 0 ? "OK" : "FAILED"}]"
}

proc dhsOpenConnectTest { } {

  # OpenConnect (0xABCD=PAN 0xFEED=WHO)
  set dhsID [dhs::OpenConnect 0xFEED {4096 4096 0 0 8 1 1 1 1 1 1}]
  puts "dhs::OpenConnect ... (dhsID=$dhsID) [expr {$dhsID >= 0 ? "OK" : "FAILED"}]"

  # CloseConnect
  set dhsStat [dhs::CloseConnect $dhsID]
  puts "dhs::CloseConnect ... (dhsID=$dhsID) [expr {$dhsStat == 0 ? "OK" : "FAILED"}]"
}
