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
 * int_ctrl.c
 *
 *  Created on: 23.02.2013
 *      Author: Püski
 */
#include <avr/io.h>
#include "int_ctrl.h"

void int1_select_falling_edge(void) {
	EICRA |= (1 << ISC11);
	EICRA &= ~(1 << ISC10);
	EIFR |= (1 << INTF1); // clear external interrupt flag
}

void int1_select_rising_edge(void) {
	EICRA |= (1 << ISC10);
	EICRA |= (1 << ISC11);
	EIFR |= (1 << INTF1); // clear external interrupt flag
}

void int1_select_any_edge(void) {
	EICRA |= (1 << ISC10);
	EICRA &= ~(1 << ISC11);
	EIFR |= (1 << INTF1); // clear external interrupt flag
}

void int1_enable(void) {
	EIMSK |= 1 << INT1;
}

void int1_disable(void) {
	EIMSK &= ~(1 << INT1);
}

