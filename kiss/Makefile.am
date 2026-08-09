
installconf:

if HAVE_KERNEL_AX25
KERNEL_PROGS = kissattach kissparms net2kiss
install-exec-hook:
	(cd $(DESTDIR)$(sbindir) ; ln -sf kissattach spattach)
endif

sbin_PROGRAMS = kissnetd mkiss $(KERNEL_PROGS)

LDADD= $(AX25_LIB)
net2kiss_LDADD = $(UTIL_LIB)

dist_man_MANS = kissattach.8 spattach.8 kissnetd.8 kissparms.8 mkiss.8 \
		net2kiss.8

AM_CPPFLAGS = -D_GNU_SOURCE \
	      -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	      -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\"

AX25_SYSCONFDIR=${sysconfdir}/ax25/
AX25_LOCALSTATEDIR=${localstatedir}/ax25/
