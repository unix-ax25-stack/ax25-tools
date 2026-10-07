#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <sys/socket.h>

#include <net/ethernet.h>

#include <netinet/in.h>

#include <sys/file.h>

#include <netax25/ax25.h>
#include <netrose/rose.h>


#include <netax25/axlib.h>
#include <netax25/axconfig.h>
#include <netax25/daemon.h>
#include <netax25/mheard.h>
#include <netax25/axmon.h>

#include <config.h>
#include <scm-version.h>

#include "../pathnames.h"

/* No packet socket outside Linux: the shim in libax25 intercepts
 * socket(PF_PACKET, SOCK_PACKET, ...) and feeds the raw AX.25 frames the
 * AGWPE server reports into it.  The values are the ones Linux uses, so
 * that a frame means the same thing on either kind of system.  */
#ifndef PF_PACKET
#define	PF_PACKET	17
#endif
#ifndef SOCK_PACKET
#define	SOCK_PACKET	10
#endif
#ifndef ETH_P_AX25
#define	ETH_P_AX25	0x0002
#endif

#define	KISS_MASK	0x0F
#define	KISS_DATA	0x00

#define	PID_SEGMENT	0x08
#define	PID_ARP		0xCD
#define	PID_NETROM	0xCF
#define	PID_IP		0xCC
#define	PID_ROSE	0x01
#define	PID_TEXNET	0xC3
#define	PID_FLEXNET	0xCE
#define	PID_TEXT	0xF0
#define	PID_PSATFT	0xBB
#define	PID_PSATPB	0xBD

#define	I		0x00
#define	S		0x01
#define	RR		0x01
#define	RNR		0x05
#define	REJ		0x09
#define	U		0x03
#define	SABM		0x2F
#define	SABME		0x6F
#define	DISC		0x43
#define	DM		0x0F
#define	UA		0x63
#define	FRMR		0x87
#define	UI		0x03

#define	PF		0x10
#define	EPF		0x01

#define	MMASK		7

#define	HDLCAEB		0x01
#define	SSID		0x1E
#define	SSSID_SPARE	0x40
#define	ESSID_SPARE	0x20

#define	ALEN		6
#define	AXLEN		7

struct mheard_list_struct {
	int in_use;
	struct mheard_struct entry;
	long position;
};

static struct mheard_list_struct *mheard_list;
#define	MHEARD_LIST_SIZE 1000
static int    mheard_list_size = MHEARD_LIST_SIZE/10;
static int    logging = FALSE;
static int    foreground = FALSE;	/* -f: no fork, for a supervisor */

static int ftype(char *, int *, int);
static struct mheard_list_struct *findentry(ax25_address *, char *);

static void terminate(int sig)
{
	if (logging) {
		syslog(LOG_INFO, "terminating on SIGTERM\n");
		closelog();
	}

	exit(0);
}

int main(int argc, char **argv)
{
	struct mheard_list_struct *mheard;
	char buffer[1500];
	char *data;
	int size, f, s;
	struct axmon mon;
	char *port = NULL;
	struct sockaddr sa;
	socklen_t asize;
	long position;
	int ctlen, type, end, extseq, flush = FALSE;
	FILE *fp;
	char *p;
	char ports[1024];
	int ports_excl = 0;

	*ports = 0;
	while ((s = getopt(argc, argv, "fFln:p:v")) != -1) {
		switch (s) {
		case 'l':
			logging = TRUE;
			break;
		case 'f':
			foreground = TRUE;
			break;
		case 'F':
			flush = TRUE;
			break;
		case 'n':
			mheard_list_size = atoi(optarg);
			if (mheard_list_size < 10 || mheard_list_size > MHEARD_LIST_SIZE) {
				fprintf(stderr, "mheardd: list size must be between 10 and %d\n", MHEARD_LIST_SIZE);
				return 1;
			}
			break;
		case 'p':
			if (strlen(optarg) > sizeof(ports)-4) {
				fprintf(stderr, "mheardd: too many ports specified.");
				return 1;
			}
			if (*optarg == '!') {
				ports_excl = 1;
				optarg++;
			}
			sprintf(ports, "|%s|", optarg);
			for (p = ports; *p; p++) {
				if (*p == ' ' || *p == ',')
					*p = '|';
			}
			break;
		case 'v':
			printf("mheardd: %s\n", FULL_VER);
			return 0;
		case ':':
			fprintf(stderr, "mheardd: option -n needs an argument\n");
			return 1;
		case '?':
			fprintf(stderr, "Usage: mheardd [-f] [-F] [-l] [-n number] [-p [!]port1[,port2,..]] [-v]\n");
			return 1;
		}
	}

	signal(SIGTERM, terminate);

	if (ax25_config_load_ports() == 0) {
		fprintf(stderr, "mheardd: no AX.25 port data configured\n");
		return 1;
	}

	mheard_list = calloc(mheard_list_size,
			     sizeof(struct mheard_list_struct));
	if (mheard_list == NULL) {
		fprintf(stderr, "mheardd: cannot allocate memory\n");
		return 1;
	}

	if (flush)
		unlink(DATA_MHEARD_FILE);

	/* Load an existing heard list */
	if ((fp = fopen(DATA_MHEARD_FILE, "r")) != NULL) {
		s = 0;
		position = ftell(fp);

		while (fread(buffer, sizeof(struct mheard_struct), 1, fp) == 1 && s < mheard_list_size) {
			memcpy(&mheard_list[s].entry, buffer, sizeof(struct mheard_struct));
			mheard_list[s].in_use   = TRUE;
			mheard_list[s].position = position;
			position = ftell(fp);
			s++;
		}

		fclose(fp);
	} else {
		fp = fopen(DATA_MHEARD_FILE, "w");
		if (fp != NULL)
			fclose(fp);
	}

	/*
	 * -f keeps the process in the foreground, which is what a supervisor
	 * wants: systemd follows the process it started, and a daemon that
	 * disappears behind a fork has to be described to it instead.
	 *
	 * It is not merely convenient.  daemon_start() skips the fork when
	 * getppid() is 1 - it takes that for "started by init" - and under
	 * systemd the parent of a service is pid 1, so no fork happens and a
	 * unit written as Type=forking waits for one until it gives up.  With
	 * -f and Type=simple neither side is guessing.
	 *
	 * The switch is also the one whose meaning changed in this program:
	 * -f used to delete mheard.dat at startup, so a script asking for
	 * the foreground got an empty list with it.  A script that still
	 * asks for the foreground now starts the daemon in front of the old
	 * list instead of emptying it - the empty list is what -F is for,
	 * see mheardd(8).
	 *
	 * The rest of what daemon_start() does is still wanted, and is done
	 * here: out of whatever directory this was started in, and no
	 * inherited umask.  Nothing else: this program starts no children, so
	 * there is no SIGCHLD to ignore, and a foreground process stays in
	 * the session it was started in on purpose.
	 */
	if (foreground) {
		if (chdir("/") < 0) {
			fprintf(stderr, "mheardd: cannot chdir to /: %s\n",
				strerror(errno));
			return 1;
		}
		umask(0);
	} else if (!daemon_start(FALSE)) {
		fprintf(stderr, "mheardd: cannot become a daemon\n");
		return 1;
	}

	/* Use syslog for error messages rather than perror/fprintf */
	if (logging) {
		openlog("mheardd", LOG_PID, LOG_DAEMON);
		syslog(LOG_INFO, "starting");
	}

	/* The monitor is opened here and not before daemon_start(): behind
	 * the libax25 AGWPE shim it is fed by a reader thread, and of a
	 * multithreaded process only the forking thread survives fork() -
	 * a monitor opened earlier would simply go quiet in the daemon.  A
	 * kernel packet socket does not care either way.
	 *
	 * Every source, not one: a host with a kernel AX.25 stack as well
	 * as an ax25netd has frames on both, and a heard list built from
	 * one of them is a heard list with a hole in it that no output
	 * shows.  The port list below is applied to the frames of either,
	 * which is why it is passed as NULL here rather than as a name:
	 * -p takes a set of ports to include or to exclude, not one port
	 * to bind to, and that is a different question from the one
	 * axmon_open() asks.
	 *
	 * AXMON_MASK_NONE, and it is the one place in the suite that asks:
	 * what is recorded below is who talked to whom, with which control
	 * field and which PID, and the information field is not one of
	 * those - it is the majority of the bytes on the wire, on a band
	 * where most of the traffic is exactly that.  So ask ax25netd to
	 * leave it out.  The addresses, the control byte and the PID are
	 * what every line of the heard list is built from, and they stay.
	 *
	 * It only reaches the ax25netd source, and that is the whole of it
	 * on a host where ax25netd carries the ports.  A kernel packet
	 * socket beside it hands over what the kernel assembled; there is
	 * no bit that says otherwise, and cutting a frame at this end
	 * would make one that decodes as a different frame.  See
	 * axmon.h.
	 */
	if (axmon_open_mask(htons(ETH_P_AX25), NULL, AXMON_MASK_NONE,
			    &mon) < 0) {
		if (logging) {
			syslog(LOG_ERR, "cannot watch for AX.25 frames: %m");
			closelog();
		} else
			perror("mheardd: cannot watch for AX.25 frames");
		return 1;
	}

	for (;;) {
		unsigned ready = 0;

		if (axmon_poll(&mon, -1, &ready) < 0) {
			if (errno == EINTR)
				continue;	/* SIGTERM, mostly */
			if (logging) {
				syslog(LOG_ERR, "poll: %m");
				closelog();
			}
			return 1;
		}

		for (f = 0; f < mon.nfd && ready; f++) {
			if ((ready & (1u << f)) == 0)
				continue;
			asize = sizeof(sa);

			/* With a kernel packet socket each read returns one frame;
			 * through the shim they arrive length prefixed.  */
			size = axmon_read(mon.fd[f], mon.framed[f], buffer,
					  sizeof(buffer), &sa, &asize);
			if (size <= 0) {
				/*
				 * The monitor ended, or a read on it failed.
				 * Not a frame either way: a KISS framed packet
				 * has at least a channel byte, so a zero length
				 * can only be the end of the stream.
				 *
				 * Which source matters, and so does whether
				 * another is left.  On a host with a kernel stack
				 * as well as an ax25netd, the kernel's ports are
				 * still on the air and still have a socket behind
				 * them, and a daemon that exits over one of two
				 * sources has stopped hearing half the band
				 * without saying which half.
				 *
				 * A failed read is the same thing.  ECONNRESET
				 * used to be answered by trying again, which is
				 * not a second answer but the same one for ever:
				 * a reset connection stays reset, and the loop
				 * spins on a source that will never read
				 * again.  An ECONNRESET here is a peer that went
				 * away mid frame - the document says so - so
				 * the source is over and is retired like any
				 * other end.  */
				const char *which = axmon_source_name(&mon, f);
				const char *rest;
				int err = (size < 0) ? errno : 0;
				int left;

				if (err == EINTR)
					continue;		/* SIGTERM, mostly */
				ready &= ~(1u << f);
				left = axmon_retire(&mon, f);
				rest = left > 0 ? ", watching the other source" : "";
				if (logging) {
					if (err == 0)
						syslog(LOG_ERR, "the %s closed%s",
							which, rest);
					else
						syslog(LOG_ERR, "the %s failed: %s%s",
							which, strerror(err), rest);
				} else if (err == 0) {
					fprintf(stderr,
						"mheardd: the %s closed%s\n",
						which, rest);
				} else {
					fprintf(stderr,
						"mheardd: the %s failed: %s%s\n",
						which, strerror(err), rest);
				}
				if (left == 0) {
					if (logging)
						closelog();
					return 1;
				}
				continue;
			}

			port = ax25_config_get_name(sa.sa_data);
			if (port == NULL) {
				if (logging)
					syslog(LOG_WARNING, "unknown port '%s'\n", sa.sa_data);
				continue;
			}
			if (*ports) {
				char testport[sizeof(sa.sa_data)+2];
				sprintf(testport, "|%s|", sa.sa_data);
				if (ports_excl) {
					if (strstr(ports, testport)) {
						continue;
					}
				} else {
					if (!strstr(ports, testport)) {
						continue;
					}
				}
			}

			data = buffer;

			if ((*data & KISS_MASK) != KISS_DATA)
				continue;

			data++;
			size--;

			if (size < (AXLEN + AXLEN + 1)) {
				if (logging)
					syslog(LOG_WARNING, "packet too short\n");
				continue;
			}

			mheard = findentry((ax25_address *)(data + AXLEN), port);

			if (!ax25_validate(data + 0) || !ax25_validate(data + AXLEN)) {
				if (logging)
					syslog(LOG_WARNING, "invalid callsign on port %s\n", port);
				continue;
			}

			memcpy(&mheard->entry.from_call, data + AXLEN, sizeof(ax25_address));
			memcpy(&mheard->entry.to_call,   data + 0,     sizeof(ax25_address));
			strcpy(mheard->entry.portname,   port);
			mheard->entry.ndigis = 0;

			extseq = ((data[AXLEN + ALEN] & SSSID_SPARE) != SSSID_SPARE);
			end    = (data[AXLEN + ALEN] & HDLCAEB);

			data += (AXLEN + AXLEN);
			size -= (AXLEN + AXLEN);

			while (!end) {
				memcpy(&mheard->entry.digis[mheard->entry.ndigis], data, sizeof(ax25_address));
				mheard->entry.ndigis++;

				end = (data[ALEN] & HDLCAEB);

				data += AXLEN;
				size -= AXLEN;
			}

			if (size == 0) {
				if (logging)
					syslog(LOG_WARNING, "packet too short\n");
				continue;
			}

			ctlen = ftype(data, &type, extseq);

			mheard->entry.count++;

			switch (type) {
			case SABM:
				mheard->entry.type = MHEARD_TYPE_SABM;
				mheard->entry.uframes++;
				break;
			case SABME:
				mheard->entry.type = MHEARD_TYPE_SABME;
				mheard->entry.uframes++;
				break;
			case DISC:
				mheard->entry.type = MHEARD_TYPE_DISC;
				mheard->entry.uframes++;
				break;
			case UA:
				mheard->entry.type = MHEARD_TYPE_UA;
				mheard->entry.uframes++;
				break;
			case DM:
				mheard->entry.type = MHEARD_TYPE_DM;
				mheard->entry.uframes++;
				break;
			case RR:
				mheard->entry.type = MHEARD_TYPE_RR;
				mheard->entry.sframes++;
				break;
			case RNR:
				mheard->entry.type = MHEARD_TYPE_RNR;
				mheard->entry.sframes++;
				break;
			case REJ:
				mheard->entry.type = MHEARD_TYPE_REJ;
				mheard->entry.sframes++;
				break;
			case FRMR:
				mheard->entry.type = MHEARD_TYPE_FRMR;
				mheard->entry.uframes++;
				break;
			case I:
				mheard->entry.type = MHEARD_TYPE_I;
				mheard->entry.iframes++;
				break;
			case UI:
				mheard->entry.type = MHEARD_TYPE_UI;
				mheard->entry.uframes++;
				break;
			default:
				if (logging)
					syslog(LOG_WARNING, "unknown packet type %02X\n", *data);
				mheard->entry.type = MHEARD_TYPE_UNKNOWN;
				break;
			}

			data += ctlen;
			size -= ctlen;

			if (type == I || type == UI) {
				unsigned char pid = *data;

				switch (pid) {
				case PID_TEXT:
					mheard->entry.mode |= MHEARD_MODE_TEXT;
					break;
				case PID_SEGMENT:
					mheard->entry.mode |= MHEARD_MODE_SEGMENT;
					break;
				case PID_ARP:
					mheard->entry.mode |= MHEARD_MODE_ARP;
					break;
				case PID_NETROM:
					mheard->entry.mode |= MHEARD_MODE_NETROM;
					break;
				case PID_IP:
					mheard->entry.mode |= (type == I) ? MHEARD_MODE_IP_VC : MHEARD_MODE_IP_DG;
					break;
				case PID_ROSE:
					mheard->entry.mode |= MHEARD_MODE_ROSE;
					break;
				case PID_TEXNET:
					mheard->entry.mode |= MHEARD_MODE_TEXNET;
					break;
				case PID_FLEXNET:
					mheard->entry.mode |= MHEARD_MODE_FLEXNET;
					break;
				case PID_PSATPB:
					mheard->entry.mode |= MHEARD_MODE_PSATPB;
					break;
				case PID_PSATFT:
					mheard->entry.mode |= MHEARD_MODE_PSATFT;
					break;
				default:
					if (logging)
						syslog(LOG_WARNING, "unknown PID %02X\n", *data);
					mheard->entry.mode |= MHEARD_MODE_UNKNOWN;
					break;
				}
			}

			if (mheard->entry.first_heard == 0)
				time(&mheard->entry.first_heard);

			time(&mheard->entry.last_heard);

			fp = fopen(DATA_MHEARD_FILE, "r+");
			if (fp == NULL) {
				if (logging)
					syslog(LOG_ERR, "cannot open mheard data file\n");
				continue;
			}

			/* ax25netd may be updating the same file at the same time;
			 * the exclusive flock keeps the two writers from tearing
			 * each other's records.  */
			flock(fileno(fp), LOCK_EX);

			if (mheard->position == 0xFFFFFF) {
				fseek(fp, 0L, SEEK_END);
				mheard->position = ftell(fp);
			}

			fseek(fp, mheard->position, SEEK_SET);

			fwrite(&mheard->entry, sizeof(struct mheard_struct), 1, fp);

			fflush(fp);
			flock(fileno(fp), LOCK_UN);

			fclose(fp);
		}
	}
}

static int ftype(char *data, int *type, int extseq)
{
	if (extseq) {
		if ((*data & 0x01) == 0) {	/* An I frame is an I-frame ... */
			*type = I;
			return 2;
		}
		if (*data & 0x02) {
			*type = *data & ~PF;
			return 1;
		} else {
			*type = *data;
			return 2;
		}
	} else {
		if ((*data & 0x01) == 0) {	/* An I frame is an I-frame ... */
			*type = I;
			return 1;
		}
		if (*data & 0x02) {		/* U-frames use all except P/F bit for type */
			*type = *data & ~PF;
			return 1;
		} else {			/* S-frames use low order 4 bits for type */
			*type = *data & 0x0F;
			return 1;
		}
	}
}

static struct mheard_list_struct *findentry(ax25_address *callsign, char *port)
{
	struct mheard_list_struct *oldest = NULL;
	int i;

	for (i = 0; i < mheard_list_size; i++)
		if (mheard_list[i].in_use &&
		    ax25_cmp(&mheard_list[i].entry.from_call, callsign) == 0 &&
		    strcmp(mheard_list[i].entry.portname, port) == 0)
			return mheard_list + i;

	for (i = 0; i < mheard_list_size; i++) {
		if (!mheard_list[i].in_use) {
			mheard_list[i].in_use   = TRUE;
			mheard_list[i].position = 0xFFFFFF;
			return mheard_list + i;
		}
	}

	for (i = 0; i < mheard_list_size; i++) {
		if (mheard_list[i].in_use) {
			if (oldest == NULL) {
				oldest = mheard_list + i;
			} else {
				if (mheard_list[i].entry.last_heard < oldest->entry.last_heard)
					oldest = mheard_list + i;
			}
		}
	}

	memset(&oldest->entry, 0x00, sizeof(struct mheard_struct));

	return oldest;
}
