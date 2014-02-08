/*
 * NEC PC-9801-86 sound board test on OpenBSD/luna88k
 *
 * Note:
 * If you want to write data to the mmapped device, you must change securelevel
 * to -1 in /etc/rc.securelevel and reboot your system.
 */

#include <stdio.h>
#include <fcntl.h>		/* open(2) */
#include <unistd.h>		/* usleep(3) */
#include <sys/ioctl.h>		/* ioctl(2) */
#include <sys/mman.h>		/* mmap(2) */
#include <sys/types.h>
#include "nec86hw.h"

#define SPKR_ON         1
#define SPKR_OFF        0
#define VOLUME_DELAY	10	/* ms */
#define delay(ms)	usleep(ms)

u_int8_t *pc98iobase;
u_int8_t *nec86base;
u_int8_t *nec86core;
int	nec86fd;

int nec86hw_rate_table[NEC86_NRATE] = {
	44100, 33075, 22050, 16538, 11025, 8269, 5513, 4134
};

/*
 * Open 
 */
int
nec86_open(void)
{
	nec86fd = open("/dev/mem", O_RDWR, 0600);
	if (nec86fd == -1) {
		perror("open");
		return -1;
	}

	pc98iobase = (u_int8_t *)mmap(NULL, 0x10000, PROT_READ | PROT_WRITE,
		MAP_SHARED, nec86fd, 0x91000000);

	if (pc98iobase == MAP_FAILED) {
		perror("mmap");
		close(nec86fd);
		return -1;
	}

	nec86base = pc98iobase + 0xa460;
	nec86core = nec86base  + NEC86_COREOFFSET;	/* == 0xa466 */
	return nec86fd;
}

/*
 * Close
 */
void
nec86_close(void)
{
	munmap((void *)pc98iobase, 0x10000);
	close(nec86fd);
}

/*
 * Initialize
 */
int
nec86hw_init(void)
{
	u_int8_t data;

	/* check the board */
	data = *nec86base;

	switch (data & 0xf0) {
	case 0x40: /* 86 board with I/O port 0x188 */
	case 0x50: /* 86 board with I/O port 0x288 */
		/* disable YM2608 extended function */
		*nec86base = (data & 0xfe);
		break;
	default:
		/* can not find 86 board */
		return -1;	/* can not find 86 board */
	}

#if 0
	/* YM2608 register 0x0e has INT and joystick information */
	*(pc98iobase + 0x0188) = 0x0e;
	data = *(pc98iobase + 0x018a);
#endif

	/* set default gains */
	nec86hw_set_volume(NEC86_VOLUME_PORT_OPNAD, NEC86_MAXVOL);
	nec86hw_set_volume(NEC86_VOLUME_PORT_OPNAI, NEC86_MAXVOL);
	nec86hw_set_volume(NEC86_VOLUME_PORT_LINED, NEC86_MAXVOL);
	nec86hw_set_volume(NEC86_VOLUME_PORT_LINEI, NEC86_MAXVOL);
	nec86hw_set_volume(NEC86_VOLUME_PORT_PCMD,  NEC86_MAXVOL);

	nec86hw_speaker_ctl(SPKR_ON);

	/* set miscellanous stuffs */
	data = *(nec86core + NEC86_CTRL);
	data &= NEC86_CTRL_MASK_PAN | NEC86_CTRL_MASK_PORT;
	data |= NEC86_CTRL_PAN_L | NEC86_CTRL_PAN_R;
	data |= NEC86_CTRL_PORT_STD;
	*(nec86core + NEC86_CTRL) = data;
}

void
nec86hw_set_volume(int port, u_int8_t vol)
{
	*(nec86core + NEC86_VOLUME) = NEC86_VOL_TO_BITS(port, vol);
	delay(VOLUME_DELAY);
}

int
nec86hw_speaker_ctl(int onoff)
{
	switch (onoff) {
	case SPKR_ON:
		*(nec86core + NEC86_VOLUME) = 0x0d1;
		delay(VOLUME_DELAY);
		break;
	case SPKR_OFF:
		*(nec86core + NEC86_VOLUME) = 0x0d0;
		delay(VOLUME_DELAY);
		break;
	default:
		return -1;
	}
	return 0;
}

void
nec86hw_set_mode_playing(void)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOCTL);
	data &= ~NEC86_FIFOCTL_RECMODE;
	*(nec86core + NEC86_FIFOCTL) = data;
}

void
nec86hw_set_mode_recording(void)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOCTL);
	data |= NEC86_FIFOCTL_RECMODE;
	*(nec86core + NEC86_FIFOCTL) = data;
}

void
nec86hw_start_fifo(void)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOCTL);
	data |= NEC86_FIFOCTL_RUN;
	*(nec86core + NEC86_FIFOCTL) = data;
}

void
nec86hw_stop_fifo(void)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOCTL);
	data &= ~NEC86_FIFOCTL_RUN;
	*(nec86core + NEC86_FIFOCTL) = data;
}

void
nec86hw_reset_fifo(void)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOCTL);
	data |= NEC86_FIFOCTL_INIT;
	*(nec86core + NEC86_FIFOCTL) = data;
	data &= ~NEC86_FIFOCTL_INIT;
	*(nec86core + NEC86_FIFOCTL) = data;
}

void
nec86hw_set_precision_real(u_int prec)
{
	u_int8_t data;

	data = *(nec86core + NEC86_CTRL);
	data &= ~NEC86_CTRL_8BITS;
	if (prec == 8)
		data |= NEC86_CTRL_8BITS;
	*(nec86core + NEC86_CTRL) = data;
}

void
nec86hw_set_rate_real(u_int8_t bits)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOCTL);
	data &= ~NEC86_FIFOCTL_MASK_RATE;
	data |= bits & NEC86_FIFOCTL_MASK_RATE;
	*(nec86core + NEC86_FIFOCTL) = data;
}

u_int8_t
nec86hw_rate_bits(u_long sr)
{
	int i;
	u_long rval, hr, min;

	min = 0;
	rval = 0;
	for (i = 0; i < NEC86_NRATE; i++) {
		hr = nec86hw_rate_table[i];
		if ((hr >= sr) && ((min == 0) || (min > hr))) {
			min = hr;
			rval = (u_int8_t) i;
		}
	}
	return rval;
}

int
nec86fifo_output_stereo_16_direct(u_int8_t *p, int cc)
{
	int i;

	for (i = 0; i < cc; i++) {
		/* L channel */
		*(nec86core + NEC86_FIFODATA) = *p;
		*(nec86core + NEC86_FIFODATA) = *(p + 1);
		p += 2;
		/* R channel */
		*(nec86core + NEC86_FIFODATA) = *p;
		*(nec86core + NEC86_FIFODATA) = *(p + 1);
		p += 2;
	}
	return cc * 4;
}

int
nec86hw_seeif_fifo_full(void)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOSTAT);
	return (data & NEC86_FIFOSTAT_FULL);
}

int
nec86hw_seeif_fifo_empty(void)
{
	u_int8_t data;

	data = *(nec86core + NEC86_FIFOSTAT);
	return (data & NEC86_FIFOSTAT_EMPTY);
}

void
nec86hw_dump_register(void)
{
	int i;
	u_int8_t data;

	printf("%04x ", 0xa460);
	for (i = 0; i < 16; i++) {
		data = *(nec86base + i);
		printf("%02x ", data);
	}
	printf("\n");
}
