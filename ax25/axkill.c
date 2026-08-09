#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <config.h>
#include <scm-version.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <netax25/ax25.h>
#include <netax25/axlib.h>
#include <netax25/axconfig.h>

/*
 * axkill - terminate an established AX.25 connection without touching
 * the application that owns it.
 *
 * The request is carried to ax25netd as an AGWPE 'Q' control frame, so
 * the session is torn down and, for a radio link, disconnected at the
 * upstream.  This is useful to get rid of a hung connection (e.g. to a
 * mailbox) instead of restarting the program.
 */
int main(int argc, char **argv)
{
	struct ax25_ctl_struct ax25_ctl;
	char *addr;
	int s;

	if (argc == 2 && strncmp(argv[1], "-v", 2) == 0) {
		printf("axkill: %s\n", FULL_VER);
		return 0;
	}

	if (argc != 4) {
		fprintf(stderr, "usage: axkill [-v] port dest src\n");
		fprintf(stderr, "       terminate the AX.25 connection from src to dest on port\n");
		return 1;
	}

	if (ax25_config_load_ports() == 0) {
		fprintf(stderr, "axkill: no AX.25 port data configured\n");
		return 1;
	}

	addr = ax25_config_get_addr(argv[1]);
	if (addr == NULL) {
		fprintf(stderr, "axkill: invalid port name - %s\n", argv[1]);
		return 1;
	}

	if (ax25_aton_entry(addr, (char *)&ax25_ctl.port_addr) == -1)
		return 1;
	if (ax25_aton_entry(argv[2], (char *)&ax25_ctl.dest_addr) == -1)
		return 1;
	if (ax25_aton_entry(argv[3], (char *)&ax25_ctl.source_addr) == -1)
		return 1;

	s = socket(AF_AX25, SOCK_SEQPACKET, 0);
	if (s < 0) {
		perror("axkill: socket");
		return 1;
	}

	ax25_ctl.cmd = AX25_KILL;
	ax25_ctl.arg = 0;
	ax25_ctl.digi_count = 0;

	if (ioctl(s, SIOCAX25CTLCON, &ax25_ctl) != 0) {
		perror("axkill: SIOCAX25CTLCON");
		return 1;
	}

	close(s);
	return 0;
}
