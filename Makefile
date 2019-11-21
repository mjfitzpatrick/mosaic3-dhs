#
#  Makefile for the DHS source tree.
#
# ---------------------------------------------------------------------------

# Compiler Flags.

RELEASE		= v1.0

CC 		= gcc
AS 		= gcc -c -x assembler
AR 		= ar clq
CP 		= cp -p

CFLAGS 		= -O2 -Wall -m32
CDEBUGFLAGS 	= -O2 -Wall -m32 -g
BOOTSTRAPCFLAGS = -O2  -pipe -march=i386 -mcpu=i686 -pipe
        

LIBDIRS 	= lib/dcalib lib/dhsCmds lib/dhslib lib/mbus lib/smCache
APPDIRS 	= super collector pxf smcmgr mosdca
SUBDIRS 	= $(LIBDIRS) $(APPDIRS)

all:: update

World::
	@echo "Building the DHS $(RELEASE) software tree"
	@echo "" ; date ; echo ""
	@echo ""
	@echo "static char *build_date = \""`date`"\";"  > build.h
	@if [ -d include ]; then \
	set +x; \
	else \
	if [ -h include ]; then \
	(set -x; rm -f include); \
	fi; \
	(set -x; $(MKDIRHIER) include); \
	fi
	$(MAKE) $(MFLAGS) x11iraf   DHSDIR=$$PWD
	$(MAKE) $(MFLAGS) libs      DHSDIR=$$PWD
	$(MAKE) $(MFLAGS) apps	    DHSDIR=$$PWD
	$(MAKE) $(MFLAGS) install   DHSDIR=$$PWD
	@echo "" ; date ; echo ""
	@echo "World Done."

update::
	@echo "Updating the DHS $(RELEASE) software tree"
	@echo "" ; date ; echo ""
	@echo ""
	@echo "static char *build_date = \""`date`"\";"  > build.h
	$(MAKE) $(MFLAGS) libs      DHSDIR=$$PWD
	$(MAKE) $(MFLAGS) apps      DHSDIR=$$PWD
	$(MAKE) $(MFLAGS) install   DHSDIR=$$PWD
	@echo "" ; date ; echo ""
	@echo "Update Done."


libs::
	$(MAKE) $(MFLAGS) dcalib     DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) dhslib     DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) dhsCmds    DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) mbus       DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) smCache    DHSDIR=$$PWD install

apps::
	$(MAKE) $(MFLAGS) super      DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) collector  DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) smcmgr     DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) pxf        DHSDIR=$$PWD install
	$(MAKE) $(MFLAGS) mosdca     DHSDIR=$$PWD install

super::
	(cd super ; echo "making all in super ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
collector::
	(cd collector ; echo "making all in collector ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
smcmgr::
	(cd smcmgr ; echo "making all in smcmgr ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
pxf::
	(cd pxf ; echo "making all in pxf ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
mosdca::
	(cd mosdca ; echo "making all in mosdca ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);


dcalib::
	(cd lib/dcalib ; echo "making all in lib/dcalib ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
dhslib::
	(cd lib/dhslib ; echo "making all in lib/dhslib ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
dhsCmds::
	(cd lib/dhsCmds ; echo "making all in lib/dhsCmds ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
mbus::
	(cd lib/mbus ; echo "making all in lib/mbus ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);
smCache::
	(cd lib/smCache ; echo "making all in lib/smCache ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' all);

x11iraf::
	(cd x11iraf ; echo "making all in x11iraf ...";\
	 	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' World);


patch::
	(tar -czf dtp.tgz */*.[ch] */*/*.[ch])


archive::
	$(MAKE) $(MFLAGS) pristine
	@(tar -cf - . | gzip > ../dts-$(RELEASE)-src.tar.gz)

pristine::
	$(MAKE) $(MFLAGS) cleandir
	$(MAKE) $(MFLAGS) generic
	$(RM) -rf bin.[a-fh-z]* lib.[a-fh-z]* bin.tar* include *spool* \
		Makefile makefile Makefile.bak */Makefile */Makefile.bak \
		**/Makefile.bak
	$(RM) -f   bin/*
	$(RM) -rf  lib/*



# ----------------------------------------------------------------------
# common rules for all Makefiles - do not edit

.c.i:
	$(RM) $@
	$(CC) -E $(CFLAGS) $(_NOOP_) $*.c > $@

.SUFFIXES: .s

.c.s:
	$(RM) $@
	$(CC) -S $(CFLAGS) $(_NOOP_) $*.c

emptyrule::

cleandir::
	(cd lib/dcalib  ; make clean)
	(cd lib/dhslib  ; make clean)
	(cd lib/dhsCmds ; make clean)
	(cd lib/mbus    ; make clean)
	(cd lib/smCache ; make clean)
	(cd super       ; make clean)
	(cd collector   ; make clean)
	(cd smcmgr      ; make clean)
	(cd pxf         ; make clean)
	(cd mosdca      ; make clean)
	$(RM) *.CKP *.ln *.BAK *.bak *.o core errs ,* *~ *.a .emacs_* tags TAGS make.log MakeOut   "#"*
	$(RM) -rf   bin/*
	$(RM) -rf   lib/libdcalib.a lib/libdhslib.a lib/libdhsCmds.a
	$(RM) -rf   lib/libmbus.a lib/libsmCache.a


Makefile::
	-@if [ -f Makefile ]; then set -x; \
	$(RM) Makefile.bak; $(MV) Makefile Makefile.bak; \
	else exit 0; fi
	$(IMAKE_CMD) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)

tags::
	$(TAGS) -w *.[ch]
	$(TAGS) -xw *.[ch] > TAGS


clean:: cleandir

distclean:: cleandir



# ----------------------------------------------------------------------
# rules for building in SUBDIRS - do not edit

install::
	@for flag in ${MAKEFLAGS} ''; do \
	case "$$flag" in *=*) ;; --*) ;; *[ik]*) set +e;; esac; done; \
	for i in $(SUBDIRS) ;\
	do \
	echo "installing" "in $(CURRENT_DIR)/$$i..."; \
	$(MAKE) -C $$i $(MFLAGS) $(PARALLELMFLAGS) DESTDIR=$(DESTDIR) install; \
	done

install.man::
	@for flag in ${MAKEFLAGS} ''; do \
	case "$$flag" in *=*) ;; --*) ;; *[ik]*) set +e;; esac; done; \
	for i in $(SUBDIRS) ;\
	do \
	echo "installing man pages" "in $(CURRENT_DIR)/$$i..."; \
	$(MAKE) -C $$i $(MFLAGS) $(PARALLELMFLAGS) DESTDIR=$(DESTDIR) install.man; \
	done

Makefiles::
	-@for flag in ${MAKEFLAGS} ''; do \
	case "$$flag" in *=*) ;; --*) ;; *[ik]*) set +e;; esac; done; \
	for flag in ${MAKEFLAGS} ''; do \
	case "$$flag" in *=*) ;; --*) ;; *[n]*) executeit="no";; esac; done; \
	for i in $(SUBDIRS) ;\
	do \
	case "$(CURRENT_DIR)" in \
	.) curdir= ;; \
	*) curdir=$(CURRENT_DIR)/ ;; \
	esac; \
	echo "making Makefiles in $$curdir$$i..."; \
	itmp=`echo $$i | sed -e 's;^\./;;g' -e 's;/\./;/;g'`; \
	curtmp="$(CURRENT_DIR)" \
	toptmp=""; \
	case "$$itmp" in \
	../?*) \
	while echo "$$itmp" | grep '^\.\./' > /dev/null;\
	do \
	toptmp="/`basename $$curtmp`$$toptmp"; \
	curtmp="`dirname $$curtmp`"; \
	itmp="`echo $$itmp | sed 's;\.\./;;'`"; \
	done \
	;; \
	esac; \
	case "$$itmp" in \
	*/?*/?*/?*/?*)	newtop=../../../../..;; \
	*/?*/?*/?*)	newtop=../../../..;; \
	*/?*/?*)	newtop=../../..;; \
	*/?*)		newtop=../..;; \
	*)		newtop=..;; \
	esac; \
	newtop="$$newtop$$toptmp"; \
	case "$(TOP)" in \
	/?*) imaketop=$(TOP) \
	imakeprefix= ;; \
	.) imaketop=$$newtop \
	imakeprefix=$$newtop/ ;; \
	*) imaketop=$$newtop/$(TOP) \
	imakeprefix=$$newtop/ ;; \
	esac; \
	$(RM) $$i/Makefile.bak; \
	if [ -f $$i/Makefile ]; then \
	echo "	$(MV) Makefile Makefile.bak"; \
	if [ "$$executeit" != "no" ]; then \
	$(MV) $$i/Makefile $$i/Makefile.bak; \
	fi; \
	fi; \
	$(MAKE) $(MFLAGS) $(MAKE_OPTS) ONESUBDIR=$$i ONECURDIR=$$curdir IMAKETOP=$$imaketop IMAKEPREFIX=$$imakeprefix $$i/Makefile; \
	if [ -d $$i ] ; then \
	cd $$i; \
	$(MAKE) $(MFLAGS) Makefiles; \
	cd $$newtop; \
	else \
	exit 1; \
	fi; \
	done

includes::
	@for flag in ${MAKEFLAGS} ''; do \
	case "$$flag" in *=*) ;; --*) ;; *[ik]*) set +e;; esac; done; \
	for i in $(SUBDIRS) ;\
	do \
	echo including "in $(CURRENT_DIR)/$$i..."; \
	$(MAKE) -C $$i $(MFLAGS) $(PARALLELMFLAGS)  includes; \
	done

distclean::
	@for flag in ${MAKEFLAGS} ''; do \
	case "$$flag" in *=*) ;; --*) ;; *[ik]*) set +e;; esac; done; \
	for i in $(SUBDIRS) ;\
	do \
	echo "cleaning" "in $(CURRENT_DIR)/$$i..."; \
	$(MAKE) -C $$i $(MFLAGS) $(PARALLELMFLAGS)  distclean; \
	done


# ----------------------------------------------------------------------
# dependencies generated by makedepend

