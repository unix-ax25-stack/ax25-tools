
installconf:
	@for app in $(SUBDIRS); do $(MAKE) -C $$app installconf; done

if HAVE_KERNEL_AX25
KERNEL_SUBDIRS = hdlcutil dmascc
endif

SUBDIRS = ax25 kiss 6pack netrom rose tcpip user_call yamdrv $(KERNEL_SUBDIRS)

EXTRA_DIST = pathnames.h scm-version.h.in ax25-tools.spec

dist_doc_DATA = README.compile README-hints-for-non-kernel-AX25-hosts.txt \
		README-linux-systemd.txt

AM_CPPFLAGS = -D_GNU_SOURCE \
	      -DAX25_SYSCONFDIR=\""$(sysconfdir)/ax25/"\" \
	      -DAX25_LOCALSTATEDIR=\""$(localstatedir)/ax25/"\"

AX25_SYSCONFDIR=@sysconfdir@/ax25/
AX25_LOCALSTATEDIR=@localstatedir@/ax25/

BUILT_SOURCES = scm-version.h
dist_noinst_SCRIPTS = setlocalversion

.PHONY:	scm-version.h
scm-version.h:
	sed -e 's/@RELEASE_VER@/@VERSION@/'				\
	    -e 's/@SCM_VER@/'$$(./setlocalversion)'/'			\
	    scm-version.h.in > scm-version.h.tmp &&			\
	(cmp -s scm-version.h.tmp scm-version.h 2>/dev/zero ||		\
	 mv scm-version.h.tmp scm-version.h)
	rm -f scm-version.h.tmp
