
installconf:


sbin_PROGRAMS = ax25_call netrom_call rose_call tcp_call

dist_man_MANS = ax25_call.8 netrom_call.8 rose_call.8 tcp_call.8

dist_doc_DATA= README.user_call

man_sed = sed -e 's|@@@AX25_SYSCONFDIR@@@|$(AX25_MAN_SC)|g' \
	      -e 's|@@@AX25_LOCALSTATEDIR@@@|$(AX25_MAN_SD)|g'

AX25_MAN_SC = $(sysconfdir)/ax25
AX25_MAN_SD = $(localstatedir)/ax25

ax25_call.8: $(srcdir)/ax25_call.man
	$(man_sed) < $(srcdir)/ax25_call.man > $@

CLEANFILES = ax25_call.8

EXTRA_DIST = ax25_call.man

LDADD = $(AX25_LIB) $(Z_LIB)

ax25_call_SOURCES =	ax25_call.c \
			user_io.c \
			user_io.h

netrom_call_SOURCES =	netrom_call.c \
			user_io.c \
			user_io.h

rose_call_SOURCES =	rose_call.c \
			user_io.c \
			user_io.h

tcp_call_SOURCES =	tcp_call.c \
			user_io.c \
			user_io.h

AM_CPPFLAGS = -D_GNU_SOURCE
