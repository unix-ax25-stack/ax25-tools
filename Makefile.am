
installconf:
	@for app in $(SUBDIRS); do $(MAKE) -C $$app installconf; done

SUBDIRS = ax25 hdlcutil kiss 6pack netrom rose tcpip user_call yamdrv dmascc

EXTRA_DIST = pathnames.h ax25-tools.spec

AM_CPPFLAGS = -DAX25_SYSCONFDIR=\""$(sysconfdir)/ax25/"\" \
	      -DAX25_LOCALSTATEDIR=\""$(localstatedir)/ax25/"\"

AX25_SYSCONFDIR=@sysconfdir@/ax25/
AX25_LOCALSTATEDIR=@localstatedir@/ax25/

BUILT_SOURCES = scm-version.h

.PHONY:	scm-version.h
scm-version.h:
	sed -e 's/@RELEASE_VER@/@VERSION@/'				\
	    -e 's/@SCM_VER@/'$$(./setlocalversion)'/'			\
	    scm-version.h.in > scm-version.h.tmp &&			\
	(cmp -s scm-version.h.tmp scm-version.h 2>/dev/zero ||		\
	 mv scm-version.h.tmp scm-version.h)
	rm scm-version.h.tmp
