
etcfiles = rsports
etcdir = $(sysconfdir)/ax25

installconf:
	$(mkinstalldirs) $(DESTDIR)$(etcdir)
	@list='$(etcfiles)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(etcdir)/$$p; \
	done

sbin_PROGRAMS = rsattach rsdwnlnk rsmemsiz rsparms rsuplnk 

LDADD = $(AX25_LIB)

rsmemsiz_LDADD = 

sbin_SCRIPTS = rsusers.sh


man_MANS = rose.4 rsports.5 rsattach.8 rsparms.8 rsdwnlnk.8 rsuplnk.8

EXTRA_DIST = $(man_MANS) $(etcfiles) $(sbin_SCRIPTS)
