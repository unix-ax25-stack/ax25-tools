installconf:

if HAVE_KERNEL_AX25
KERNEL_PROGS = kissattach
endif

# kissparms now supports all backends, always build it
KISSPARMS_PROGS = kissparms

sbin_PROGRAMS = kissnetd mkiss net2kiss $(KERNEL_PROGS) kissparms

LDADD = $(AX25_LIB)
net2kiss_LDADD = $(AX25_LIB) $(UTIL_LIB)
kissparms_SOURCES = kissparms.c kissparms_wampes.c kissparms_agwpe.c
kissparms_LDADD = $(AX25_LIB)

dist_man_MANS = kissattach.8 spattach.8 kissnetd.8 kissparms.8 mkiss.8 \
		net2kiss.8

AM_CPPFLAGS = -D_GNU_SOURCE \
	      -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	      -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\" \
	      -I/Users/thomas/unix-ax25-stack/libax25.git \
	      -I$(top_srcdir) -I$(top_srcdir)/../libax25

AX25_SYSCONFDIR=${sysconfdir}/ax25/
AX25_LOCALSTATEDIR=${localstatedir}/ax25/

dist_man_MANS = kissattach.8 spattach.8 kissnetd.8 kissparms.8 mkiss.8 \
		net2kiss.8

EXTRA_DIST = kissparms_wampes.c kissparms_agwpe.c