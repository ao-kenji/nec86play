/*
 * NEC PC-9801-86 sound board test on OpenBSD/luna88k
 */

/* Originally came from: */
/*	$NecBSD: nec86hwvar.h,v 1.10 1998/03/14 07:04:55 kmatsuda Exp $	*/

/*
 * [NetBSD for NEC PC-98 series]
 *  Copyright (c) 1996, 1997, 1998
 *	NetBSD/pc98 porting staff. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * nec86hwvar.h
 *
 * NEC PC-9801-86 SoundBoard PCM driver for NetBSD/pc98.
 * Written by NAGAO Tadaaki, Feb 10, 1996.
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
void	nec86hw_enable_fifointr(void);
void	nec86hw_disable_fifointr(void);
int	nec86hw_seeif_intrflg(void);
void	nec86hw_clear_intrflg(void);
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
extern u_int8_t *ym2608reg;
extern u_int8_t *ym2608data;
extern int	nec86fd;
extern int	nec86intlevel;
