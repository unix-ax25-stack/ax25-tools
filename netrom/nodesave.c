#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#ifdef HAVE_NETAX25_AX25_H
#include <netax25/ax25.h>
#else
#include <netax25/kernel_ax25.h>
#endif

#include <netax25/axconfig.h>

#include <netax25/procutils.h>

int main(int argc, char **argv)
{
	FILE *fp = stdout;
	struct proc_nr_nodes *nodes, *nop;
	struct proc_nr_neigh *neighs, *nep;

	if (ax25_config_load_ports() == 0) {
		fprintf(stderr, "nodesave: no AX.25 port data configured\n");
		return 1;
	}

	if (argc > 1) {
		if ((fp = fopen(argv[1], "w")) == NULL) {
			fprintf(stderr, "nodesave: cannot open file %s\n", argv[1]);
			return 1;
		}
	}

	if ((neighs = read_proc_nr_neigh()) == NULL && errno != 0) {
		perror("nodesave: read_proc_nr_neigh");
		fclose(fp);
		return 1;
	}

	if ((nodes = read_proc_nr_nodes()) == NULL && errno != 0) {
		perror("nodesave: read_proc_nr_nodes");
		free_proc_nr_neigh(neighs);
		fclose(fp);
		return 1;
	}

	fprintf(fp, "#! /bin/sh\n#\n# Locked routes:\n#\n");

	for (nep = neighs; nep != NULL; nep = nep->next) {
		if (nep->lock) {
			fprintf(fp, "nrparms -routes \"%s\" %s + %d\n",
				ax25_config_get_name(nep->dev),
				nep->call,
				nep->qual);
		}
	}

	fprintf(fp, "#\n# Nodes:\n#\n");

	for (nop = nodes; nop != NULL; nop = nop->next) {
		if ((nep = find_neigh(nop->addr1, neighs)) != NULL) {
			fprintf(fp, "nrparms -nodes %s + \"%s\" %d %d \"%s\" %s\n",
				nop->call,
				nop->alias,
				nop->qual1,
				nop->obs1,
				ax25_config_get_name(nep->dev),
				nep->call);
		}

		if (nop->n > 1 && (nep = find_neigh(nop->addr2, neighs)) != NULL) {
			fprintf(fp, "nrparms -nodes %s + \"%s\" %d %d \"%s\" %s\n",
				nop->call,
				nop->alias,
				nop->qual2,
				nop->obs2,
				ax25_config_get_name(nep->dev),
				nep->call);
		}

		if (nop->n > 2 && (nep = find_neigh(nop->addr3, neighs)) != NULL) {
			fprintf(fp, "nrparms -nodes %s + \"%s\" %d %d \"%s\" %s\n",
				nop->call,
				nop->alias,
				nop->qual3,
				nop->obs3,
				ax25_config_get_name(nep->dev),
				nep->call);
		}
	}

	free_proc_nr_neigh(neighs);
	free_proc_nr_nodes(nodes);

	fclose(fp);

	if (argc > 1) {
		chmod(argv[1], S_IEXEC);
	}

	return 0;
}
