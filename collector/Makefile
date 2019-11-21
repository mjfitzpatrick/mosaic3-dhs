#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME    =  collector
VERSION = 1.0
HERE   := $(shell /bin/pwd)


# secondary dependencies

LIBBASE     = lib$(NAME)
STATICLIB   = $(HERE)/$(LIBBASE).a
SHAREDLIB   = $(HERE)/$(LIBBASE).so.$(VERSION)
MONSOON_INC = $(MONSOON_HOME)/inc


#PLATFORM    = MACOSX
#PVM_ROOT    = /usr/local/pvm/pvm3/
#PVM_ARCH    = DARWIN

PLATFORM    = LINUX
PVM_ROOT    = ../lib/pvm3/
PVM_ARCH    = LINUXX86_64
PVM_ARCH    = LINUXI386


# stuff that's precious to keep

.PRECIOUS:
.KEEP_STATE:


# includes, flags and libraries

#CFLAGS = -O2 -m32 -g -w
#CFLAGS = -O2 -g -w -D__USE_MISC
CFLAGS = -O2 -g -m32 -D__USE_MISC -DXLONG=int


# list of source and include files

SRCS   = collector.c colPanHandler.c colSocket.c \
	 consMsgHandler.c mbusMsgHandler.c
OBJS   = collector.o colPanHandler.o colSocket.o \
	 consMsgHandler.o mbusMsgHandler.o

LFLAGS = -L../lib -L$(PVM_ROOT)/lib/$(PVM_ARCH) 
LIBS   = -ldcalib -lsmCache -lmbus -lpvm3 -lgpvm3 -lm
CINCS  = -I../include -I/MNSN/soft_dev/inc -I/home/monsoon/inc -I../dev/src

# targets

all: collector

clean:
	/bin/rm -rf collector *.o
	/bin/rm -rf .make.state .nse_depinfo

realclean:
	/bin/rm -rf *.o
	/bin/rm -rf .archive/* *~ .make.state .nse_depinfo

install:
	cp collector ../bin

everything:
	make clean
	make all

help: HELP


# Unit test programs to be built.

collector: $(OBJS)
	cc -o collector -w $(CFLAGS) $(CINCS) $(OBJS) $(LFLAGS) $(LIBS)
	cp collector ../bin

%.o: %.c $(INCS)
	gcc -Wall $(CINCS) $(CFLAGS) -c $< -o $@

