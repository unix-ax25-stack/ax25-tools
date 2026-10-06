#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>

#include <config.h>
#include <scm-version.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <sys/socket.h>

#include <netax25/ax25.h>
#include <netrose/rose.h>

#include <netax25/axlib.h>
#include <netax25/axconfig.h>

/*
 * The command words axctl knows and the ioctl each one selects.
 *
 * A table rather than a chain of strcmp(), for one reason beyond looks:
 * the word has to be looked up twice - once to decide whether the
 * arguments are in the short or the long spelling, which is the only way
 * to tell "port:dest src cmd parm" from "port dest src cmd", and once to
 * send the command.
 *
 * The chain this replaces left ax25_ctl.cmd unset for a word it did not
 * know, so a typo did not fail: it sent an ioctl whose command was
 * whatever happened to be in that local.  "maxq" was in the usage text
 * with no branch behind it, so "axctl p d s maxq" has been doing that.
 * An unknown word is now refused, and maxq with it - there is no
 * AX25_MAXQ, and AX25_IPMAXQUEUE has never had a word of its own here.
 */
static const struct axctl_cmd {
	const char *word;
	int cmd;
} axctl_cmds[] = {
	{ "window",	AX25_WINDOW },
	{ "-window",	AX25_WINDOW },
	{ "t1",		AX25_T1 },
	{ "-t1",	AX25_T1 },
	{ "t2",		AX25_T2 },
	{ "-t2",	AX25_T2 },
	{ "t3",		AX25_T3 },
	{ "-t3",	AX25_T3 },
	{ "n2",		AX25_N2 },
	{ "-n2",	AX25_N2 },
	{ "idle",	AX25_IDLE },
	{ "-idle",	AX25_IDLE },
	{ "paclen",	AX25_PACLEN },
	{ "-paclen",	AX25_PACLEN },
	{ "kill",	AX25_KILL },
	{ "-kill",	AX25_KILL },
	{ NULL,		0 }
};

/* The ioctl a command word selects, or -1 for a word axctl does not know. */
static int axctl_cmd_lookup(const char *word)
{
	int i;

	for (i = 0; axctl_cmds[i].word != NULL; i++)
		if (strcmp(word, axctl_cmds[i].word) == 0)
			return axctl_cmds[i].cmd;
	return -1;
}

static void usage(void)
{
	fprintf(stderr, "Usage: axctl [-v] port dest src window|t1|t2|t3|n2|idle|paclen|kill [parm]\n");
	fprintf(stderr, "       axctl [-v] port:dest src window|t1|t2|t3|n2|idle|paclen|kill [parm]\n");
}

/*
 * "port dest" may be written as one argument, "port:dest"; see axkill.c for
 * why the split is at the last colon rather than the first.  The two tools
 * take the same three addresses and answer them the same way, and one of
 * them being stricter than the other about the spelling would be a small
 * trap that only one of them could trip over.
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
	int s, off;

	if (argc == 2 && strncmp(argv[1], "-v", 2) == 0) {
		printf("axctl: %s\n", FULL_VER);
		return 0;
	}

	/*
	 * off is where the command word sits: the fourth argument, or the
	 * third when the port and the destination share one.
	 *
	 * Four arguments can only be the short form, since the long one
	 * needs five.  Five can be either - "port dest src cmd" with no
	 * parameter, or "port:dest src cmd parm" - and there the command
	 * word decides: in the long form it is argv[4], in the short one
	 * argv[3].  The long form is tried first, because it is the
	 * spelling that has always worked and because a long-form port
	 * name may itself carry a colon ("radio0:2"), which would
	 * otherwise be read as a short form with "2" as the destination.
	 */
	if (argc >= 5 && axctl_cmd_lookup(argv[4]) >= 0) {
		off = 4;
	} else if (argc >= 4 && axctl_cmd_lookup(argv[3]) >= 0) {
		off = 3;
	} else {
		usage();
		return 1;
	}

	if (off == 3) {
		if (split_port_dest(argv[1], &portarg, &destarg) != 0) {
			fprintf(stderr, "axctl: expected port:dest, got '%s'\n",
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
		fprintf(stderr, "axctl: no AX.25 port data configured\n");
		return 1;
	}

	addr = ax25_config_get_addr(portarg);
	if (addr == NULL) {
		fprintf(stderr, "axctl: invalid port name - %s\n", portarg);
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
		perror("axctl: socket");
		return 1;
	}

	/* Looked up a second time, and the result is used rather than
	 * matched again: a word that slipped past the spelling check is
	 * refused here instead of leaving the command unset. */
	ax25_ctl.cmd = axctl_cmd_lookup(argv[off]);
	if (ax25_ctl.cmd < 0) {
		fprintf(stderr, "axctl: unknown command '%s'\n", argv[off]);
		close(s);
		return 1;
	}

	if (ax25_ctl.cmd == AX25_KILL) {
		ax25_ctl.arg = 0;
	} else {
		if (argc < off + 2) {
			fprintf(stderr, "axctl: parameter missing\n");
			close(s);
			return 1;
		}
		ax25_ctl.arg = atoi(argv[off + 1]);
	}

	ax25_ctl.digi_count = 0;

	if (ioctl(s, SIOCAX25CTLCON, &ax25_ctl) != 0) {
		perror("axctl: SIOCAX25CTLCON");
		return 1;
	}

	return 0;
}
