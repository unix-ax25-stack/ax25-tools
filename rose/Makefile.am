installconf:

if HAVE_KERNEL_AX25
KERNEL_PROGS = rsattach rsmemsiz rsparms
endif

sbin_PROGRAMS = rsdwnlnk rsuplnk $(KERNEL_PROGS)

LDADD = $(AX25_LIB)

rsmemsiz_LDADD =

sbin_SCRIPTS =
if HAVE_KERNEL_AX25
sbin_SCRIPTS += rsusers.sh
endif


dist_man_MANS = rose.4 rsattach.8 rsparms.8 rsdwnlnk.8 rsuplnk.8

man_sed = sed -e 's|@@@AX25_SYSCONFDIR@@@|$(AX25_MAN_SC)|g' \
	      -e 's|@@@AX25_LOCALSTATEDIR@@@|$(AX25_MAN_SD)|g'

AX25_MAN_SC = $(sysconfdir)/ax25
AX25_MAN_SD = $(localstatedir)/ax25

rsparms.8: $(srcdir)/rsparms.man
	$(man_sed) < $(srcdir)/rsparms.man > $@
rsdwnlnk.8: $(srcdir)/rsdwnlnk.man
	$(man_sed) < $(srcdir)/rsdwnlnk.man > $@
rsuplnk.8: $(srcdir)/rsuplnk.man
	$(man_sed) < $(srcdir)/rsuplnk.man > $@

CLEANFILES = rsparms.8 rsdwnlnk.8 rsuplnk.8

EXTRA_DIST = $(sbin_SCRIPTS) rsparms.man rsdwnlnk.man rsuplnk.man

AM_CPPFLAGS = -D_GNU_SOURCE \
	      -DAX25_SYSCONFDIR=\""$(AX25_SYSCONFDIR)"\" \
	      -DAX25_LOCALSTATEDIR=\""$(AX25_LOCALSTATEDIR)"\"

AX25_SYSCONFDIR=${sysconfdir}/ax25/
AX25_LOCALSTATEDIR=${localstatedir}/ax25/
