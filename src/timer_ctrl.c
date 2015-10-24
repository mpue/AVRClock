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
 * timer_ctrl.c
 *
 *  Created on: 23.02.2013
 *      Author: Püski
 */
#include <avr/io.h>
#include "timer_ctrl.h"

void timer1_start_normal(void) {
	TCCR1B = (0 << CS12) | (0 << CS11) | (1 << CS10); // no prescaler
}
void timer1_stop(void) {
	TCCR1B = (0 << CS12) | (0 << CS11) | (0 << CS10); // stop timer
}

void timer1_stop_ctc(void) {
	TCCR1B = (0 << CS12) | (0 << CS11) | (0 << CS10); // stop timer
	TIMSK1 &= ~(1 << OCIE1A); // disable ctc interrupt
}

void timer1_start_ctc(uint16_t cmp) {
	OCR1A = cmp; // set value to output compare register
	TCCR1B = (1 << WGM12) | (1 << CS12) | (0 << CS11) | (0 << CS10); // ctc, 256 prescale
	//Enable the Output Compare A interrupt
	TIMSK1 |= (1 << OCIE1A);
}

void timer0_start_normal(void) {
	TCCR0B = (0 << CS02) | (0 << CS01) | (1 << CS00); //  no prescaler
}

void timer0_stop(void) {
	TCCR0B = (0 << CS02) | (0 << CS01) | (0 << CS00); // stop
}

void timer0_reset(void) {
	TCNT0 = 0;
}

void timer0_enable_overflow_isr(void) {
	TIMSK0 |= (1 << TOIE0);
}

void timer0_disable_overflow_isr(void) {
	TIMSK0 &= ~(1 << TOIE0);
}
