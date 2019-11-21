#///////////////////////////////////////////////////////////////////////////////
#//
#//
#///////////////////////////////////////////////////////////////////////////////

# primary dependencies

NAME =  smCache
VERSION = 1.0
#PLATFORM  = MACOSX
PLATFORM  = LINUX
HERE := $(shell /bin/pwd)


# secondary dependencies

LIBBASE = lib$(NAME)
STATICLIB = $(HERE)/$(LIBBASE).a
SHAREDLIB = $(HERE)/$(LIBBASE).so.$(VERSION)


# stuff that's precious to keep

.PRECIOUS:	$(STATICLIB) $(SHAREDLIB)
.KEEP_STATE:


# includes, flags and libraries
CC = gcc
CINCS  = -I$(HERE)
# CFLAGS = -O2 -Wall -D$(PLATFORM)
# CFLAGS = -O2 -DSMC_DEBUG -D$(PLATFORM)
CFLAGS = -g -Wall -D$(PLATFORM) -m32


# list of source and include files

SRCS = smCache.c smConfig.c smState.c smUtil.c
INCS = smCache.h


# targets

#all: Shared $(SHAREDLIB) Static $(STATICLIB)
all: Static $(STATICLIB) smcop

all_static: Static $(STATICLIB)

all_shared: Shared $(SHAREDLIB)

smcop: all_static smcop.c
	$(CC) -g -w -D$(PLATFORM) -m32 -o smcop smcop.c -L. -lsmCache -lm -lc

clean:
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
	cp smCache.h ../../include
	cp libsmCache.a ../
	cp smcstat smcop ../../bin
	cp smclean ../../
	cp smclean ../../bin
	#mv zzdebug zzcollector zzdca zzrtd ../../bin


# Unit test programs to be built.

zztest: all_static
	./smclean
	$(CC) $(CFLAGS) -o zztest zztest.c -L. -lsmCache -lm -lc

zzdebug: all_static
#	./smclean
	$(CC) $(CFLAGS) -o zzdebug zzdebug.c -L. -lsmCache -lm -lc

zzcollector: all_static
#	./smclean
	$(CC) $(CFLAGS) -o zzcollector zzcollector.c -L. -lsmCache -lm -lc

zzdca: all_static
#	./smclean
	$(CC) $(CFLAGS) -o zzdca zzdca.c -L. -lsmCache -lm -lc

zzrtd: all_static
#	./smclean
	$(CC) $(CFLAGS) -o zzrtd zzrtd.c -I../../include -L../ -lsmCache -lcdl -lm -lc


# leave this stuff alone

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
