#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME        =  test
VERSION     = 1.0
HERE   	   := $(shell /bin/pwd)


# secondary dependencies

LIBBASE     = lib$(NAME)
STATICLIB   = $(HERE)/$(LIBBASE).a
SHAREDLIB   = $(HERE)/$(LIBBASE).so.$(VERSION)
MONSOON_INC = $(MONSOON_HOME)/inc


#PLATFORM    = MACOSX
#PVM_ARCH    = DARWIN

PLATFORM    = LINUX
PVM_ROOT    = ../../lib/pvm3/
PVM_ARCH    = LINUXI386
#PVM_ARCH    = LINUXX86_64


# stuff that's precious to keep

.PRECIOUS:
.KEEP_STATE:


# includes, flags and libraries

CFLAGS = -O2 -g -Wall -D__USE_MISC -DXLONG=int -m32 -DMOSAIC #-DUSE2AMP


# list of source and include files

CMD_SRCS   = dhscmd.c
CMD_OBJS   = dhscmd.o

EXP_SRCS   = exp.c
EXP_OBJS   = exp.o

PAN_SRCS   = pan.c
PAN_OBJS   = pan.o

NOCS_SRCS  = nocs.c
NOCS_OBJS  = nocs.o

LFLAGS = -L../lib -L../../lib -L$(PVM_ROOT)/lib/$(PVM_ARCH) 
LIBS   = -ldhslib -ldcalib -lsmCache -lmbus -lpvm3 -lgpvm3 -lm
CINCS  = -I../include -I../../include -I/MNSN/soft_dev/inc -I/home/monsoon/inc


# targets

all: exp pan nocs dhscmd

clean:
	/bin/rm -rf dhscmd cmd exp nocs pan *.o
	/bin/rm -rf .make.state .nse_depinfo

everything:
	make clean
	make all
	make install

help: HELP

install:
	cp exp pan nocs dhscmd ../../bin


# Unit test programs to be built.

exp: $(EXP_OBJS)
	cc -o exp -w $(CFLAGS) $(CINCS) $(EXP_OBJS) $(LFLAGS) $(LIBS)

pan: $(PAN_OBJS)
	cc -o pan -w $(CFLAGS) $(CINCS) $(PAN_OBJS) $(LFLAGS) $(LIBS)

nocs: $(NOCS_OBJS)
	cc -o nocs -w $(CFLAGS) $(CINCS) $(NOCS_OBJS) $(LFLAGS) $(LIBS)

dhscmd: $(CMD_OBJS)
	cc -o dhscmd -w $(CFLAGS) $(CINCS) $(CMD_OBJS) $(LFLAGS) $(LIBS)


#----------------------------------------------------------------------------

%.o: %.c $(INCS)
	gcc $(CINCS) $(CFLAGS) -c $< -o $@

