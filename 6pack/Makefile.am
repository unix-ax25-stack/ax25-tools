
installconf:

sbin_PROGRAMS = m6pack

LDADD= $(AX25_LIB)

dist_man_MANS = m6pack.8

INCLUDES = -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	   -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\" 

AX25_SYSCONFDIR=${sysconfdir}/ax25/
AX25_LOCALSTATEDIR=${localstatedir}/ax25/

install-exec-hook:
	(cd $(DESTDIR)$(sbindir))

