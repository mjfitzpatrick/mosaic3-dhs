#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME 	  =  dcalib
VERSION   = 1.0
HERE := $(shell /bin/pwd)


#PLATFORM  = MACOSX
#PVM_ROOT  = /usr/local/pvm/pvm3/
#PVM_ARCH  = DARWIN

PLATFORM  = LINUX
PVM_ROOT  = ..//pvm3/
PVM_ARCH  = LINUXI386
#PVM_ARCH  = LINUXX86_64


# secondary dependencies

LIBBASE = lib$(NAME)
STATICLIB = $(HERE)/$(LIBBASE).a
SHAREDLIB = $(HERE)/$(LIBBASE).so.$(VERSION)


# stuff that's precious to keep

.PRECIOUS:	$(STATICLIB) $(SHAREDLIB)
.KEEP_STATE:


# includes, flags and libraries
CC 	= gcc
CINCS   = -I$(HERE) -I$(PVM_ROOT)/include -L$(PVM_ROOT)/lib/$(PVM_ARCH) \
    	  -I../../include  -I/MNSN/soft_dev/inc -I/home/monsoon/inc
CFLAGS  = -g -m32 -Wall $(CINCS) -D$(PLATFORM) -DXLONG=int \
	  -Wall -Wstrict-prototypes -Wmissing-prototypes \
    	  -D__MNSNERR__ -D_MNSN_STD_HDR_ -D_shmUtil_H_ -D__USE_MISC


# list of source and include files

SRCS = 	dcaCloseChannel.c dcaInitChannel.c dcaSimulator.c \
   	dcaConnectionACK.c dcaOpenChannel.c dcaSuper.c \
   	dcaDev.c dcaRecv.c dcamsg.c dcaHost.c dcaSend.c
OBJS = 	dcaCloseChannel.o dcaInitChannel.o dcaSimulator.o \
   	dcaConnectionACK.o dcaOpenChannel.o dcaSuper.o \
   	dcaDev.o dcaRecv.o dcamsg.o dcaHost.o dcaSend.o
INCS = 	dcaDhs.h


# targets

#all: Shared $(SHAREDLIB) Static $(STATICLIB)
all: Static $(STATICLIB)

all_static: Static $(STATICLIB)

all_shared: Shared $(SHAREDLIB)

clean:
	/bin/rm -rf libdcalib.a
	/bin/rm -rf Shared Static *.o .make.state .nse_depinfo UnitTests/*

realclean:
	/bin/rm -rf Shared Static *.o .archive/* *~ UnitTests/*
	/bin/rm -rf .make.state .nse_depinfo  $(SHAREDLIB) $(STATICLIB)

everything:
	make clean
	make all
	make install

help: HELP

install: all 
	cp libdcalib.a ../
	cp dcaDhs.h ../../include


# Unit test programs to be built.

zzdebug: all_static
	$(CC) $(CFLAGS) -o zzdebug zzdebug.c -L. -lmbus -lgpvm3 -lpvm3 -lm -lc



#############################################################################
# leave this stuff alone
#############################################################################

$(STATICLIB): $(SRCS:%.c=Static/%.o)
	/usr/bin/ar rv $@ $?
Static/%.o: %.c $(INCS)
	/usr/bin/gcc $(CINCS) $(CFLAGS) -c $< -o $@
Static:
	/bin/mkdir $@
	chmod 777 $@

$(SHAREDLIB): $(SRCS:%.c=Shared/%.o)
	/usr/bin/ld -shared -o $@ $? -lc -ldl
Shared/%.o: %.c $(INCS)
	/usr/bin/gcc $(CINCS) $(CFLAGS) -fpic -shared -c $< -o $@
Shared:
	/bin/mkdir $@
	chmod 777 $@
