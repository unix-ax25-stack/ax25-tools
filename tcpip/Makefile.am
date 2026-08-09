
etcfiles = rip98d.conf ttylinkd.conf

installconf:
	$(mkinstalldirs) $(DESTDIR)$(AX25_SYSCONFDIR)
	@list='$(etcfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(AX25_SYSCONFDIR)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(AX25_SYSCONFDIR)/$$p; \
	done

if HAVE_KERNEL_AX25
KERNEL_PROGS = rip98d
endif

sbin_PROGRAMS = ttylinkd $(KERNEL_PROGS)

dist_man_MANS = rip98d.conf.5 rip98d.8 ttylinkd.conf.5 ttylinkd.8

dist_doc_DATA = ttylinkd.README ttylinkd.INSTALL

EXTRA_DIST = $(etcfiles)

rip98d_SOURCES = 	\
	rip98d.c	\
	rip98r.c	\
	rip98t.c	\
	rip98d.h

rip98d_LDADD = $(AX25_LIB)

AM_CPPFLAGS = -D_GNU_SOURCE \
	      -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	      -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\"

AX25_SYSCONFDIR=${sysconfdir}/ax25/
AX25_LOCALSTATEDIR=${localstatedir}/ax25/
