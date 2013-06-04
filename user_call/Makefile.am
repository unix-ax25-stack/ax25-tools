
installconf:


sbin_PROGRAMS = ax25_call netrom_call rose_call tcp_call

dist_man_MANS = ax25_call.8 netrom_call.8 rose_call.8 tcp_call.8

dist_doc_DATA= README.user_call

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

