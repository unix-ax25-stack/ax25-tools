/*****************************************************************************/

/*
 *	setcrystal.c  --  crystal soundcard configuration utility
 *
 *	Copyright (C) 1996  Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Please note that the GPL allows you to use the driver, NOT the radio.
 *  In order to use the radio, you need a license from the communications
 *  authority of your country.
 *
 */

/* --------------------------------------------------------------------- */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef __GLIBC__
#include <sys/io.h>
#else
#include <asm/io.h>
#endif

/* --------------------------------------------------------------------- */

static const unsigned char crystal_key[32] = {
	0x96, 0x35, 0x9a, 0xcd, 0xe6, 0xf3, 0x79, 0xbc, 
	0x5e, 0xaf, 0x57, 0x2b, 0x15, 0x8a, 0xc5, 0xe2, 
	0xf1, 0xf8, 0x7c, 0x3e, 0x9f, 0x4f, 0x27, 0x13, 
	0x09, 0x84, 0x42, 0xa1, 0xd0, 0x68, 0x34, 0x1a
}; 

#define KEY_PORT 0x279
#define CSN 1 /* card select number */

/* --------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	int wssbase = 0x534;
	int synbase = 0x388;
	int sbbase = 0x220;
	int irq = 5;
	int dma = 1;
	int dma2 = 3;

	int i;

	printf("Crystal PnP Soundcard enabler\n"
	       "(C) 1996 by Thomas Sailer, HB9JNX/AE4WA\n"
	       "WARNING: This utility is incompatible with OS PnP support!\n"
	       "         Remove it as soon as Linux supports PnP!\n");
	while ((i = getopt(argc, argv, "i:d:c:s:w:f:h")) != EOF) {
		switch (i) {
		case 'i':
			irq = strtoul(optarg, NULL, 0);
			break;
		case 'd':
			dma = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			dma2 = strtoul(optarg, NULL, 0);
			break;
		case 's':
			sbbase = strtoul(optarg, NULL, 0);
			break;
		case 'w':
			wssbase = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			synbase = strtoul(optarg, NULL, 0);
			break;

		default:
		case ':':
		case '?':
		case 'h':
			fprintf(stderr, "usage: [-w wssio] [-s sbio] "
				"[-f synthio] [-i irq] [-d dma] [-c dma2]\n");
			exit(1);
		}
	}

	if ((i = ioperm(KEY_PORT, 1, 1))) {
		perror("ioperm");
		exit(1);
	}
	/*
	 * send crystal key
	 */
	for (i = 0; i < 32; i++) 
		outb(crystal_key[i], KEY_PORT);
	/*
	 * send config data
	 */
	outb(0x6, KEY_PORT);
	outb(CSN, KEY_PORT);
	outb(0x15, KEY_PORT);
	outb(0x0, KEY_PORT); /* logical device 0 */
	outb(0x47, KEY_PORT);
	outb(wssbase >> 8, KEY_PORT);
	outb(wssbase & 0xff, KEY_PORT);
	outb(0x48, KEY_PORT);
	outb(synbase >> 8, KEY_PORT);
	outb(synbase & 0xff, KEY_PORT);
	outb(0x42, KEY_PORT);
	outb(sbbase >> 8, KEY_PORT);
	outb(sbbase & 0xff, KEY_PORT);
	outb(0x22, KEY_PORT);
	outb(irq, KEY_PORT);
	outb(0x2a, KEY_PORT);
	outb(dma, KEY_PORT);
	outb(0x25, KEY_PORT);
	outb(dma2, KEY_PORT);
	outb(0x33, KEY_PORT);
	outb(0x1, KEY_PORT); /* activate logical device */
	outb(0x79, KEY_PORT); /* activate part */

	printf("Crystal CS423[26] set to: WSS iobase 0x%x, Synth iobase 0x%x,"
	       " SB iobase 0x%x,\n  IRQ %u, DMA %u, DMA2 %u\n", wssbase,
	       synbase, sbbase, irq, dma, dma2);
	exit(0);
}

/* --------------------------------------------------------------------- */
