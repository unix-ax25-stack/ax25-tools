#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netax25/ax25.h>
#include <netrose/rose.h>
#include <netax25/axlib.h>
#include <netax25/axconfig.h>
#include <netax25/rsconfig.h>

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
	int n, s, addrlen = sizeof(struct sockaddr_rose);
	struct sockaddr_rose rosebind, roseconnect;

	/*
	 * Arguments should be "rose_call port mycall remcall remaddr"
	 */
	if (argc != 5) {
		strcpy(buffer, "ERROR: invalid number of parameters\r");
		err(buffer);
	}

	if (rs_config_load_ports() == 0) {
		strcpy(buffer, "ERROR: problem with rsports file\r");
		err(buffer);
	}

	/*
	 * Parse the passed values for correctness.
	 */
	roseconnect.srose_family = rosebind.srose_family = AF_ROSE;
	roseconnect.srose_ndigis = rosebind.srose_ndigis = 0;

	if ((addr = rs_config_get_addr(argv[1])) == NULL) {
		sprintf(buffer, "ERROR: invalid Rose port name - %s\r", argv[1]);
		err(buffer);
	}

	if (rose_aton(addr, rosebind.srose_addr.rose_addr) == -1) {
		sprintf(buffer, "ERROR: invalid Rose port address - %s\r", argv[1]);
		err(buffer);
	}

	if (ax25_aton_entry(argv[2], rosebind.srose_call.ax25_call) == -1) {
		sprintf(buffer, "ERROR: invalid callsign - %s\r", argv[2]);
		err(buffer);
	}

	if (ax25_aton_entry(argv[3], roseconnect.srose_call.ax25_call) == -1) {
		sprintf(buffer, "ERROR: invalid callsign - %s\r", argv[3]);
		err(buffer);
	}

	if (rose_aton(argv[4], roseconnect.srose_addr.rose_addr) == -1) {
		sprintf(buffer, "ERROR: invalid Rose address - %s\r", argv[4]);
		err(buffer);
	}

	/*
	 * Open the socket into the kernel.
	 */
	if ((s = socket(AF_ROSE, SOCK_SEQPACKET, 0)) < 0) {
		sprintf(buffer, "ERROR: cannot open Rose socket, %s\r", strerror(errno));
		err(buffer);
	}

	/*
	 * Set our AX.25 callsign and Rose address accordingly.
	 */
	if (bind(s, (struct sockaddr *)&rosebind, addrlen) != 0) {
		sprintf(buffer, "ERROR: cannot bind Rose socket, %s\r", strerror(errno));
		err(buffer);
	}

	sprintf(buffer, "Connecting to %s @ %s ...\r", argv[3], argv[4]);
	write(STDOUT_FILENO, buffer, strlen(buffer));

	/*
	 * If no response in 30 seconds, go away.
	 */
	alarm(30);

	signal(SIGALRM, alarm_handler);

	/*
	 * Lets try and connect to the far end.
	 */
	if (connect(s, (struct sockaddr *)&roseconnect, addrlen) != 0) {
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
				sprintf(buffer, "ERROR: cannot connect to Rose address, %s\r", strerror(errno));
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
