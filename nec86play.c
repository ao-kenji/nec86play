/*
 * Copyright (c) 2014 Kenji Aoyama.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * NEC PC-9801-86 sound board test on OpenBSD/luna88k
 *
 * Note
 * If you want to write data to the mmapped device, you must change
 * securelevel to -1 in /etc/rc.securelevel and reboot your system.
 */

#include <stdio.h>
#include <stdlib.h>	/* getprogname(3) */
#include <unistd.h>	/* getopt(3) */
#include <math.h>
#include "nec86hw.h"

void	usage(void);
int	set_data(u_int8_t *, int, int, int);
void	setup_freq_table();

/* global */
int bpf;		/* bytes per frame */
int debug = 0;		/* debug */
u_int8_t buf[NEC86_BUFFSIZE];

#define NNOTE	128	/* number of MIDI note */
int freq[NNOTE];

struct note {
	int num;	/* note number */
	int dur;	/* duration */
};

/* From "An die Freude / Ode to Joy" by Ludwig van Beethoven */
struct note music[] = {
	{ 64,	2400 },	/* E */
	{ 64,	2400 },	/* E */
	{ 65,	2400 },	/* F */
	{ 67,	2400 },	/* G */

	{ 67,	2400 },	/* G */
	{ 65,	2400 },	/* F */
	{ 64,	2400 },	/* E */
	{ 62,	2400 },	/* D */

	{ 60,	2400 },	/* C */
	{ 60,	2400 },	/* C */
	{ 62,	2400 },	/* D */
	{ 64,	2400 },	/* E */

	{ 64,	3600 },	/* E */
	{ 62,	1200 },	/* D */
	{ 62,	4800 },	/* D */

	{ -1,	-1 },	/* EOM: End Of Music:-) */
};

int
main(int argc, char **argv)
{
	u_long rate = 8000, hwrate;
	u_int prec = 16;
	int chan = 2;

	int count, finish, nframes;
	u_int8_t bits;
	u_int8_t *p = buf;

	/*
	 * parse options
	 */
	int ch;
	extern char *optarg;
	extern int optind, opterr;

	while ((ch = getopt(argc, argv, "dr:")) != -1) {
		switch (ch) {
		case 'd':	/* debug flag */
			debug = 1;
			break;
		case 'r':	/* sampling rate */
			rate = atoi(optarg);
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	setup_freq_table();

	bpf = (prec / 8) * chan;	/* bytes per frame */

	nec86_open();
	if (nec86hw_init() == -1) goto exit;

	nec86hw_set_mode_playing();
	nec86hw_set_precision_real(prec);
	bits = nec86hw_rate_bits(rate);
	nec86hw_set_rate_real(bits);
	hwrate = nec86hw_rate_table[bits];

	printf("requested rate: %d -> hardware rate: %d\n", rate, hwrate);

	nframes = set_data(buf, hwrate, freq[music[0].num], music[0].dur);

	nec86hw_reset_fifo();

	if (debug) {
		/* check status */
		printf("full: 0x%02x, empty: 0x%02x\n",
			nec86hw_seeif_fifo_full(), nec86hw_seeif_fifo_empty());
	}

	/* send first block to fifo */
	nec86fifo_output_stereo_16_direct(buf, nframes);	

	/* start fifo */
	count = 1;
	finish = 0;
	nec86hw_start_fifo();

	for (;;) {
		if (!finish) {
			/* prepare next block */
			nframes = set_data(buf, hwrate,
				freq[music[count].num], music[count].dur);
		}

		/* wait for fifo becomes empty */
		do { /* nothing */ } while (!nec86hw_seeif_fifo_empty());

		if (finish)
			break;

		/* send next block to fifo */
		nec86fifo_output_stereo_16_direct(buf, nframes);	

		count++;
		if (music[count].num == -1)
			finish = 1;
	}

	nec86hw_stop_fifo();
exit:
	nec86_close();
}

int
set_data(u_int8_t *p, int rate, int hz, int samples)
{
	/*
	 * We use signed 16 bit data here.
	 * The hardware is designed with MSByte first (Big Endian).
	 */
	int16_t val;
	int i;

	if (debug) {
		printf("hz: %d, samples: %d\n", hz, samples);
	}

	/* If data size is larger than the hardware buffer size, clip it. */
	if (samples > NEC86_BUFFSIZE / bpf)
		samples = NEC86_BUFFSIZE / bpf;

	for (i = 0; i < samples; i++) {
		val = (int16_t)(32767 *
			sin(2 * M_PI * i * hz / rate));
		/* L channel */
		*p++ = (val >> 8) & 0xff;
		*p++ = val & 0xff;
		/* R channel */
		*p++ = (val >> 8) & 0xff;
		*p++ = val & 0xff;
	}

	return i;
}

void
usage(void)
{
	printf("Usage: %s [options] ...\n", getprogname());
	printf("\t-d	: debug flag\n");
	printf("\t-r #	: sampling rate\n");
	exit(1);
}

void
setup_freq_table(void) {
	int i;

	for (i = 0; i < NNOTE; i++) {
		freq[i] = (int)(440.0 * pow(2.0, ((double)i - 69.0) / 12.0));
	}
}
