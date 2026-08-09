
if HAVE_KERNEL_AX25
KERNEL_PROGS = yamcfg
endif

sbin_PROGRAMS = mcs2h $(KERNEL_PROGS)

yamcfg_SOURCES =  yamcfg.c yam.h

mcs2h_SOURCES = mcs2h.c

dist_doc_DATA = README.yamdrv

AM_CPPFLAGS = -D_GNU_SOURCE

installconf:
