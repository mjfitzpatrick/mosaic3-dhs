# Make the Mosaic development software.

CC		= cc -m32
CDEBUGFLAGS	= -g -I../include -I/usr/local/include -I../lib/pvm3/include
MOSOBJ		= mosdca.o dsim.o kwdb.o kwdbtcl.o vmcache.o

# Redhat Linux
LFLAGS		= -L../lib/pvm3/lib/LINUXI386/ -L/usr/local/lib -L../lib/
#LFLAGS		= -L../lib/pvm3/lib/LINUXX86_64/ -L/usr/local/lib -L../lib/
OSLIBS		= -ldl -lm -lmbus -lpvm3 -lgpvm3 -ltcl

INSTALL		= ../
LIBDIR		= $(INSTALL)/lib
INCDIR		= $(INSTALL)/include
BINDIR		= $(INSTALL)/bin/


World:	mosdca zzfeed zzktm zzdsim

all:	mosdca

mosdca: $(MOSOBJ)
	$(CC) $(CDEBUGFLAGS) $(MOSOBJ) $(LFLAGS) $(OSLIBS) -o mosdca 

zzfeed: zzfeed.o
	$(CC) $(CDEBUGFLAGS) zzfeed.o $(LFLAGS) -lpvm3 $(OSLIBS) -o zzfeed

zzdsim: zzdsim.o dsim.o kwdb.o
	$(CC) $(CDEBUGFLAGS) zzdsim.o dsim.o kwdb.o vmcache.o $(LFLAGS) \
	    $(OSLIBS) -o zzdsim

zzktm:  zzktm.o kwdb.o kwdbtcl.o
	$(CC) $(CDEBUGFLAGS) zzktm.o kwdbtcl.o kwdb.o -ltcl \
	$(LFLAGS) $(OSLIBS) -o zzktm

mosdca.o: mosdca.c dso.h dsim.h kwdb.h mosdca.h
	$(CC) -c $(CDEBUGFLAGS) mosdca.c

zzfeed.o: dso.h mosdca.h
	$(CC) -c $(CDEBUGFLAGS) zzfeed.c

zzdsim.o: dso.h dsim.h
	$(CC) -c $(CDEBUGFLAGS) zzdsim.c

zzktm.o: kwdb.h dso.h
	$(CC) -c $(CDEBUGFLAGS) zzktm.c

dsim.o: dsim.c dso.h kwdb.h dsim.h
	$(CC) -c $(CDEBUGFLAGS) dsim.c

kwdb.o: kwdb.c dso.h kwdb.h
	$(CC) -c $(CDEBUGFLAGS) kwdb.c

kwdbtcl.o: kwdbtcl.c dso.h kwdb.h
	$(CC) -c $(CDEBUGFLAGS) kwdbtcl.c

vmcache.o: vmcache.c
	$(CC) -c $(CDEBUGFLAGS) vmcache.c

install: mosdca
	cp mosdca $(BINDIR);

clean:
	/bin/rm -f pix*fits zzfeed zzdsim mosdca zzktm *.o
