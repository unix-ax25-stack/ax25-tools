
etcfiles = ax25.profile ax25d.conf axspawn.conf rxecho.conf
varfiles = mheard.dat

installconf:
	$(mkinstalldirs) $(DESTDIR)$(etcdir)
	@list='$(etcfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p; \
	done
	$(mkinstalldirs) $(DESTDIR)$(vardir)
	@list='$(varfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(vardir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(vardir)/$$p; \
	done


sbin_PROGRAMS = ax25d axctl axparms axspawn beacon bpqparms mheardd rxecho

bin_PROGRAMS = mheard

man_MANS = ax25.4 ax25d.conf.5 axports.5 axspawn.conf.5 rxecho.conf.5 \
	   ax25d.8 axctl.8 axparms.8 axspawn.8 beacon.8 bpqparms.8 \
	   mheard.1 mheardd.8 rxecho.8



EXTRA_DIST = $(man_MANS) $(etcfiles) $(varfiles)

ax25d_SOURCES = ax25d.c
axctl_SOURCES = axctl.c
axparms_SOURCES = axparms.c
axspawn_SOURCES = axspawn.c
beacon_SOURCES = beacon.c
bpqparms_SOURCES = bpqparms.c
mheard_SOURCES = mheard.c
mheardd_SOURCES = mheardd.c
rxecho_SOURCES = rxecho.c
