#
# $Id: Makefile,v 4.50 1998/04/09 21:11:20 kohl Exp $
#

default: local

install:
	$(PVM_ROOT)/lib/aimk install
	@ ctags -w *.c *.h &

winstall:
	$(PVM_ROOT)/lib/aimk winstall
	@ ctags -w *.c *.h &

local:
	$(PVM_ROOT)/lib/aimk local
	@ ctags -w *.c *.h &

objs:
	$(PVM_ROOT)/lib/aimk objs

tcls:
	$(PVM_ROOT)/lib/aimk tcls

again:
	$(PVM_ROOT)/lib/aimk again

clean:
	$(PVM_ROOT)/lib/aimk clean

veryclean:
	$(PVM_ROOT)/lib/aimk veryclean

