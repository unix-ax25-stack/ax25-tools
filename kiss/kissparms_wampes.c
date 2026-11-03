/* kissparms_wampes.c - KISS parameter commands via WAMPES node
 *
 * Sends KISS parameter commands to a WAMPES node via its service socket.
 * WAMPES supports a subset of KISS parameters via its "param" command.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netax25/ax25.h>
#include <netax25/axconfig.h>
#include <netax25/wampes_client.h>
#include <netax25/axlib.h>
#include "kissparms.h"

static const struct {
    int kiss_param;
    const char *wampes_name;
} param_map_unused[] __attribute__((unused)) = {
    { PARAM_TXDELAY,    "TxDelay" },
    { PARAM_PERSIST,    "Persist" },
    { PARAM_SLOTTIME,   "SlotTime" },
    { PARAM_TXTAIL,     "TxTail" },
    { PARAM_FULLDUP,    "FullDup" },
    { PARAM_HARDWARE,   "Hardware" },
    { 0, NULL }
};

/* unused */

/* Send a single KISS parameter command to WAMPES node via "param" command */
static int send_wampes_param_cmd(int fd, const char *iface, const char *param,
                                 int value)
{
    char cmd[128];
    int len;

    if (value >= 0)
        len = snprintf(cmd, sizeof(cmd), "param %s %s %d\n", iface, param, value);
    else
        len = snprintf(cmd, sizeof(cmd), "param %s %s\n", iface, param);

    if (len < 0 || len >= (int)sizeof(cmd))
        return -1;

    fprintf(stderr, "DEBUG: sending cmd='%s'", cmd);
    if (write(fd, cmd, len) != len)
        return -1;

    /* Read response (WAMPES sends "***" on success or error) */
    char buf[256];
    char *p = buf;
    int remaining = sizeof(buf) - 1;
    while (remaining > 0) {
        ssize_t n = read(fd, p, remaining);
        fprintf(stderr, "DEBUG: read %zd bytes\n", n);
        if (n <= 0)
            return -1;
        p += n;
        remaining -= n;
        if (p - buf >= 4 && strncmp(p - 4, "*** ", 4) == 0)
            break;
    }

    fprintf(stderr, "DEBUG: response='%.*s'\n", (int)(p - buf), buf);
    return 0;
}

/* unused */

/* Send CRC mode command - maps to "ifconfig <iface> crc <mode>" */
static int send_wampes_crc(int fd, const char *iface, int mode)
{
    const char *crc_mode;
    switch (mode) {
        case 0: crc_mode = "no"; break;      /* No CRC */
        case 1: crc_mode = "smack"; break;   /* SMACK CRC */
        case 2: crc_mode = "flexnet"; break; /* FlexNet CRC */
        default: crc_mode = "auto"; break;   /* Auto-detect */
    }

    char cmd[128];
    int len = snprintf(cmd, sizeof(cmd), "ifconfig %s crc %s\n", iface, crc_mode);
    if (len < 0 || len >= (int)sizeof(cmd))
        return -1;

    if (write(fd, cmd, len) != len)
        return -1;

    /* Read response */
    char buf[256];
    char *p = buf;
    int remaining = sizeof(buf) - 1;
    while (remaining > 0) {
        ssize_t n = read(fd, p, remaining);
        if (n <= 0)
            return -1;
        p += n;
        remaining -= n;
        if (p - buf >= 4 && strncmp(p - 4, "*** ", 4) == 0)
            break;
    }

    return 0;
}

/* unused */

/* Main entry point for WAMPES kissparms */
int kissparms_wampes(const char *port, struct ax25_port_info *pinfo,
                     int crcmode, int txdelay, int txtail, int persist,
                     int slottime, int fulldup, int hardware, int feclevel,
                     int kissoff, int X)
{
    const char *iface;

    /* Get the interface name from port info */
    iface = wampes_iface(port);
    if (!iface) {
        fprintf(stderr, "kissparms: no interface for port '%s'\n", port);
        return 1;
    }
    fprintf(stderr, "DEBUG: iface='%s'\n", iface);

    /* Open connection to WAMPES command channel (netcmd) for param commands */
    const char *addr = wampes_cmd_address(port);
    fprintf(stderr, "DEBUG: wampes_cmd_address='%s'\n", addr);
    int fd = wampes_dial(addr);
    if (fd < 0) {
        fprintf(stderr, "kissparms: cannot connect to WAMPES node for port '%s' (errno=%d: %s)\n", port, errno, strerror(errno));
        return 1;
    }
    fprintf(stderr, "DEBUG: connected to WAMPES node, fd=%d\n", fd);

    /* Set binary mode */
    if (write(fd, "binary\n", 7) != 7) {
        fprintf(stderr, "kissparms: failed to set binary mode\n");
        close(fd);
        return 1;
    }

    int rc = 0;

    /* Handle special cases first */
    if (crcmode != -1) {
        if (send_wampes_crc(fd, iface, crcmode) != 0)
            rc = 1;
    }

    /* Send KISS parameters via WAMPES "param" command */
    if (txdelay != -1) {
        fprintf(stderr, "DEBUG: sending TxDelay=%d\n", txdelay);
        if (send_wampes_param_cmd(fd, iface, "TxDelay", txdelay) != 0)
            rc = 1;
    }
    if (txtail != -1) {
        fprintf(stderr, "DEBUG: sending TxTail=%d\n", txtail);
        if (send_wampes_param_cmd(fd, iface, "TxTail", txtail) != 0)
            rc = 1;
    }
    if (persist != -1) {
        fprintf(stderr, "DEBUG: sending Persist=%d\n", persist);
        if (send_wampes_param_cmd(fd, iface, "Persist", persist) != 0)
            rc = 1;
    }
    if (slottime != -1) {
        fprintf(stderr, "DEBUG: sending SlotTime=%d\n", slottime);
        if (send_wampes_param_cmd(fd, iface, "SlotTime", slottime) != 0)
            rc = 1;
    }
    if (fulldup != -1) {
        fprintf(stderr, "DEBUG: sending FullDup=%d\n", fulldup);
        if (send_wampes_param_cmd(fd, iface, "FullDup", fulldup) != 0)
            rc = 1;
    }
    if (hardware != -1) {
        fprintf(stderr, "DEBUG: sending Hardware=%d\n", hardware);
        if (send_wampes_param_cmd(fd, iface, "Hardware", hardware) != 0)
            rc = 1;
    }
    //if (badconcept_TxMute != -1) {
        //fprintf(stderr, "DEBUG: sending TxMute=%d\n", feclevel);
        //if (send_wampes_param_cmd(fd, iface, "TxMute", TxMute) != 0)
            //rc = 1;
    //}
    //if (feclevel != -1) {
        //fprintf(stderr, "DEBUG: sending RTS=%d\n", feclevel);
        //if (send_wampes_param_cmd(fd, iface, "RTS", feclevel) != 0)
            //rc = 1;
    //}
    //if (badconcept_TxRTS != -1) {
        //fprintf(stderr, "DEBUG: sending RTS=%d\n", feclevel);
        //if (send_wampes_param_cmd(fd, iface, "RTS", feclevel) != 0)
            //rc = 1;
    //}

#ifdef notdef
  /* AI, please read:

    //...badconcept: and the other missing ones:
        //{ PARAM_SPEED,    "Speed" },
        //{ PARAM_ENDDELAY, "EndDelay" },
        //{ PARAM_GROUP,    "Group" },
        //{ PARAM_IDLE,     "Idle" },
        //{ PARAM_MIN,      "Min" },
        //{ PARAM_MAXKEY,   "MaxKey" },

     // the really bad idea by the AI which implemented this for kissparms_wampes.c and kissparms_agwpe.c is,
     // that the separation makes it more complex. The basic idea was to have understandable code. But this
     // is a mess. My first correction was: the PARAM_ defines of every kisspasms*c file moved to one, kissparms.h
     // That means,  hardcoded "int kissparms_wampes(const char *port, struct ax25_port_info *pinfo,¶
                     int crcmode, int txdelay, int txtail, int persist,¶
                     int slottime, int fulldup, int hardware, int feclevel,¶
                     int kissoff, int X)¶" is a desingn flaw.
        The names (for wampes at least) cound be defined in kissparms.h.

	We will support kiss params 0x01-0x0f and 0xff for wampes and agwpe.
	And for wampes: wampes does not support multiport-kiss. So cmd | 0x80 is safe. -> Down and Up will be
	executed at localy (-> asy_down, asy_up) and is not forwarded to the tnc.
	0xff will be forwarded to the kiss tnc attached to the wampes port.
	In wampes we need to fix in kiss.c and kisstcp.c: PARAM_HARDWARE with more than one argument (see byte sequence concept in the kissparms kernel-ax25 part). About agwpe, look at the specs.
	In wampes we have a problem with PARAM_RETURN. kiss_ioctl says  set = 1;        /* Note fall-thru */,
	But PARAM_RETURN has no argument. This leads the part
	"                *cp = (unsigned char) val;
                hbp->cnt = 2;"
	to add a random byte.
	Other params < 0x0f are ignored (not explicitely assiged in the case blocks -> for tnc hardware with
	commnnslike i.E. command PARAM_MUTE is not supported without recompile. No reason for being so pedantic here.
	Same for kisstcp.c - except: It's ok that parameters > 0x0f && < 0xff ere left out.
#endif /* notdef */


    close(fd);
    return rc;
}
