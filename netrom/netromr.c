#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/ethernet.h>

#include <netax25/ax25.h>
#include <netrom/netrom.h>
#include <netrose/rose.h>
#include <netax25/axlib.h>
#include "../pathnames.h"

#include "netromd.h"

extern int compliant;

static int validmnemonic(char *mnemonic)
{
	if (compliant) {
		if (strspn(mnemonic, "#_&-/ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == strlen(mnemonic))
			return TRUE;
	} else {
		if (strspn(mnemonic, "#_&-/abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == strlen(mnemonic))
			return TRUE;
	}

	return FALSE;
}

void receive_nodes(unsigned char *buffer, int length, ax25_address *neighbour, int index)
{
	struct nr_route_struct nr_node;
	char neigh_buffer[90], *portcall, *p;
	FILE *fp;
	int s;
	int quality, obs_count, qual, lock;
	char *addr, *callsign, *device;

	nr_node.type   = NETROM_NODE;
	/*nr_node.ndigis = 0;*/

	sprintf(neigh_buffer, "%s/obsolescence_count_initialiser", PROC_NR_SYSCTL_DIR);

	if ((fp = fopen(neigh_buffer, "r")) == NULL) {
		if (logging)
			syslog(LOG_ERR, "netromr: cannot open %s\n", neigh_buffer);
		return;
	}

	fgets(neigh_buffer, 90, fp);
	
	obs_count = atoi(neigh_buffer);

	fclose(fp);

	if ((s = socket(AF_NETROM, SOCK_SEQPACKET, 0)) < 0) {
		if (logging)
			syslog(LOG_ERR, "netromr: socket: %m");
		return;
	}

	if ((fp = fopen(PROC_NR_NEIGH_FILE, "r")) == NULL) {
		if (logging)
			syslog(LOG_ERR, "netromr: cannot open %s\n", PROC_NR_NEIGH_FILE);
		close(s);
		return;
	}

	fgets(neigh_buffer, 90, fp);
	
	portcall = ax25_ntoa(neighbour);

	quality = port_list[index].default_qual;

	while (fgets(neigh_buffer, 90, fp) != NULL) {
		addr     = strtok(neigh_buffer, " ");
		callsign = strtok(NULL, " ");
		device   = strtok(NULL, " ");
		qual     = atoi(strtok(NULL, " "));
		lock     = atoi(strtok(NULL, " "));

		if (strcmp(callsign, portcall) == 0 &&
		    strcmp(device, port_list[index].device) == 0 &&
		    lock == 1) {
			quality = qual;
			break;
		}
	}

	fclose(fp);

	nr_node.callsign    = *neighbour;
	memcpy(nr_node.mnemonic, buffer, MNEMONIC_LEN);
	nr_node.mnemonic[MNEMONIC_LEN] = '\0';

	if ((p = strchr(nr_node.mnemonic, ' ')) != NULL)
		*p = '\0';

	if (!validmnemonic(nr_node.mnemonic)) {
		if (debug && logging)
			syslog(LOG_DEBUG, "rejecting route, invalid mnemonic - %s\n", nr_node.mnemonic);
	} else {
		nr_node.neighbour   = *neighbour;
		strcpy(nr_node.device, port_list[index].device);
		nr_node.quality     = quality;
		nr_node.obs_count   = obs_count;

		if (ioctl(s, SIOCADDRT, &nr_node) == -1) {
			if (logging)
				syslog(LOG_ERR, "netromr: SIOCADDRT: %m");
			close(s);
			return;
		}
	}

	buffer += MNEMONIC_LEN;
	length -= MNEMONIC_LEN;

	while (length >= ROUTE_LEN) {
		/*
		 * Ensure that a) the route is not via me, and
		 *             b) it is better than my minimum acceptable quality
		 */
		if (ax25_cmp(&my_call, (ax25_address *)(buffer + 13)) != 0 &&
		    buffer[20] > port_list[index].worst_qual) {
			memcpy(&nr_node.callsign, buffer + 0, CALLSIGN_LEN);
			memcpy(nr_node.mnemonic,  buffer + 7, MNEMONIC_LEN);
			nr_node.mnemonic[MNEMONIC_LEN] = '\0';

			if ((p = strchr(nr_node.mnemonic, ' ')) != NULL)
				*p = '\0';

			if (!validmnemonic(nr_node.mnemonic)) {
				if (debug && logging)
					syslog(LOG_DEBUG, "rejecting route, invalid mnemonic - %s\n", nr_node.mnemonic);
			} else {
				nr_node.neighbour   = *neighbour;
				strcpy(nr_node.device, port_list[index].device);
				nr_node.quality     = ((quality * buffer[20]) + 128) / 256;
				nr_node.obs_count   = obs_count;

				if (ioctl(s, SIOCADDRT, &nr_node) == -1) {
					if (logging)
						syslog(LOG_ERR, "netromr: SIOCADDRT: %m");
					close(s);
					return;
				}
			}
		}

		buffer += ROUTE_LEN;
		length -= ROUTE_LEN;
	}

	close(s);
}
