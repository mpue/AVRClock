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
 * int_ctrl.h
 *
 *  Created on: 23.02.2013
 *      Author: Püski
 */

#ifndef INT_CTRL_H_
#define INT_CTRL_H_

/**
 * Select any edge for INT1 pin
 */
void int1_select_any_edge(void);
/**
 * Select rising edge for INT1 pin
 */
void int1_select_rising_edge(void);
/**
 *  trigger on falling edge for INT1
 */
void int1_select_falling_edge(void);
/**
 * Turn INT1 off
 */
void int1_enable(void);
/**
 * Turn INT1 on
 */
void int1_disable(void);


#endif /* INT_CTRL_H_ */
