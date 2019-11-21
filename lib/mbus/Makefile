#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME 	  =  mbus
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
CINCS   = -I$(HERE) -I$(PVM_ROOT)/include -L$(PVM_ROOT)/lib/$(PVM_ARCH)
CFLAGS  = -g -m32 -Wall $(CINCS) -D$(PLATFORM)


# list of source and include files

SRCS = 	mbApp.c mbAck.c mbBcast.c mbConnect.c mbDisconnect.c mbGroup.c \
	mbHandle.c mbHalt.c mbInit.c mbNotify.c mbPing.c mbRecv.c \
	mbSend.c mbSpawn.c mbUtil.c
OBJS = 	mbApp.o mbAck.o mbBcast.c mbConnect.o mbDisconnect.o mbGroup.o \
	mbHandle.o mbHalt.o mbInit.o mbNotify.o mbPing.o mbRecv.o \
	mbSend.o mbSpawn.o mbUtil.o
INCS = 	mbus.h


# targets

#all: Shared $(SHAREDLIB) Static $(STATICLIB)
all: Static $(STATICLIB)

all_static: Static $(STATICLIB)

all_shared: Shared $(SHAREDLIB)

clean:
	/bin/rm -rf libmbus.a
	/bin/rm -rf Shared Static *.o
	/bin/rm -rf .make.state .nse_depinfo
	/bin/rm -rf UnitTests/*

realclean:
	/bin/rm -rf Shared Static *.o
	/bin/rm -rf .archive/* *~
	/bin/rm -rf .make.state .nse_depinfo  $(SHAREDLIB) $(STATICLIB)
	/bin/rm -rf UnitTests/*

everything:
	make clean
	make all
	make install

help: HELP

install: all 
	cp libmbus.a ../
	cp mbus.h ../../include
	cp kpvm ../../bin



# Unit test programs to be built.

zzdebug: all_static
	$(CC) $(CFLAGS) -o zzdebug zzdebug.c -L. -lmbus -lgpvm3 -lpvm3 -lm -lc

zzsup: all_static
	$(CC) $(CFLAGS) -o zzsup zzsup.c -L. -lmbus -lgpvm3 -lpvm3 -lm -lc

zzclient: all_static
	$(CC) $(CFLAGS) -o zzclient zzclient.c -L. -lmbus -lgpvm3 -lpvm3 -lm -lc




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
