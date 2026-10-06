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
static void usage(void)
{
	fprintf(stderr, "usage: axkill [-v] port dest src\n");
	fprintf(stderr, "       axkill [-v] port:dest src\n");
	fprintf(stderr, "terminate the AX.25 connection from src to dest on port\n");
}

/*
 * "port dest" may be written as one argument, "port:dest".
 *
 * Split at the last colon, not the first: a callsign has no colon in it,
 * and the one colon a port name may carry is the channel of an axports
 * entry ("radio0:2").  So the last colon is the only one that can be the
 * boundary, and cutting at the first would leave the channel behind in
 * the destination.  ax25tcpd splits at the first colon because there the
 * port is spelled with a "/" for the channel; here the port goes to
 * ax25_config_get_addr() the way it was written in axports, and the ":N"
 * is part of that spelling.
 *
 * Both an empty port and an empty destination are refused: "axkill :DB0AAA"
 * is a shell quoting mistake, not a port.
 */
static int split_port_dest(char *arg, char **portp, char **destp)
{
	char *colon = strrchr(arg, ':');

	if (colon == NULL || colon == arg || colon[1] == '\0')
		return -1;
	*colon = '\0';
	*portp = arg;
	*destp = colon + 1;
	return 0;
}

int main(int argc, char **argv)
{
	struct ax25_ctl_struct ax25_ctl;
	char *addr;
	char *portarg, *destarg, *srcarg;
	int s;

	if (argc == 2 && strncmp(argv[1], "-v", 2) == 0) {
		printf("axkill: %s\n", FULL_VER);
		return 0;
	}

	if (argc != 4 && argc != 3) {
		usage();
		return 1;
	}

	if (argc == 3) {
		if (split_port_dest(argv[1], &portarg, &destarg) != 0) {
			fprintf(stderr,
				"axkill: expected port:dest, got '%s'\n",
				argv[1]);
			return 1;
		}
		srcarg = argv[2];
	} else {
		portarg = argv[1];
		destarg = argv[2];
		srcarg = argv[3];
	}

	if (ax25_config_load_ports() == 0) {
		fprintf(stderr, "axkill: no AX.25 port data configured\n");
		return 1;
	}

	addr = ax25_config_get_addr(portarg);
	if (addr == NULL) {
		fprintf(stderr, "axkill: invalid port name - %s\n", portarg);
		return 1;
	}

	if (ax25_aton_entry(addr, (char *)&ax25_ctl.port_addr) == -1)
		return 1;
	if (ax25_aton_entry(destarg, (char *)&ax25_ctl.dest_addr) == -1)
		return 1;
	if (ax25_aton_entry(srcarg, (char *)&ax25_ctl.source_addr) == -1)
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
