#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netax25/ax25.h>
#include <netrom/netrom.h>
#include <netrose/rose.h>

#include <netax25/axlib.h>
#include <netax25/axconfig.h>
#include <netax25/nrconfig.h>

void alarm_handler(int sig)
{
}

void err(char *message)
{
	write(STDOUT_FILENO, message, strlen(message));
	exit(1);
}

int main(int argc, char **argv)
{
	char buffer[512], *addr;
	fd_set read_fd;
	int n, s, addrlen = sizeof(struct full_sockaddr_ax25);
	struct full_sockaddr_ax25 nrbind, nrconnect;

	/*
	 * Arguments should be "netrom_call port mycall remaddr"
	 */
	if (argc != 4) {
		strcpy(buffer, "ERROR: invalid number of parameters\r");
		err(buffer);
	}

	if (nr_config_load_ports() == 0) {
		strcpy(buffer, "ERROR: problem with nrports file\r");
		err(buffer);
	}

	/*
	 * Parse the passed values for correctness.
	 */
	nrconnect.fsa_ax25.sax25_family = nrbind.fsa_ax25.sax25_family = AF_NETROM;
	nrbind.fsa_ax25.sax25_ndigis    = 1;
	nrconnect.fsa_ax25.sax25_ndigis = 0;

	if ((addr = nr_config_get_addr(argv[1])) == NULL) {
		sprintf(buffer, "ERROR: invalid NET/ROM port name - %s\r", argv[1]);
		err(buffer);
	}

	if (ax25_aton_entry(addr, nrbind.fsa_ax25.sax25_call.ax25_call) == -1) {
		sprintf(buffer, "ERROR: invalid NET/ROM port callsign - %s\r", argv[1]);
		err(buffer);
	}

	if (ax25_aton_entry(argv[2], nrbind.fsa_digipeater[0].ax25_call) == -1) {
		sprintf(buffer, "ERROR: invalid callsign - %s\r", argv[2]);
		err(buffer);
	}

	if ((addr = strchr(argv[3], ':')) == NULL)
		addr = argv[3];
	else
		addr++;

	if (ax25_aton_entry(addr, nrconnect.fsa_ax25.sax25_call.ax25_call) == -1) {
		sprintf(buffer, "ERROR: invalid callsign - %s\r", argv[3]);
		err(buffer);
	}

	/*
	 * Open the socket into the kernel.
	 */
	if ((s = socket(AF_NETROM, SOCK_SEQPACKET, 0)) < 0) {
		sprintf(buffer, "ERROR: cannot open NET/ROM socket, %s\r", strerror(errno));
		err(buffer);
	}

	/*
	 * Set our AX.25 callsign and NET/ROM callsign accordingly.
	 */
	if (bind(s, (struct sockaddr *)&nrbind, addrlen) != 0) {
		sprintf(buffer, "ERROR: cannot bind NET/ROM socket, %s\r", strerror(errno));
		err(buffer);
	}

	sprintf(buffer, "Connecting to %s ...\r", argv[3]);
	write(STDOUT_FILENO, buffer, strlen(buffer));

	/*
	 * If no response in 30 seconds, go away.
	 */
	alarm(30);

	signal(SIGALRM, alarm_handler);

	/*
	 * Lets try and connect to the far end.
	 */
	if (connect(s, (struct sockaddr *)&nrconnect, addrlen) != 0) {
		switch (errno) {
			case ECONNREFUSED:
				strcpy(buffer, "*** Connection refused - aborting\r");
				break;
			case ENETUNREACH:
				strcpy(buffer, "*** No known route - aborting\r");
				break;
			case EINTR:
				strcpy(buffer, "*** Connection timed out - aborting\r");
				break;
			default:
				sprintf(buffer, "ERROR: cannot connect to NET/ROM node, %s\r", strerror(errno));
				break;
		}

		err(buffer);
	}

	/*
	 * We got there.
	 */
	alarm(0);

	strcpy(buffer, "*** Connected\r");
	write(STDOUT_FILENO, buffer, strlen(buffer));

	/*
	 * Loop until one end of the connection goes away.
	 */
	for (;;) {
		FD_ZERO(&read_fd);
		FD_SET(STDIN_FILENO, &read_fd);
		FD_SET(s, &read_fd);
		
		select(s + 1, &read_fd, NULL, NULL, NULL);

		if (FD_ISSET(s, &read_fd)) {
			if ((n = read(s, buffer, 512)) == -1) {
				strcpy(buffer, "\r*** Disconnected\r");
				err(buffer);
			}
			write(STDOUT_FILENO, buffer, n);
		}

		if (FD_ISSET(STDIN_FILENO, &read_fd)) {
			if ((n = read(STDIN_FILENO, buffer, 512)) == -1) {
				close(s);
				break;
			}
			write(s, buffer, n);
		}
	}

	return 0;
}
