#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME     =  pxf
VERSION  = 1.0
HERE    := $(shell /bin/pwd)


# secondary dependencies

LIBBASE     = lib$(NAME)
STATICLIB   = $(HERE)/$(LIBBASE).a
SHAREDLIB   = $(HERE)/$(LIBBASE).so.$(VERSION)
MONSOON_INC = $(MONSOON_HOME)/inc

#PLATFORM    = MACOSX
#PVM_ROOT    = /usr/local/pvm/pvm3/
#PVM_ARCH    = DARWIN

PLATFORM    = LINUX
PVM_ROOT    = /dhs/lib/pvm3/
PVM_ARCH    = LINUXI386
#PVM_ARCH    = LINUXX86_64


# stuff that's precious to keep

.PRECIOUS:
.KEEP_STATE:


# includes, flags and libraries

CINCS   = -I./ -I../include -I/MNSN/soft_dev/inc -I../lib/cfitsio -I../dev/src \
		-I$(PVM_ROOT)/include -L$(PVM_ROOT)/lib/$(PVM_ARCH) \
		-I/home/monsoon/inc
CFLAGS  = -O2 -m32 -Wall -g $(CINCS) -D$(PLATFORM) -DSPLIT_AMPS
#CFLAGS  = -O2 -g $(CINCS) -D$(PLATFORM) -D__USE_MISC
#CFLAGS  = -O2 -g $(CINCS) -D$(PLATFORM) -D__USE_MISC -DXLONG=int


# list of source and include files

SRCS   = pxf.c pxfAux.c pxfDCA.c pxfFITS.c pxfProcess.c mbusMsgHandler.c
OBJS   = pxf.o pxfAux.o pxfDCA.o pxfFITS.o pxfProcess.o mbusMsgHandler.o
INCS   = smCache.h mbus.h

LFLAGS = -L../lib -L$(PVM_ROOT)/lib/$(PVM_ARCH) -L../smc -L/usr/local/lib
LIBS   = -lcfitsio -lsmCache -lmbus -ldhsCmds -lpvm3 -lgpvm3 -lm



# targets

all:  pxf

clean:
	/bin/rm -rf pxf *.o
	/bin/rm -rf .make.state .nse_depinfo

realclean:
	/bin/rm -rf pxf *.o
	/bin/rm -rf .archive/* *~
	/bin/rm -rf .make.state .nse_depinfo

everything:
	make clean
	make all

help: HELP


# Unit test programs to be built.

pxf: $(OBJS)
	cc -o pxf -O2 -m32 -g -w $(OBJS) $(LFLAGS) $(LIBS)
	cp pxf ../bin

%.o: %.c $(INCS)
	gcc $(CINCS) $(CFLAGS) -c $< -o $@


