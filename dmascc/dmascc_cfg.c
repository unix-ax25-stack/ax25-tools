/*
 * $Id: dmascc_cfg.c,v 0.7 1997/12/02 11:31:21 oe1kib Exp $
 *
 * Configuration utility for dmascc driver
 * Copyright (C) 1997 Klaus Kudielka
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/dmascc.h>
#include <asm/param.h>

void usage(char *name)
{
  fprintf(stderr,
	  "usage:   %s <interface> [ options ... ]\n\n"
	  "options: --show           show configuration\n"
	  "         --speed <f>      set baud rate\n"
	  "         --nrzi 0 | 1     set NRZ or NRZI encoding\n"
	  "         --clocks <i>     set clock mode\n"
	  "         --txdelay <f>    set transmit delay in ms\n"
	  "         --txtime <f>     set maximum transmit time in s\n"
	  "         --sqdelay <f>    set squelch delay in ms\n"
	  "         --slottime <f>   set slot time in ms\n"
	  "         --waittime <f>   set wait time after transmit in ms\n"
	  "         --persist <i>    set persistence parameter\n"
	  "         --dma <i>        set DMA channel\n", name);
}

int main(int argc, char *argv[])
{
  int sk, show = 0, set = 0, error = 0;
  struct ifreq ifr;
  struct scc_param param;
  char **option, *end;

  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return 2;
  }

  strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);
  ifr.ifr_data = (caddr_t) &param;
  if (ioctl(sk, SIOCGSCCPARAM, &ifr) < 0) {
    perror("ioctl");
    close(sk);
    return 3;
  }

  option = argv + 2;
  while (!error && *option != NULL) {
    if (!strcmp(*option, "--show")) {
      show = 1;
      option++;
    } else if (!strcmp(*option, "--speed")) {
      option++;
      if (*option != NULL) {
	double speed;
	set = 1;
	speed = strtod(*option++, &end);
	if (*end) error = 1; else {
	  if (speed <= 0.0) param.brg_tc = -1; 
	  else param.brg_tc = param.pclk_hz / (speed * 2) - 2;
	  if (param.brg_tc > 0xffff) param.brg_tc = -1;
	}
      } else error = 1;
    } else if (!strcmp(*option, "--nrzi")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.nrzi = strtol(*option++, &end, 0);
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--clocks")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.clocks = strtol(*option++, &end, 0);
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--txdelay")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.txdelay = TMR_0_HZ * strtod(*option++, &end) / 1000.0;
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--txtime")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.txtime = HZ * strtod(*option++, &end);
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--sqdelay")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.sqdelay = TMR_0_HZ * strtod(*option++, &end) / 1000.0;
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--slottime")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.slottime = TMR_0_HZ * strtod(*option++, &end) / 1000.0;
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--waittime")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.waittime = TMR_0_HZ * strtod(*option++, &end) / 1000.0;
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--persist")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.persist = strtol(*option++, &end, 0);
	if (*end) error = 1;
      } else error = 1;
    } else if (!strcmp(*option, "--dma")) {
      option++;
      if (*option != NULL) {
	set = 1;
	param.dma = strtol(*option++, &end, 0);
	if (*end) error = 1;
      } else error = 1;
    } else error = 1;
  }

  if (error) {
    usage(argv[0]);
    close(sk);
    return 1;
  }

  if (set) {
    if (ioctl(sk, SIOCSSCCPARAM, &ifr) < 0) {
      perror("ioctl");
      close(sk);
      return 4;
    }
  }

  if (show) {
    double speed;
    if (param.brg_tc < 0) speed = 0.0;
    else speed = ((double) param.pclk_hz) / ( 2 * (param.brg_tc + 2));
    printf("speed    = %.2f\nnrzi     = %d\nclocks   = 0x%02X\n"
	   "txdelay  = %.2f ms\ntxtime   = %.2f s\nsqdelay  = %.2f ms\n"
	   "slottime = %.2f ms\nwaittime = %.2f ms\npersist  = %d\n"
	   "dma      = %d\n",
	   speed, param.nrzi, param.clocks,
	   param.txdelay * 1000.0 / TMR_0_HZ,
	   (double) param.txtime / HZ,
	   param.sqdelay * 1000.0 / TMR_0_HZ,
	   param.slottime * 1000.0 / TMR_0_HZ,
	   param.waittime * 1000.0 / TMR_0_HZ,
	   param.persist, param.dma);
  }

  close(sk);
  return 0;
}
