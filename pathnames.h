
#define	CONF_AX25D_FILE		AX25_SYSCONFDIR"ax25d.conf"
#define	CONF_AXPORTS_FILE	AX25_SYSCONFDIR"axports"
#define	CONF_AXSPAWN_FILE	AX25_SYSCONFDIR"axspawn.conf"
#define	CONF_AXSPAWN_PROF_FILE	AX25_SYSCONFDIR"ax25.profile"
#define	CONF_NETROMD_FILE	AX25_SYSCONFDIR"nrbroadcast"
#define	CONF_NRPORTS_FILE	AX25_SYSCONFDIR"nrports"
#define	CONF_RIP98D_FILE	AX25_SYSCONFDIR"rip98d.conf"
#define	CONF_RSPORTS_FILE	AX25_SYSCONFDIR"rsports"
#define	CONF_RXECHO_FILE	AX25_SYSCONFDIR"rxecho.conf"
#define	CONF_TTYLINKD_FILE	AX25_SYSCONFDIR"ttylinkd.conf"

#define	DATA_MHEARD_FILE	AX25_LOCALSTATEDIR"mheard/mheard.dat"

#define	PROG_AX25RTCTL		AX25_SBINDIR"ax25rtctl"
#define	PROG_NRPARMS		AX25_SBINDIR"nrparms"

/*
 * useradd(8) is part of the system (passwd/shadow-utils), not of this
 * package.  AX25_SBINDIR is the directory axspawn is installed into, which
 * is only the same place with the default --prefix=/usr; with
 * --prefix=/opt/uax25 the old definition looked for
 * /opt/uax25/sbin/useradd and account creation failed with ENOENT.
 * On Linux the system path is fixed.  Elsewhere there is no useradd at all
 * in the usual case (macOS, BSD), and create_with_useradd is off by default;
 * keep the old path for a user-supplied wrapper.
 */
#ifdef __linux__
#define	PROG_USERADD		"/usr/sbin/useradd"
#else
#define	PROG_USERADD		AX25_SBINDIR"useradd"
#endif

/*
 * /var/lock is a Linux convention; macOS keeps its serial line locks in
 * /var/run.  Both are writable by the root that runs axspawn.
 */
#ifdef __APPLE__
#define	LOCK_AXSPAWN_FILE	"/var/run/axspawn"
#define	LOCK_SERIAL_DIR		"/var/run"
#else
#define	LOCK_AXSPAWN_FILE	"/var/lock/axspawn"
#define	LOCK_SERIAL_DIR		"/var/lock"
#endif

#define PROC_LOADAVG_FILE	"/proc/loadavg"
#define PROC_MEMINFO_FILE	"/proc/meminfo"
#define PROC_UPTIME_FILE	"/proc/uptime"

#define	PROC_AX25_FILE		"/proc/net/ax25"
#define	PROC_AX25_ROUTE_FILE	"/proc/net/ax25_route"
#define	PROC_IP_ROUTE_FILE	"/proc/net/route"
#define	PROC_NR_SYSCTL_DIR	"/proc/sys/net/netrom"
#define	PROC_RS_NEIGH_FILE	"/proc/net/rose_neigh"
#define	PROC_RS_NODES_FILE	"/proc/net/rose_nodes"
#define PROC_AX25_CALLS_FILE	"/proc/net/ax25_calls"
#define PROC_NR_NEIGH_FILE	"/proc/net/nr_neigh"
#define PROC_NR_NODES_FILE	"/proc/net/nr_nodes"
