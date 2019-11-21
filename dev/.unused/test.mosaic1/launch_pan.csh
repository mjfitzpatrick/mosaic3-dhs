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

setenv MONSOON_DHS "127.0.0.1:4150"
setenv USER_DISPLAY 	$DISPLAY
setenv RTD		fast:inet:3200

set SUP_LOG = /dhs/test//sup.log
set PAN_LOG = /dhs/test/pan.log
set DCA_LOG = /dhs/test/dca.log
set NOC_LOG = /dhs/test/noc.log
set COL_LOG1 = /dhs/test/col1.log
set COL_LOG2 = /dhs/test/col2.log
set SMC_LOG = /dhs/test/smc.log
set PXF_LOG = /dhs/test/pxf.log

if ( -e $PAN_LOG ) rm -rf $PAN_LOG

setenv DISPLAY :1
set    XTERM = "xterm -fn fixed"
set    XTERM1 = "xterm -fn fixed -iconic "
#
#set    XTERM1 = "xterm -fn fixed "
if ( 1 ) then

echo "Starting Pan Client...."
echo ""

($XTERM -geometry 80x12+300+720 -e "echo `date` Starting >! $PAN_LOG ; /dhs/test/pan -sim -debug 0 -host nfpan-a -A  |& tee -a $PAN_LOG ; echo `date` Exitting >> $PAN_LOG ; sleep 30 ") &

