
etcfiles = rip98.conf ttylinkd.conf
etcdir = $(sysconfdir)/ax25

installconf:
	$(mkinstalldirs) $(DESTDIR)$(etcdir)
	@list='$(etcfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p; \
	done

sbin_PROGRAMS = rip98d ttylinkd

man_MANS = rip98.conf.5 rip98d.8 ttylinkd.conf.5 ttylinkd.8

EXTRA_DIST = $(man_MANS) $(etcfiles)

rip98d_SOURCES = 	\
	rip98d.c	\
	rip98r.c	\
	rip98t.c	\
	rip98d.h

