SUBDIRS = axgetput

etcfiles = ax25.profile ax25d.conf axspawn.conf rxecho.conf
etcsrcfiles = ax25.profile ax25d.conf.in axspawn.conf rxecho.conf
noinst_DATA = $(etcfiles)
varfiles = mheard.dat

subst_path = sed -e 's|[@]SBINDIR[@]|$(sbindir)|' \
		 -e 's|[@]SYSCONFDIR[@]|$(sysconfdir)|'

ax25d.conf: ax25d.conf.in
	$(subst_path) < $(srcdir)/ax25d.conf.in > ax25d.conf.tmp
	mv ax25d.conf.tmp ax25d.conf

man_sed = sed -e 's|@@@AX25_SYSCONFDIR@@@|$(AX25_MAN_SC)|g' \
	      -e 's|@@@AX25_LOCALSTATEDIR@@@|$(AX25_MAN_SD)|g'

AX25_MAN_SC = $(sysconfdir)/ax25
AX25_MAN_SD = $(localstatedir)/ax25

ax25d.8: $(srcdir)/ax25d.man
	$(man_sed) < $(srcdir)/ax25d.man > $@
axctl.8: $(srcdir)/axctl.man
	$(man_sed) < $(srcdir)/axctl.man > $@
axparms.8: $(srcdir)/axparms.man
	$(man_sed) < $(srcdir)/axparms.man > $@
axspawn.8: $(srcdir)/axspawn.man
	$(man_sed) < $(srcdir)/axspawn.man > $@
axspawn.conf.5: $(srcdir)/axspawn.conf.man
	$(man_sed) < $(srcdir)/axspawn.conf.man > $@
mheard.1: $(srcdir)/mheard.man
	$(man_sed) < $(srcdir)/mheard.man > $@
mheardd.8: $(srcdir)/mheardd.man
	$(man_sed) < $(srcdir)/mheardd.man > $@
rxecho.8: $(srcdir)/rxecho.man
	$(man_sed) < $(srcdir)/rxecho.man > $@
rxecho.conf.5: $(srcdir)/rxecho.conf.man
	$(man_sed) < $(srcdir)/rxecho.conf.man > $@
ax25d.conf.5: $(srcdir)/ax25d.conf.man
	$(man_sed) < $(srcdir)/ax25d.conf.man > $@

installconf: $(etcfiles)
	$(mkinstalldirs) $(DESTDIR)$(AX25_SYSCONFDIR)
	@list='$(etcfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(AX25_SYSCONFDIR)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(AX25_SYSCONFDIR)/$$p; \
	done
	$(mkinstalldirs) $(DESTDIR)$(AX25_LOCALSTATEDIR)/mheard
	@list='$(varfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(AX25_LOCALSTATEDIR)/mheard/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(AX25_LOCALSTATEDIR)/mheard/$$p; \
	done


# These three speak to the kernel stack directly: axparms and rxecho
# through route and interface ioctls, bpqparms through linux/bpqether.h.
# Nothing outside Linux to point them at.
if HAVE_KERNEL_AX25
KERNEL_PROGS = axparms bpqparms rxecho
endif

# The rest reach the stack through libax25, which serves them from the
# kernel or from AGWPE depending on what is there - mheardd included, whose
# packet socket the shim answers with the AGWPE monitor.
sbin_PROGRAMS = axwrapper beacon ax25d axctl axkill axspawn mheardd \
		$(KERNEL_PROGS)

bin_PROGRAMS = mheard

LDADD = $(AX25_LIB)
axspawn_LDADD = $(AX25_LIB) $(UTIL_LIB)

AM_CPPFLAGS = -D_GNU_SOURCE \
	      -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	      -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\"

AX25_SYSCONFDIR=$(sysconfdir)/ax25/
AX25_LOCALSTATEDIR=$(localstatedir)/ax25/

dist_man_MANS = ax25.4 ax25d.conf.5 axspawn.conf.5 rxecho.conf.5 \
	   ax25d.8 axctl.8 axparms.8 axspawn.8 axwrapper.8 beacon.8 \
	   bpqparms.8 mheard.1 mheardd.8 rxecho.8



CLEANFILES = ax25d.conf ax25d.conf.5 axspawn.conf.5 rxecho.conf.5 \
	   ax25d.8 axctl.8 axparms.8 axspawn.8 mheard.1 mheardd.8 rxecho.8
EXTRA_DIST = $(etcsrcfiles) $(varfiles) \
	   ax25d.conf.man axspawn.conf.man rxecho.conf.man \
	   ax25d.man axctl.man axparms.man axspawn.man \
	   mheard.man mheardd.man rxecho.man

ax25d_SOURCES = ax25d.c
axctl_SOURCES = axctl.c
axparms_SOURCES = axparms.c
axkill_SOURCES = axkill.c
axspawn_SOURCES = axspawn.c axspawn.h access.c access.h md5.c md5.h
axwrapper_SOURCES = axwrapper.c
beacon_SOURCES = beacon.c
bpqparms_SOURCES = bpqparms.c
mheard_SOURCES = mheard.c
mheardd_SOURCES = mheardd.c
rxecho_SOURCES = rxecho.c
