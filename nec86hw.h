/*
 * NEC PC-9801-86 sound board test on OpenBSD/luna88k
 *
 * Note:
 * If you want to write data to the mmapped device, you must change securelevel
 * to -1 in /etc/rc.securelevel and reboot your system.
 */

#include "nec86reg.h"
#include "nec86hwvar.h"

#define SPKR_ON         1
#define SPKR_OFF        0
#define VOLUME_DELAY	10	/* ms */
#define delay(ms)	usleep(ms)

extern int nec86hw_rate_table[NEC86_NRATE];

int	nec86_open(void);
void	nec86_close(void);
int	nec86hw_init(void);
void	nec86hw_set_volume(int, u_int8_t);
int	nec86hw_speaker_ctl(int);
void	nec86hw_set_mode_playing(void);
void	nec86hw_set_mode_recording(void);
void	nec86hw_start_fifo(void);
void	nec86hw_stop_fifo(void);
void	nec86hw_reset_fifo(void);
void	nec86hw_set_precision_real(u_int);
void	nec86hw_set_rate_real(u_int8_t);
u_int8_t nec86hw_rate_bits(u_long);
int	nec86fifo_output_stereo_16_direct(u_int8_t *, int);
int	nec86hw_seeif_fifo_full(void);
int	nec86hw_seeif_fifo_empty(void);
void	nec86hw_dump_register(void);

extern u_int8_t *pc98iobase;
extern u_int8_t *nec86base;
extern u_int8_t *nec86core;
extern int	nec86fd;
