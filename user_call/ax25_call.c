#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <config.h>

#include <sys/time.h>
#include <sys/types.h>

#include <sys/socket.h>
#ifdef HAVE_NETAX25_AX25_H
#include <netax25/ax25.h>
#else
#include <netax25/kernel_ax25.h>
#endif
#ifdef HAVE_NETROSE_ROSE_H
#include <netrose/rose.h>
#else 
#include <netax25/kernel_rose.h>
#endif

#include <netax25/axlib.h>
#include <netax25/axconfig.h>

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
	struct full_sockaddr_ax25 axbind, axconnect;

	/*
	 * Arguments should be "ax25_call port mycall remcall [digis ...]"
	 */
	if (argc < 4) {
		strcpy(buffer, "ERROR: invalid number of parameters\r");
		err(buffer);
	}

	if (ax25_config_load_ports() == 0) {
		strcpy(buffer, "ERROR: problem with axports file\r");
		err(buffer);
	}

	/*
	 * Parse the passed values for correctness.
	 */
	axconnect.fsa_ax25.sax25_family = axbind.fsa_ax25.sax25_family = AF_AX25;
	axbind.fsa_ax25.sax25_ndigis = 1;

	if ((addr = ax25_config_get_addr(argv[1])) == NULL) {
		sprintf(buffer, "ERROR: invalid AX.25 port name - %s\r", argv[1]);
		err(buffer);
	}

	if (ax25_aton_entry(addr, axbind.fsa_digipeater[0].ax25_call) == -1) {
		sprintf(buffer, "ERROR: invalid AX.25 port callsign - %s\r", argv[1]);
		err(buffer);
	}

	if (ax25_aton_entry(argv[2], axbind.fsa_ax25.sax25_call.ax25_call) == -1) {
		sprintf(buffer, "ERROR: invalid callsign - %s\r", argv[2]);
		err(buffer);
	}

	if (ax25_aton_arglist(argv + 3, &axconnect) == -1) {
		sprintf(buffer, "ERROR: invalid destination callsign or digipeater\r");
		err(buffer);
	}

	/*
	 * Open the socket into the kernel.
	 */
	if ((s = socket(AF_AX25, SOCK_SEQPACKET, 0)) < 0) {
		sprintf(buffer, "ERROR: cannot open AX.25 socket, %s\r", strerror(errno));
		err(buffer);
	}

	/*
	 * Set our AX.25 callsign and AX.25 port callsign accordingly.
	 */
	if (bind(s, (struct sockaddr *)&axbind, addrlen) != 0) {
		sprintf(buffer, "ERROR: cannot bind AX.25 socket, %s\r", strerror(errno));
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
	if (connect(s, (struct sockaddr *)&axconnect, addrlen) != 0) {
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
				sprintf(buffer, "ERROR: cannot connect to AX.25 callsign, %s\r", strerror(errno));
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
