#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME 	  = dhsCmds
VERSION   = 1.0
HERE := $(shell /bin/pwd)


#PLATFORM  = MACOSX
#PVM_ROOT  = /usr/local/pvm/pvm3/
#PVM_ARCH  = DARWIN

PLATFORM  = LINUX
PVM_ROOT  = /dhs/lib/pvm3/
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
CC     = gcc
CINCS  = -I$(HERE) -I../../dev/src -I$(PVM_ROOT)/include -L$(PVM_ROOT)/lib/$(PVM_ARCH)
CFLAGS = -g -Wall -D__USE_MISC $(CINCS) -D$(PLATFORM) -m32 -DSPLIT_AMPS


# list of source and include files

SRCS = 	dhsCmds.c
OBJS = 	dhsCmds.o
INCS = 	dhsCmds.h


# targets

#all: Shared $(SHAREDLIB) Static $(STATICLIB)
all: Static $(STATICLIB)

all_static: Static $(STATICLIB)

all_shared: Shared $(SHAREDLIB)

clean:
	/bin/rm -rf Shared Static *.o *.a
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
	cp libdhsCmds.a ../
	cp dhsCmds.h ../../include/



# Unit test programs to be built.



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
