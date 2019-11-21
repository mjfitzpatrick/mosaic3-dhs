#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME 	  =  dhslib
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
    	  -I../../include -I/MNSN/soft_dev/inc -I/home/monsoon/inc \
	  -I../cfitsio
CFLAGS  = -g -m32 -Wall $(CINCS) -D$(PLATFORM) -D__USE_MISC -DXLONG=int


# list of source and include files

SRCS = 	dhsUtilAux.c dhsUtilCloseConnect.c dhsUtilCloseExp.c  \
	dhsUtilFileOpen.c dhsUtilInit.c dhsUtilIOCtl.c  \
	dhsUtilOpenConnect.c dhsUtilOpenExp.c dhsUtilReadImage.c  \
	dhsUtilSendMetaData.c dhsUtilSendPixelData.c dhsUtilSysClose.c  \
	dhsUtilSysOpen.c dhsUtilUnInit.c  
OBJS = 	dhsUtilAux.o dhsUtilCloseConnect.o dhsUtilCloseExp.o  \
	dhsUtilFileOpen.o dhsUtilInit.o dhsUtilIOCtl.o  \
	dhsUtilOpenConnect.o dhsUtilOpenExp.o dhsUtilReadImage.o  \
	dhsUtilSendMetaData.o dhsUtilSendPixelData.o dhsUtilSysClose.o  \
	dhsUtilSysOpen.o dhsUtilUnInit.o  
INCS = 	dhsImplementationSpecifics.h dhsTcl.h


# targets

#all: Shared $(SHAREDLIB) Static $(STATICLIB)
all: Static $(STATICLIB)

all_static: Static $(STATICLIB)

all_shared: Shared $(SHAREDLIB)

clean:
	/bin/rm -rf libdhslib.a
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
	cp libdhslib.a ../
	cp dhsImplementationSpecifics.h ../../include


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
