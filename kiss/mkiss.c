/*
 * mkiss.c
 * Fake out AX.25 code into supporting dual port TNCS by routing serial
 * port data to/from two pseudo ttys.
 *
 * Version 1.03
 *
 * N0BEL
 * Kevin Uhlir
 * kevinu@flochart.com
 *
 * 1.01 12/30/95 Ron Curry - Fixed FD_STATE bug where FD_STATE was being used for
 * three state machines causing spurious writes to wrong TNC port. FD_STATE is
 * now used for real serial port, FD0_STATE for first psuedo tty, and FD1_STATE
 * for second psuedo tty. This was an easy fix but a MAJOR bug.
 *
 * 1.02 3/1/96 Jonathan Naylor - Make hardware handshaking default to off.
 * Allowed for true multiplexing of the data from the two pseudo ttys.
 * Numerous formatting changes.
 *
 * 1.03 4/20/96 Tomi Manninen - Rewrote KISS en/decapsulation (much of the
 * code taken from linux/drivers/net/slip.c). Added support for all the 16
 * possible kiss ports and G8BPQ-style checksumming on ttyinterface. Now
 * mkiss also sends a statistic report to stderr if a SIGUSR1 is sent to it.
 *
 * 1.04 25/5/96 Jonathan Naylor - Added check for UUCP style tty lock files.
 * As they are now supported by kissattach as well.
 *
 * 1.05 20/8/96 Jonathan Naylor - Convert to becoming a daemon and added
 * system logging.
 *
 * 1.06 23/11/96 Tomi Manninen - Added simple support for polled kiss.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>

#include <netax25/ttyutils.h>
#include <netax25/daemon.h>

#include <config.h>

#define	SIZE		4096

#define FEND		0300	/* Frame End			(0xC0)	*/
#define FESC		0333	/* Frame Escape			(0xDB)	*/
#define TFEND		0334	/* Transposed Frame End		(0xDC)	*/
#define TFESC		0335	/* Transposed Frame Escape	(0xDD)	*/

#define POLL		0x0E

/*
 * Keep these off the stack.
 */
static unsigned char ibuf[SIZE];	/* buffer for input operations	*/
static unsigned char obuf[SIZE];	/* buffer for kiss_tx()		*/

static int crc_errors		= 0;
static int invalid_ports	= 0;
static int return_polls		= 0;

static char *usage_string	= "usage: mkiss [-p] [-c] [-h] [-l] [-s speed] [-v] ttyinterface pty ...\n";

static int dump_report		= FALSE;

static int logging              = FALSE;
static int crcflag		= FALSE;
static int hwflag		= FALSE;
static int pollflag		= FALSE;

struct iface
{
	char		*name;		/* Interface name (/dev/???)	*/
	int		fd;		/* File descriptor		*/
	int		escaped;	/* FESC received?		*/
	unsigned char	crc;		/* Incoming frame crc		*/
	unsigned char	obuf[SIZE];	/* TX buffer			*/
	unsigned char	*optr;		/* Next byte to transmit	*/
	unsigned int	errors;		/* KISS protocol error count	*/
	unsigned int	nondata;	/* Non-data frames rx count	*/
	unsigned int	rxpackets;	/* RX frames count		*/
	unsigned int	txpackets;	/* TX frames count		*/
	unsigned long	rxbytes;	/* RX bytes count		*/
	unsigned long	txbytes;	/* TX bytes count		*/
};

static int poll(int fd, int ports)
{
	char buffer[3];
	static int port = 0;

	buffer[0] = FEND;
	buffer[1] = POLL | (port << 4);
	buffer[2] = FEND;
	if (write(fd, buffer, 3) == -1) {
		perror("mkiss: poll: write");
		return 1;
	}
	if (++port >= ports)
		port = 0;
	return 0;
}

static int kiss_tx(int fd, int port, unsigned char *s, int len, int usecrc)
{
	unsigned char *ptr = obuf;
	unsigned char c;
	unsigned char crc = 0;
	int first = TRUE;

	/* Non-data frames don't get a checksum byte */
	if ((s[0] & 0x0F) != 0)
		usecrc = FALSE;

	/* Allow for checksum byte */
	if (usecrc)
		len++;

	/*
	 * Send an initial FEND character to flush out any
	 * data that may have accumulated in the receiver
	 * due to line noise.
	 */

	*ptr++ = FEND;

	/*
	 * For each byte in the packet, send the appropriate
	 * character sequence, according to the SLIP protocol.
	 */

	while (len-- > 0) {
		c = *s++;
		if (first) {			/* Control byte */
			c = (c & 0x0F) | (port << 4);
			first = FALSE;
		}
		if (usecrc) {
			if (len == 0)		/* Now past user data...   */
				c = crc;	/* ...time to encode cksum */
			else
				crc ^= c;	/* Adjust checksum */
		}
		switch (c) {
		case FEND:
			*ptr++ = FESC;
			*ptr++ = TFEND;
			break;
		case FESC:
			*ptr++ = FESC;
			*ptr++ = TFESC;
			break;
		default:
			*ptr++ = c;
			break;
		}
	}
	*ptr++ = FEND;
	return write(fd, obuf, ptr - obuf);
}

static int kiss_rx(struct iface *ifp, unsigned char c, int usecrc)
{
	int len;

	switch (c) {
	case FEND:
		len = ifp->optr - ifp->obuf;
		if (len != 0 && ifp->escaped) {	/* protocol error...	*/
			len = 0;		/* ...drop frame	*/
			ifp->errors++;
		}
		if (len != 0 && (ifp->obuf[0] & 0x0F) != 0) {
			/*
			 * Non-data frames don't have checksum.
			 */
			usecrc = 0;
			if ((ifp->obuf[0] & 0x0F) == POLL) {
				len = 0;	/* drop returned polls	*/
				return_polls++;
			} else
				ifp->nondata++;
		}
		if (len != 0 && usecrc) {
			if (ifp->crc != 0) {	/* checksum failed...	*/
				len = 0;	/* ...drop frame	*/
				crc_errors++;
			} else
				len--;		/* delete checksum byte	*/
		}
		if (len != 0) {
			ifp->rxpackets++;
			ifp->rxbytes += len;
		}
		/*
		 * Clean up.
		 */
		ifp->optr = ifp->obuf;
		ifp->crc = 0;
		ifp->escaped = FALSE;
		return len;
	case FESC:
		ifp->escaped = TRUE;
		return 0;
	case TFESC:
		if (ifp->escaped) {
			ifp->escaped = FALSE;
			c = FESC;
		}
		break;
	case TFEND:
		if (ifp->escaped) {
			ifp->escaped = FALSE;
			c = FEND;
		}
		break;
	default:
		if (ifp->escaped) {		/* protocol error...	*/
			ifp->escaped = FALSE;
			ifp->errors++;
		}
		break;
	}
	*ifp->optr++ = c;
	if (usecrc)
		ifp->crc ^= c;
	return 0;
}

static void sigterm_handler(int sig)
{
	if (logging) {
		syslog(LOG_INFO, "terminating on SIGTERM\n");
		closelog();
	}
	
	exit(0);
}

static void sigusr1_handler(int sig)
{
	signal(SIGUSR1, sigusr1_handler);
	dump_report = TRUE;
}

static void report(struct iface *tty, struct iface **pty, int numptys)
{
	int i;
	long t;

	time(&t);
	syslog(LOG_INFO, "version %s.", VERSION);
	syslog(LOG_INFO, "Status report at %s", ctime(&t));
	syslog(LOG_INFO, "Hardware handshaking %sabled.",
	       hwflag  ? "en" : "dis");
	syslog(LOG_INFO, "G8BPQ checksumming %sabled.",
	       crcflag ? "en" : "dis");
	syslog(LOG_INFO, "polling %sabled.",
	       pollflag ? "en" : "dis");
	syslog(LOG_INFO, "ttyinterface is %s (fd=%d)", tty->name, tty->fd);
	for (i = 0; i < numptys; i++)
		syslog(LOG_INFO, "pty%d is %s (fd=%d)", i, pty[i]->name,
			pty[i]->fd);
	syslog(LOG_INFO, "Checksum errors: %d", crc_errors);
	syslog(LOG_INFO, "Invalid ports: %d", invalid_ports);
	syslog(LOG_INFO, "Returned polls: %d", return_polls);
	syslog(LOG_INFO, "Interface   TX frames TX bytes  RX frames RX bytes  Errors");
	syslog(LOG_INFO, "%-11s %-9u %-9lu %-9u %-9lu %u",
	       tty->name,
	       tty->txpackets, tty->txbytes,
	       tty->rxpackets, tty->rxbytes,
	       tty->errors);
	for (i = 0; i < numptys; i++) {
		syslog(LOG_INFO, "%-11s %-9u %-9lu %-9u %-9lu %u",
		       pty[i]->name,
		       pty[i]->txpackets, pty[i]->txbytes,
		       pty[i]->rxpackets, pty[i]->rxbytes,
		       pty[i]->errors);
	}
	return;
}

int main(int argc, char *argv[])
{
	struct iface *pty[16];
	struct iface *tty;
	unsigned char *icp;
	int topfd;
	fd_set readfd;
	struct timeval timeout;
	int retval, numptys, i, size, len;
	int speed	= -1;

	while ((size = getopt(argc, argv, "chlps:v")) != -1) {
		switch (size) {
		case 'c':
			crcflag = TRUE;
			break;
		case 'h':
			hwflag = TRUE;
			break;
		case 'l':
		        logging = TRUE;
		        break;
		case 'p':
		        pollflag = TRUE;
		        break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'v':
			printf("mkiss: %s\n", VERSION);
			return 1;
		case ':':
		case '?':
			fprintf(stderr, usage_string);
			return 1;
		}
	}

	if ((argc - optind) < 2) {
		fprintf(stderr, usage_string);
		return 1;
	}
	if ((numptys = argc - optind - 1) > 16) {
		fprintf(stderr, "mkiss: max 16 pty interfaces allowed.\n");
		return 1;
	}

	/*
	 * Check for lock files before opening any TTYs
	 */
	if (tty_is_locked(argv[optind])) {
		fprintf(stderr, "mkiss: tty %s is locked by another process\n", argv[optind]);
		return 1;
	}
	for (i = 0; i < numptys; i++) {
		if (tty_is_locked(argv[optind + i + 1])) {
			fprintf(stderr, "mkiss: tty %s is locked by another process\n", argv[optind + i + 1]);
			return 1;
		}
	}

	/*
	 * Open and configure the tty interface. Open() is
	 * non-blocking so it won't block regardless of the modem
	 * status lines.
	 */
	if ((tty = calloc(1, sizeof(struct iface))) == NULL) {
		perror("mkiss: malloc");
		return 1;
	}

	if ((tty->fd = open(argv[optind], O_RDWR | O_NDELAY)) == -1) {
		perror("mkiss: open");
		return 1;
	}

	tty->name = argv[optind];
	tty_raw(tty->fd, hwflag);
	if (speed != -1) tty_speed(tty->fd, speed);
	tty->optr = tty->obuf;
	topfd = tty->fd;
	/*
	 * Make it block again...
	 */
	fcntl(tty->fd, F_SETFL, 0);

	/*
	 * Open and configure the pty interfaces
	 */
	for (i = 0; i < numptys; i++) {
		if ((pty[i] = calloc(1, sizeof(struct iface))) == NULL) {
			perror("mkiss: malloc");
			return 1;
		}
		if ((pty[i]->fd = open(argv[optind + i + 1], O_RDWR)) == -1) {
			perror("mkiss: open");
			return 1;
		}
		pty[i]->name = argv[optind + i + 1];
		tty_raw(pty[i]->fd, FALSE);
		pty[i]->optr = pty[i]->obuf;
		topfd = (pty[i]->fd > topfd) ? pty[i]->fd : topfd;
	}

	/*
	 * Now all the ports are open, lock them.
	 */
	tty_lock(argv[optind]);
	for (i = 0; i < numptys; i++)
		tty_lock(argv[optind + i + 1]);

	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, sigusr1_handler);
	signal(SIGTERM, sigterm_handler);

	if (!daemon_start(FALSE)) {
		fprintf(stderr, "mkiss: cannot become a daemon\n");
		return 1;
	}

	if (logging) {
		openlog("mkiss", LOG_PID, LOG_DAEMON);
		syslog(LOG_INFO, "starting");
	}

	/*
	 * Loop until an error occurs on a read.
	 */
	while (TRUE) {
		FD_ZERO(&readfd);
		FD_SET(tty->fd, &readfd);
		for (i = 0; i < numptys; i++)
			FD_SET(pty[i]->fd, &readfd);

		if (pollflag) {
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
		}

		errno = 0;
		retval = select(topfd + 1, &readfd, NULL, NULL, pollflag ? &timeout : NULL);

		if (retval == -1) {
			if (dump_report) {
				if (logging)
					report(tty, pty, numptys);
				dump_report = FALSE;
				continue;
			} else {
				perror("mkiss: select");
				continue;
			}
		}

		/*
		 * Timer expired.
		 */
		if (retval == 0 && pollflag) {
			poll(tty->fd, numptys);
			continue;
		}

		/*
		 * A character has arrived on the ttyinterface.
		 */
		if (FD_ISSET(tty->fd, &readfd)) {
			if ((size = read(tty->fd, ibuf, SIZE)) < 0 && errno != EINTR) {
				if (logging)
					syslog(LOG_ERR, "tty->fd: %m");
				break;
			}
			for (icp = ibuf; size > 0; size--, icp++) {
				if ((len = kiss_rx(tty, *icp, crcflag)) != 0) {
					if ((i = (*tty->obuf & 0xF0) >> 4) < numptys) {
						kiss_tx(pty[i]->fd, 0, tty->obuf, len, FALSE);
						pty[i]->txpackets++;
						pty[i]->txbytes += len;
					} else
						invalid_ports++;
					if (pollflag)
						poll(tty->fd, numptys);
				}
			}
		}

		for (i = 0; i < numptys; i++) {
			/*
			 * A character has arrived on pty[i].
			 */
			if (FD_ISSET(pty[i]->fd, &readfd)) {
				if ((size = read(pty[i]->fd, ibuf, SIZE)) < 0 && errno != EINTR) {
					if (logging)
						syslog(LOG_ERR, "pty[%d]->fd: %m\n", i);
					goto end;
				}
				for (icp = ibuf; size > 0; size--, icp++) {
					if ((len = kiss_rx(pty[i], *icp, FALSE)) != 0) {
						kiss_tx(tty->fd, i, pty[i]->obuf, len, crcflag);
						tty->txpackets++;
						tty->txbytes += len;
					}
				}
			}
		}
	}

      end:
	if (logging)
		closelog();

	close(tty->fd);
	free(tty);

	for (i = 0; i < numptys; i++) {
		close(pty[i]->fd);
		free(pty[i]);
	}

	return 1;
}
