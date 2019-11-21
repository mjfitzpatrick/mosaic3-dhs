#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME     =  smcmgr
VERSION  = 1.0
PLATFORM = LINUX
HERE    := $(shell /bin/pwd)


#PLATFORM    = MACOSX
#PVM_ROOT    = /usr/local/pvm/pvm3/
#PVM_ARCH    = DARWIN

PLATFORM    = LINUX
PVM_ROOT    = ../lib/pvm3/
PVM_ARCH    = LINUXI386
#PVM_ARCH    = LINUXX86_64


# stuff that's precious to keep

.PRECIOUS:
.KEEP_STATE:


# includes, flags and libraries
CC      = gcc
CINCS   = -I$(HERE) -I../include -I../dev/src
CFLAGS  = -g -Wall -D$(PLATFORM) $(CINCS) -DXLONG=int -m32

LFLAGS  = -L../lib -L$(PVM_ROOT)/lib/$(PVM_ARCH) -L../x11iraf/lib/
LIBS    = -lsmCache -lmbus -lcdl -lpvm3 -lgpvm3 -lm -lpthread


# list of source and include files

SRCS = smcmgr.c mbusMsgHandler.c rotate.c smcProcess.c smcRTD.c smcRectify.c
OBJS = smcmgr.o mbusMsgHandler.o rotate.o smcProcess.o smcRTD.o smcRectify.o
INCS = smcmgr.h

DEV_SRCS  = rectify.c rtd.c
DEV_OBJS  = rectify.o rtd.o


# targets

all: smcmgr

#smcmgr: smcmgr.c
#	$(CC) $(CFLAGS) -o smcmgr smcmgr.c -L../lib -lsmCache -lmbus -lm -lc

clean:
	/bin/rm -rf smcmgr *.o .make.state .nse_depinfo

everything:
	make clean
	make all
	make install

help: HELP

install: all 
	cp smcmgr ../bin

smcmgr: $(OBJS) $(INCS) $(DEV_OBJS)
	cc -o smcmgr -w $(CFLAGS) $(CINCS) $(OBJS) $(DEV_OBJS) $(LFLAGS) $(LIBS)
	cp smcmgr ../bin

%.o: %.c $(INCS)
	gcc $(CINCS) $(CFLAGS) -c $< -o $@

