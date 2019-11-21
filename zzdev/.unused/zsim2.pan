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

setenv USER_DISPLAY 	$DISPLAY
setenv RTD		fast:inet:3200

#-------------------------------------------------------------------------------


set    XTERM    = "xterm -fn fixed"

(sleep 1; $XTERM -geometry 80x12+500+720 -e ~/dhs/test/pan  -debug 0 -host nfpan-b -B ) &
(sleep 2; $XTERM -geometry 80x12+300+720 -e ~/dhs/test/pan  -debug 0 -host nfpan-a -A ) &
(sleep 3; $XTERM -geometry 80x12+100+720 -e ~/dhs/test/nocs -debug 0 -host newfirm -interactive )  &

