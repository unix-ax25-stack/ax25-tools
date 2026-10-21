/* kissparms_wampes.c - KISS parameter commands via WAMPES node
 *
 * Sends KISS parameter commands to a WAMPES node via its service socket.
 * WAMPES supports a subset of KISS parameters via its "kiss" command.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netax25/ax25.h>
#include <netax25/axconfig.h>
#include <netax25/wampes_client.h>
#include "wampes.h"
#include <netax25/axlib.h>

/* KISS parameter numbers (from kissparms.h) */
#define PARAM_TXDELAY     1
#define PARAM_PERSIST     2
#define PARAM_SLOTTIME    3
#define PARAM_TXTAIL      4
#define PARAM_FULLDUP     5
#define PARAM_HARDWARE    6
#define PARAM_FECLEVEL    8
#define PARAM_RETURN      0xFF
#define PARAM_UP          0x82
#define PARAM_DOWN        0x81

/* WAMPES parameter names (from devparam.h) */
static const struct {
    int kiss_param;
    const char *wampes_name;
} param_map[] = {
    { PARAM_TXDELAY,    "TxDelay" },
    { PARAM_PERSIST,    "Persist" },
    { PARAM_SLOTTIME,   "SlotTime" },
    { PARAM_TXTAIL,     "TxTail" },
    { PARAM_FULLDUP,    "FullDup" },
    { PARAM_HARDWARE,   "Hardware" },
    { 0, NULL }
};

/* Map kissparms parameter to WAMPES parameter name */
static const char *kiss_to_wampes_param(int param)
{
    for (int i = 0; param_map[i].wampes_name; i++) {
        if (param_map[i].kiss_param == param)
            return param_map[i].wampes_name;
    }
    return NULL;
}

/* Send a single KISS parameter command to WAMPES node */
static int send_wampes_kiss_cmd(int fd, const char *iface, const char *param,
                                int value)
{
    char cmd[128];
    int len;

    if (value >= 0)
        len = snprintf(cmd, sizeof(cmd), "kiss %s %s %d\n", iface, param, value);
    else
        len = snprintf(cmd, sizeof(cmd), "kiss %s %s\n", iface, param);

    if (len < 0 || len >= (int)sizeof(cmd))
        return -1;

    if (write(fd, cmd, len) != len)
        return -1;

    /* Read response (WAMPES sends "***" on success or error) */
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

/* Send special RETURN command (0xFF) - sends KISS 0xC0 0xFF 0xC0 to TNC */
static int send_wampes_return(int fd, const char *iface)
{
    /* WAMPES doesn't have a direct "return" param, send raw KISS */
    /* We send it as a raw frame via monitor socket - but kissparms
     * doesn't have access to that here. For now, return not implemented. */
    fprintf(stderr, "kissparms: RETURN (0xFF) not implemented for WAMPES\n");
    return 1;
}

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

/* Send UP/DOWN commands - passed through to TNC */
static int send_wampes_updown(int fd, const char *iface, int up)
{
    /* WAMPES doesn't have ifconfig up/down for interfaces.
     * Pass through to TNC as KISS command. */
    const char *cmd = up ? "param Up\n" : "param Down\n";
    if (write(fd, cmd, strlen(cmd)) < 0)
        return -1;
    return 0;
}

/* Main entry point for WAMPES kissparms */
int kissparms_wampes(const char *port, struct ax25_port_info *pinfo,
                     int crcmode, int txdelay, int txtail, int persist,
                     int slottime, int fulldup, int hardware, int feclevel,
                     int kissoff, int X)
{
    const char *iface;
    int fd_wampes;

    /* Get the interface name from port info */
    iface = wampes_iface(port);
    if (!iface) {
        fprintf(stderr, "kissparms: no interface for port '%s'\n", port);
        return 1;
    }

    /* Open connection to WAMPES service socket */
    int fd = wampes_dial(wampes_address(port));
    if (fd < 0) {
        fprintf(stderr, "kissparms: cannot connect to WAMPES node for port '%s'\n", port);
        return 1;
    }

    /* Set binary mode */
    if (write(fd, "binary\n", 7) != 7) {
        close(fd);
        return 1;
    }

    int rc = 0;

    /* Handle special cases first */
    if (crcmode != -1) {
        if (send_wampes_crc(fd, iface, crcmode) != 0)
            rc = 1;
    }

    /* Send KISS parameters via WAMPES "kiss" command */
    if (txdelay != -1) {
        if (send_wampes_kiss_cmd(fd, iface, "TxDelay", txdelay) != 0)
            rc = 1;
    }
    if (txtail != -1) {
        if (send_wampes_kiss_cmd(fd, iface, "TxTail", txtail) != 0)
            rc = 1;
    }
    if (persist != -1) {
        if (send_wampes_kiss_cmd(fd, iface, "Persist", persist) != 0)
            rc = 1;
    }
    if (slottime != -1) {
        if (send_wampes_kiss_cmd(fd, iface, "SlotTime", slottime) != 0)
            rc = 1;
    }
    if (fulldup != -1) {
        if (send_wampes_kiss_cmd(fd, iface, "FullDup", fulldup) != 0)
            rc = 1;
    }
    if (hardware != -1) {
        if (send_wampes_kiss_cmd(fd, iface, "Hardware", hardware) != 0)
            rc = 1;
    }
    if (feclevel != -1) {
        if (send_wampes_kiss_cmd(fd, iface, "FECLevel", feclevel) != 0)
            rc = 1;
    }

    close(fd);
    return rc;
}