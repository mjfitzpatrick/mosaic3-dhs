#!/bin/csh -f 

# Set the location of the system to help in configuring the machine names.
# The location is simply a 2-character code stored in the '_location' file
# in the top directory, values may be:
#
#	"CR"		=> Tucson Clean Room
#	"FR"		=> Tucson Flex Rig
#	"KP"		=> KPNO 4-meter
#	"CT"		=> CTIO 4-meter
#

if ( $#argv != 6 ) then
  echo "syntax: sim_pan.csh display monsoon_dhs super_host pan_host pan_delay(s) noc_delay(s)"
  exit
endif
set DISPLAY = $argv[1]
setenv MONSOON_DHS "$argv[2]"
set super_host = $argv[3]
set pan_host = $argv[4]
set PAN_DELAY = $argv[5]
set NOC_DELAY = $argv[6]

echo "MONSOON_DHS environment variable = " $MONSOON_DHS
echo "super host = " $super_host
echo "pan_host = " $pan_host
echo "pan delay = " $PAN_DELAY
echo "NOC_DELAY = " $NOC_DELAY

#setenv MONSOON_DHS "127.0.0.1:4150"

set PAN_LOG = /dhs/test/pan.log
set NOC_LOG = /dhs/test/noc.log
set DEBUG_60 = "-debug 0"
set DEBUG_0 = "-debug 0"
set DEBUG_1 = "-debug 0"
set DEBUG_2 = "-debug 0"

set XTERM       = "xterm -ls -sl 4096 -iconic -display $DISPLAY -fn fixed -geometry 80x20+0+400 "

rm -rf $PAN_LOG  $NOC_LOG 

#setenv DISPLAY :1

echo "Starting Pan Client...."
echo ""

(sleep $PAN_DELAY ; $XTERM -e "echo `date` Starting >! $PAN_LOG ; /dhs/test/pan -verbose -sim $DEBUG_0 -host $pan_host -A  |& tee -a $PAN_LOG ; echo `date` Exitting >> $PAN_LOG ; sleep 30 ") &


echo "Starting NOC Clients...."
echo ""

(sleep $NOC_DELAY ; $XTERM -e "echo `date` Starting >! $NOC_LOG ;/dhs/test/nocs -verbose -sim $DEBUG_0 -host $super_host -interactive |& tee -a $NOC_LOG ; echo `date` Exitting >> $NOC_LOG ; sleep 30 ")  &


echo "All done"
echo ""
