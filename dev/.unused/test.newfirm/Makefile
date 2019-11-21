#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME    =  test
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
PVM_ARCH    = LINUXI386
#PVM_ARCH    = LINUXX86_64


# stuff that's precious to keep

.PRECIOUS:
.KEEP_STATE:


# includes, flags and libraries

#CFLAGS = -O2 -m32 -g
#CFLAGS = -O2 -g -D__USE_MISC -static
CFLAGS = -O2 -g -D__USE_MISC -static -DXLONG=int -m32


# list of source and include files

CMD_SRCS   = dhscmd.c pTime.c
CMD_OBJS   = dhscmd.o pTime.o

EXP_SRCS   = exp.c pTime.c
EXP_OBJS   = exp.o pTime.o

PAN_SRCS   = pan.c pTime.c
PAN_OBJS   = pan.o pTime.o

IPAN_SRCS  = ipan.c pTime.c
IPAN_OBJS  = ipan.o pTime.o

NOCS_SRCS  = nocs.c pTime.c
NOCS_OBJS  = nocs.o pTime.o

LFLAGS = -L../lib -L$(PVM_ROOT)/lib/$(PVM_ARCH) 
LIBS   = -ldhslib -ldcalib -lsmCache -lmbus -lpvm3 -lgpvm3 -lm
CINCS  = -I../include -I/MNSN/soft_dev/inc -I/home/monsoon/inc

# targets

all: exp pan nocs dhscmd

clean:
	/bin/rm -rf dhscmd cmd exp nocs ipan pan *.o
	/bin/rm -rf .make.state .nse_depinfo

everything:
	make clean
	make all
	make install

help: HELP

install:
	cp dhscmd ../bin


# Unit test programs to be built.

exp: $(EXP_OBJS)
	cc -o exp -w $(CFLAGS) $(CINCS) $(EXP_OBJS) $(LFLAGS) $(LIBS)

pan: $(PAN_OBJS)
	cc -o pan -w $(CFLAGS) $(CINCS) $(PAN_OBJS) $(LFLAGS) $(LIBS)

nocs: $(NOCS_OBJS)
	cc -o nocs -w $(CFLAGS) $(CINCS) $(NOCS_OBJS) $(LFLAGS) $(LIBS)

xx: xx.o pTime.o
	cc -o pan -w $(CFLAGS) $(CINCS) xx.o pTime.o $(LFLAGS) $(LIBS)

#----------------------------------------------------------------------------

dhscmd: $(CMD_OBJS)
	cc -o dhscmd -w $(CFLAGS) $(CINCS) $(CMD_OBJS) $(LFLAGS) $(LIBS)
	cp dhscmd ../bin

ipan: $(IPAN_OBJS)
	cc -o ipan -w $(CFLAGS) $(CINCS) $(IPAN_OBJS) $(LFLAGS) $(LIBS)

%.o: %.c $(INCS)
	gcc $(CINCS) $(CFLAGS) -c $< -o $@

