
installconf:

sbin_PROGRAMS = setcrystal sethdlc smmixer smdiag

man_MANS = sethdlc.8 smdiag.8 smmixer.8 baycom.9 hdlcdrv.9 soundmodem.9

EXTRA_DIST = $(man_MANS)

setcrystal_SOURCES = 	\
	setcrystal.c

sethdlc_SOURCES =  	\
	sethdlc.c 	\
	hdrvcomm.c	\
	hdrvcomm.h	\
	usersmdiag.h

smmixer_SOURCES = 	\
	smmixer.c	\
	hdrvcomm.c	\
	hdrvcomm.h	\
	usersmdiag.h

smdiag_SOURCES = 	\
	smdiag.c	\
	hdrvcomm.c	\
	hdrvcomm.h	\
	usersmdiag.h

smdiag_LDADD =		\
	$(X_LIBS)	\
	-lX11
