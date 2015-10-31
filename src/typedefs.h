/*
 * typedefs.h
 *
 *  Created on: 14.06.2013
 *      Author: mpue
 */

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <avr/io.h>

#define true  1
#define false 0

typedef uint8_t boolean;

typedef struct {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t day;
	uint8_t day_of_week;
	uint8_t month;
	uint8_t year;
} date_t;

#endif /* TYPEDEFS_H_ */
