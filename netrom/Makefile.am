
etcfiles = nrbroadcast nrports
etcdir = $(sysconfdir)/ax25

installconf:
	$(mkinstalldirs) $(DESTDIR)$(etcdir)
	@list='$(etcfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p; \
	done


sbin_PROGRAMS = netromd nodesave nrattach nrparms nrsdrv

man_MANS = netrom.4 nrports.5 nrbroadcast.5 netromd.8 nodesave.8 \
	   nrattach.8 nrparms.8 nrsdrv.8

EXTRA_DIST = $(man_MANS) $(etcfiles)

netromd_SOURCES = \
	netromd.c \
	netromd.h \
	netromr.c \
	netromt.c 


