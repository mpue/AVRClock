/**

	ATMega and TSIC temperature measurement

	Written and maintained by Matthias Pueski

	Copyright (c) 2013 Matthias Pueski

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
 * timer_ctrl.h
 *
 *  Created on: 23.02.2013
 *      Author: Püski
 */

#ifndef TIMER_CTRL_H_
#define TIMER_CTRL_H_

/**
 * Start timer1 in normal mode
 */
void timer1_start_normal(void);
/**
 * Stop timer1
 */
void timer1_stop(void);
/**
 * Stop timer1 from ctc mode
 */
void timer1_stop_ctc(void);
/**
 * Start timer1 in ctc mode and load the output compare register (OCR1A)
 */
void timer1_start_ctc(uint16_t cmp);

void timer0_start_normal(void);

void timer0_stop(void);

void timer0_reset(void);

void timer0_enable_overflow_isr(void);

void timer0_disable_overflow_isr(void);


#endif /* TIMER_CTRL_H_ */
