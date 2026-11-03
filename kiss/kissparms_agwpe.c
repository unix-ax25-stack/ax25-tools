/* kissparms_agwpe.c - KISS parameter commands via AGWPE server
 *
 * Sends KISS parameter commands via AGWPE 'X' (raw KISS) frames.
 * AGWPE 'X' frames carry raw KISS commands to the TNC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netax25/ax25.h>
#include <netax25/axconfig.h>
#include <netax25/agwpe_client.h>
#include <netax25/agwpe.h>

/* KISS parameter numbers */
#define PARAM_TXDELAY     1
#define PARAM_PERSIST     2
#define PARAM_SLOTTIME    3
#define PARAM_TXTAIL      4
#define PARAM_FULLDUP     5
#define PARAM_HARDWARE    6
#define PARAM_FECLEVEL    8
#define PARAM_RETURN      0xFF

/* Build a KISS command frame */
static int build_kiss_cmd(unsigned char *frame, int param, int value)
{
    unsigned char *p = frame;
    *p++ = 0xC0;  /* FEND */

    if (param == 0xFF) {
        /* RETURN - no argument */
        *p++ = 0xFF;
        *p++ = 0xC0;
        return p - frame;
    }

    *p++ = param & 0xFF;
    *p++ = value & 0xFF;
    *p++ = 0xC0;  /* FEND */
    return p - frame;
}

/* Send KISS command via AGWPE 'X' frame */
static int send_agwpe_kiss_cmd(agwpe_client_t *cl, unsigned char port,
                               unsigned char channel, int param, int value)
{
    unsigned char frame[32];
    int len = build_kiss_cmd(frame, param, value);

    struct agwpe_s hdr;
    agwpe_header_init(&hdr, port, AGWPE_CMD_RAW, 0, NULL, NULL, len);

    int rc = agwpe_client_send_frame(cl, &hdr, frame);
    return rc;
}

/* Main entry point for AGWPE kissparms */
int kissparms_agwpe(const char *port, struct ax25_port_info *pinfo,
                    int crcmode, int txdelay, int txtail, int persist,
                    int slottime, int fulldup, int hardware, int feclevel,
                    int kissoff, int X)
{
    agwpe_client_t *cl = agwpe_client_new(NULL, NULL);
    if (!cl) {
        fprintf(stderr, "kissparms: cannot create AGWPE client\n");
        return 1;
    }

    /* Connect to AGWPE server */
    const char *host = getenv("AGWPE_HOST");
    int tcp_port = atoi(getenv("AGWPE_PORT") ? : "8200");

    int rc = 0;
    if (strchr(port, ':') || strchr(port, '/')) {
        /* Unix socket */
        rc = agwpe_client_connect_unix(cl, port);
    } else {
        rc = agwpe_client_connect_host(cl, host, tcp_port);
    }
    if (rc != 0) {
        fprintf(stderr, "kissparms: cannot connect to AGWPE server\n");
        agwpe_client_free(cl);
        return 1;
    }

    unsigned char port_num = pinfo->port;
    unsigned char channel = pinfo->channel;

    int rc_final = 0;

    /* Send KISS commands via AGWPE 'X' frames */
    if (crcmode != -1) {
        /* CRC mode not directly supported via KISS - firmware dependent */
        fprintf(stderr, "kissparms: CRC mode via AGWPE not implemented\n");
        rc = 1;
    }

    if (txdelay != -1) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_TXDELAY, txdelay) != 0)
            rc = 1;
    }
    if (txtail != -1) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_TXTAIL, txtail) != 0)
            rc = 1;
    }
    if (persist != -1) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_PERSIST, persist) != 0)
            rc = 1;
    }
    if (slottime != -1) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_SLOTTIME, slottime) != 0)
            rc = 1;
    }
    if (fulldup != -1) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_FULLDUP, fulldup) != 0)
            rc = 1;
    }
    if (hardware != -1) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_HARDWARE, hardware) != 0)
            rc = 1;
    }
    if (feclevel != -1) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_FECLEVEL, feclevel) != 0)
            rc = 1;
    }

    if (kissoff) {
        if (send_agwpe_kiss_cmd(cl, port_num, channel, PARAM_RETURN, -1) != 0)
            rc = 1;
    }

    agwpe_client_close(cl);
    agwpe_client_free(cl);

    return rc;
}