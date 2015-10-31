/*
 * buttons.h
 *
 *  Created on: 29.12.2014
 *      Author: mpue
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef BUTTONS_H_
#define BUTTONS_H_

#define KEY_DDR         DDRD
#define KEY_PORT        PORTD
#define KEY_PIN         PIND
#define KEY0            0
#define KEY1            1
#define KEY2            2
#define ALL_KEYS        (1<<KEY0 | 1<<KEY1 | 1<<KEY2)

#define REPEAT_MASK     (1<<KEY0 | 1<<KEY1 | 1<<KEY2)       // repeat: key1, key2
#define REPEAT_START    100                        // after 500ms
#define REPEAT_NEXT     20                        // every 200ms

volatile uint8_t key_state;                                // debounced and inverted key state:
                                                  // bit = 1: key pressed
volatile uint8_t key_press;                                // key press detect

volatile uint8_t key_rpt;


ISR( TIMER2_OVF_vect );

///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed. Each pressed key is reported
// only once
//
uint8_t get_key_press( uint8_t key_mask );
///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed long enough such that the
// key repeat functionality kicks in. After a small setup delay
// the key is reported being pressed in subsequent calls
// to this function. This simulates the user repeatedly
// pressing and releasing the key.
//
uint8_t get_key_rpt( uint8_t key_mask );
///////////////////////////////////////////////////////////////////
//
// check if a key is pressed right now
//
uint8_t get_key_state( uint8_t key_mask );
///////////////////////////////////////////////////////////////////
//
uint8_t get_key_short( uint8_t key_mask );
///////////////////////////////////////////////////////////////////
//
uint8_t get_key_long( uint8_t key_mask );


#endif /* BUTTONS_H_ */
