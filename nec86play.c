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
 * NEC PC-9801-86 sound board test (PCM part) on OpenBSD/luna88k
 *
 * Note: use with /dev/pcexio support kernel.
 */

#include <stdio.h>
#include <stdlib.h>	/* getprogname(3) */
#include <unistd.h>	/* getopt(3) */
#include <math.h>
#include <sys/ioctl.h>
#include <machine/pcex.h>
#include "nec86hw.h"

#define DPRINTF(x)      if (debug) printf x

void	usage(void);
int	set_data(u_int8_t *, int, int, int);
void	setup_freq_table();

/* global */
int bpf;		/* bytes per frame */
int debug = 0;		/* debug */
u_int8_t  buf[NEC86_BUFFSIZE + 4];
u_int16_t wav[(NEC86_BUFFSIZE + 4) / 2];
#if 0
u_int8_t  fread_buf[NEC86_BUFFSIZE + 4];
#else
u_int8_t  fread_buf[65536];
#endif

FILE *wav_fp = NULL;
int wav_finish = 0;

int
main(int argc, char **argv)
{
	u_long rate = 8000, hwrate;
	u_int prec = 16;
	int chan = 2;
	int chunkframes;

	u_int level;
	int count = 0;
	int finish, nbytes, nframes, wm;
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

	if (argc != 1) {
		usage();
		return 1;
	}

	/* bytes per frame */
	bpf = (prec / 8) * chan;

	nec86_open();
	if (nec86hw_init() == -1) goto exit;

	/* should be set after calling nec86hw_init()! */
	level = nec86intlevel;
	DPRINTF(("Using INT%d\n", level));

	if (ioctl(nec86fd, PCEXSETLEVEL, &level) != 0) {
		printf("PCEXSETLEVEL failed\n");
		goto exit;
	}

	nec86hw_set_mode_playing();
	nec86hw_set_precision_real(prec);
	bits = nec86hw_rate_bits(rate);
	nec86hw_set_rate_real(bits);
	hwrate = nec86hw_rate_table[bits];

	DPRINTF(("requested rate: %d -> hardware rate: %d\n", rate, hwrate));

	/* number of frames in one chunk = 75% of hardware FIFO */
	chunkframes = (NEC86_BUFFSIZE + 4) / bpf * 3 / 4;

	/* watermark is 1/2 of chunkframes, should be multiple of 128 */
#if 0
	wm = (chunkframes * bpf / 4) & 0xffffff80;
#endif
	wm = ((NEC86_BUFFSIZE + 4) - chunkframes * bpf) & 0xffffff80;

	DPRINTF(("chunkframes %d, watermark %d bytes\n", chunkframes, wm));

	DPRINTF(("open %s\n", argv[0]));
	if (wav_open(argv[0]) != 0)
		return 1;

	nframes = set_wav_data(buf, hwrate, chunkframes);
	
	DPRINTF(("count=%d, put %d frames to buf\n", count, nframes));

	nec86hw_reset_fifo();

	/* send first block to fifo */
	nbytes = nec86fifo_output_stereo_16_direct(buf, nframes);	
	DPRINTF(("%d bytes to fifo\n", nbytes));

	/* start fifo */
	count++;
	finish = 0;
	nec86hw_start_fifo();
	nec86hw_enable_fifointr();
	nec86hw_set_watermark(wm);

	for (;;) {
		if (wav_finish) {
			/* prepare last silent block */
			nframes = set_data(buf, hwrate, 0, chunkframes);
			DPRINTF(("count=%d, put %d frames to buf (silent)\n",
				count, nframes));

			/* send last silent block to fifo */
			nec86hw_disable_fifointr();
			nframes = set_wav_data(buf, hwrate, chunkframes);
			DPRINTF(("count=%d, put %d frames to buf\n",
				count, nframes));
			nec86hw_enable_fifointr();
			nec86hw_set_watermark(wm);	/* need this? */
			DPRINTF(("%d bytes to fifo (silent)\n", nbytes));
		} else {
			/* prepare next block */
			nframes = set_wav_data(buf, hwrate, chunkframes);
			DPRINTF(("count=%d, put %d frames to buf\n",
				count, nframes));
		}

		/* wait for fifo becomes low */
		DPRINTF(("wait for INT\n"));
		ioctl(nec86fd, PCEXWAITINT, &level);
		nec86hw_clear_intrflg();

		if (finish)
			break;

		/* send next block to fifo */
		nec86hw_disable_fifointr();
		nbytes = nec86fifo_output_stereo_16_direct(buf, nframes);	
		nec86hw_enable_fifointr();
		nec86hw_set_watermark(wm);	/* need this? */
		DPRINTF(("%d bytes to fifo\n", nbytes));

		count++;
		if (wav_finish) finish = 1;
	}
	nec86hw_disable_fifointr();
	nec86hw_stop_fifo();
exit:
	wav_close();
	ioctl(nec86fd, PCEXRESETLEVEL, &level);
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

	DPRINTF(("hz: %d, samples: %d\n", hz, samples));

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

int
set_wav_data(u_int8_t *p, int rate, int samples)
{
	/*
	 * We use signed 16 bit data here.
	 * The hardware is designed with MSByte first (Big Endian).
	 */
	int16_t val;
	int i;
	size_t ns;

	/* If data size is larger than the hardware buffer size, clip it. */
	if (samples > NEC86_BUFFSIZE / bpf)
		samples = NEC86_BUFFSIZE / bpf;

	bzero(wav, NEC86_BUFFSIZE);
	ns = fread(wav, sizeof(u_int16_t), samples * 2, wav_fp);
	DPRINTF(("%s: read %d samples\n", __func__, ns / 2));

	if (feof(wav_fp))
		wav_finish = 1;
	
	for (i = 0; i < samples * 2; i+= 2) {
		/* L channel */
		*p++ = wav[i] & 0xff;
		*p++ = (wav[i] >> 8) & 0xff;
		/* R channel */
		*p++ = wav[i + 1] & 0xff;
		*p++ = (wav[i + 1] >> 8) & 0xff;
	}
	return samples;
}

int
wav_open(char *wav_file)
{
	if ((wav_fp = fopen(wav_file, "rb")) == NULL)
		return 1;
	/* XXX: Skip 44 bytes = typical header size */
	if (fseek(wav_fp, 44, SEEK_SET) != 0) {
		fclose(wav_fp);
		return 1;
	}
	/* XXX: Is this effective? */
	setvbuf(wav_fp, fread_buf, _IOFBF, sizeof(fread_buf));
	return 0;
}

int
wav_close(void)
{
	return fclose(wav_fp);
	wav_fp = NULL;
}

void
usage(void)
{
	printf("Usage: %s [options] wavfile.wav\n", getprogname());
	printf("\t-d	: debug flag\n");
	printf("\t-r #	: sampling rate\n");
	printf("\twavfile must be LE, 16bit, stereo\n");
	exit(1);
}
