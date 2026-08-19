
installconf:

if HAVE_KERNEL_AX25
KERNEL_PROGS = kissattach kissparms
install-exec-hook:
	(cd $(DESTDIR)$(sbindir) ; ln -sf kissattach spattach)
endif

# net2kiss turns what it hears on a port into a KISS stream on a pty.  It
# needs no kernel stack of its own: with one it reads the packet socket,
# without one libax25 hands it the AGWPE monitor instead - which is why it
# must link libax25 (before libc, so the interposed calls bind there).
sbin_PROGRAMS = kissnetd mkiss net2kiss $(KERNEL_PROGS)

LDADD= $(AX25_LIB)
net2kiss_LDADD = $(AX25_LIB) $(UTIL_LIB)

dist_man_MANS = kissattach.8 spattach.8 kissnetd.8 kissparms.8 mkiss.8 \
		net2kiss.8

AM_CPPFLAGS = -D_GNU_SOURCE \
	      -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	      -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\"

AX25_SYSCONFDIR=${sysconfdir}/ax25/
AX25_LOCALSTATEDIR=${localstatedir}/ax25/
