
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <netdb.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netax25/ax25.h>
#include <netrose/rose.h>

#include <netax25/daemon.h>
#include <netax25/axlib.h>
#include <netax25/ttyutils.h>

#include <config.h>

#include "../pathnames.h"

static char *callsign;
static int  speed   = 0;
static int  mtu     = 0;
static int  logging = FALSE;

static void terminate(int sig)
{
	if (logging) {
		syslog(LOG_INFO, "terminating on SIGTERM\n");
		closelog();
	}

	exit(0);
}

static int readconfig(char *port)
{
	FILE *fp;
	char buffer[90], *s;
	int n = 0;
	
	if ((fp = fopen(CONF_AXPORTS_FILE, "r")) == NULL) {
		fprintf(stderr, "kissattach: cannot open axports file\n");
		return FALSE;
	}

	while (fgets(buffer, 90, fp) != NULL) {
		n++;
	
		if ((s = strchr(buffer, '\n')) != NULL)
			*s = '\0';

		if (strlen(buffer) > 0 && *buffer == '#')
			continue;

		if ((s = strtok(buffer, " \t\r\n")) == NULL) {
			fprintf(stderr, "kissattach: unable to parse line %d of the axports file\n", n);
			return FALSE;
		}
		
		if (strcmp(s, port) != 0)
			continue;
			
		if ((s = strtok(NULL, " \t\r\n")) == NULL) {
			fprintf(stderr, "kissattach: unable to parse line %d of the axports file\n", n);
			return FALSE;
		}

		callsign = strdup(s);

		if ((s = strtok(NULL, " \t\r\n")) == NULL) {
			fprintf(stderr, "kissattach: unable to parse line %d of the axports file\n", n);
			return FALSE;
		}

		speed = atoi(s);

		if ((s = strtok(NULL, " \t\r\n")) == NULL) {
			fprintf(stderr, "kissattach: unable to parse line %d of the axports file\n", n);
			return FALSE;
		}

		if (mtu == 0) {
			if ((mtu = atoi(s)) <= 0) {
				fprintf(stderr, "kissattach: invalid paclen setting\n");
				return FALSE;
			}
		}

		fclose(fp);
		
		return TRUE;
	}
	
	fclose(fp);

	fprintf(stderr, "kissattach: cannot find port %s in axports\n", port);
	
	return FALSE;
}


static int setifcall(int fd, char *name)
{
	char call[7];

	if (ax25_aton_entry(name, call) == -1)
		return FALSE;
	
	if (ioctl(fd, SIOCSIFHWADDR, call) != 0) {
		close(fd);
		perror("kissattach: SIOCSIFHWADDR");
		return FALSE;
	}

	return TRUE;
}


static int startiface(char *dev, struct hostent *hp)
{
	struct ifreq ifr;
	int fd;
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("kissattach: socket");
		return FALSE;
	}

	strcpy(ifr.ifr_name, dev);
	
	if (hp != NULL) {
		ifr.ifr_addr.sa_family = AF_INET;
		
		ifr.ifr_addr.sa_data[0] = 0;
		ifr.ifr_addr.sa_data[1] = 0;
		ifr.ifr_addr.sa_data[2] = hp->h_addr_list[0][0];
		ifr.ifr_addr.sa_data[3] = hp->h_addr_list[0][1];
		ifr.ifr_addr.sa_data[4] = hp->h_addr_list[0][2];
		ifr.ifr_addr.sa_data[5] = hp->h_addr_list[0][3];
		ifr.ifr_addr.sa_data[6] = 0;

		if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
			perror("kissattach: SIOCSIFADDR");
			return FALSE;
		}
	}

	ifr.ifr_mtu = mtu;

	if (ioctl(fd, SIOCSIFMTU, &ifr) < 0) {
		perror("kissattach: SIOCSIFMTU");
		return FALSE;
	}

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("kissattach: SIOCGIFFLAGS");
		return FALSE;
	}

	ifr.ifr_flags &= IFF_NOARP;
	ifr.ifr_flags |= IFF_UP;
	ifr.ifr_flags |= IFF_RUNNING;

	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		perror("kissattach: SIOCSIFFLAGS");
		return FALSE;
	}
	
	close(fd);
	
	return TRUE;
}
	

int main(int argc, char *argv[])
{
	int  fd;
	int  disc = N_AX25;
	char dev[64];
	int  v = 4;
	struct hostent *hp = NULL;

	while ((fd = getopt(argc, argv, "i:lm:v")) != -1) {
		switch (fd) {
			case 'i':
				if ((hp = gethostbyname(optarg)) == NULL) {
					fprintf(stderr, "kissattach: invalid internet name/address - %s\n", optarg);
					return 1;
				}
				break;
			case 'l':
				logging = TRUE;
				break;
			case 'm':
				if ((mtu = atoi(optarg)) <= 0) {
					fprintf(stderr, "kissattach: invalid mtu size - %s\n", optarg);
					return 1;
				}
				break;
			case 'v':
				printf("kissattach: %s\n", VERSION);
				return 0;
			case ':':
			case '?':
				fprintf(stderr, "usage: kissattach [-i inetaddr] [-l] [-m mtu] [-v] ttyinterface port\n");
				return 1;
		}
	}

	if ((argc - optind) != 2) {
		fprintf(stderr, "usage: kissattach [-i inetaddr] [-l] [-m mtu] [-v] ttyinterface port\n");
		return 1;
	}

	if (tty_is_locked(argv[optind])) {
		fprintf(stderr, "kissattach: device %s already in use\n", argv[optind]);
		return 1;
	}

	if (!readconfig(argv[optind + 1]))
		return 1;

	if ((fd = open(argv[optind], O_RDONLY | O_NONBLOCK)) == -1) {
		perror("kissattach: open");
		return 1;
	}

	if (speed != 0 && !tty_speed(fd, speed))
		return 1;
	
	if (ioctl(fd, TIOCSETD, &disc) == -1) {
		perror("kissattach: TIOCSETD");
		return 1;
	}
	
	if (ioctl(fd, SIOCGIFNAME, dev) == -1) {
		perror("kissattach: SIOCGIFNAME");
		return 1;
	}
	
	if (!setifcall(fd, callsign))
		return 1;

	/* Now set the encapsulation */
	if (ioctl(fd, SIOCSIFENCAP, &v) == -1) {
		perror("kissattach: SIOCSIFENCAP");
		return 1;
	}
		
	if (!startiface(dev, hp))
		return 1;		

	printf("AX.25 port %s bound to device %s\n", argv[optind + 1], dev);

	if (logging) {
		openlog("kissattach", LOG_PID, LOG_DAEMON);
		syslog(LOG_INFO, "AX.25 port %s bound to device %s\n", argv[optind + 1], dev);
	}
		
	signal(SIGHUP, SIG_IGN);
	signal(SIGTERM, terminate);

	/*
	 * Become a daemon if we can.
	 */
	if (!daemon_start(FALSE)) {
		fprintf(stderr, "kissattach: cannot become a daemon\n");
		return 1;
	}

	if (!tty_lock(argv[optind]))
		return 1;

	while (1)
		sleep(10000);
		
	/* NOT REACHED */
	return 0;
}
