#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "timer_ctrl.h"
#include "buttons.h"

#define setPin(PORT,PIN) PORT |= (1 << PIN)
#define clearPin(PORT,PIN) PORT &= ~(1 << PIN)

typedef uint8_t bool;

#define true 1
#define false 0

#define SEG_A clearPin(PORTB, 4)
#define SEG_B clearPin(PORTC, 5)
#define SEG_C clearPin(PORTC, 3)
#define SEG_D clearPin(PORTC, 2)
#define SEG_E clearPin(PORTB, 2)
#define SEG_F clearPin(PORTB, 3)
#define SEG_G clearPin(PORTC, 4)

#define SW_DELAY 2

volatile uint8_t hour = 0;
volatile uint8_t minute = 0;
volatile uint8_t second = 0;

volatile uint8_t al_hour = 0;
volatile uint8_t al_minute = 0;
volatile uint8_t al_second = 0;

volatile bool withDot = false;
volatile bool alarmEnable = false;
volatile bool alarmRunning = false;

enum Mode {
	TIME,
	ALARM
};

volatile enum Mode displayMode = TIME;

ISR (TIMER1_COMPA_vect) {

	if (second < 59) {
		second++;
		if (displayMode == TIME) {
			withDot = !withDot;
		}
		else {
			withDot = true;
		}
	}
	else {
		second = 0;

		if (minute < 59) {
			minute++;
		}
		else {
			minute = 0;

			if (hour < 23) {
				hour++;
			}
			else {
				hour = 0;
			}
		}
	}

	if (al_hour == hour && al_minute == minute && al_second == second) {
		alarmRunning = true;
	}

}

void displayNumber(uint8_t num, bool withDot) {

	setPin(PORTC, 5);
	setPin(PORTC, 4);
	setPin(PORTC, 3);
	setPin(PORTC, 2);
	setPin(PORTB, 4);
	setPin(PORTB, 3);
	setPin(PORTB, 2);
	setPin(PORTB, 1);


	if (withDot) {
		clearPin(PORTB, 1);
	}
	else {
		setPin(PORTB,1);
	}

	if (num == 0) {
		SEG_A;
		SEG_B;
		SEG_C;
		SEG_D;
		SEG_E;
		SEG_F;
	}
	else if (num == 1) {
		SEG_B;
		SEG_C;
	}
	else if (num == 2) {
		SEG_A;
		SEG_B;
		SEG_G;
		SEG_D;
		SEG_E;
	}
	else if (num == 3) {
		SEG_A;
		SEG_B;
		SEG_C;
		SEG_D;
		SEG_G;
	}
	else if (num == 4) {
		SEG_B;
		SEG_C;
		SEG_F;
		SEG_G;
	}
	else if (num == 5) {
		SEG_A;
		SEG_F;
		SEG_G;
		SEG_C;
		SEG_D;
	}
	else if (num == 6) {
		SEG_A;
		SEG_F;
		SEG_G;
		SEG_C;
		SEG_D;
		SEG_E;
	}
	else if (num == 7) {
		SEG_A;
		SEG_B;
		SEG_C;
	}
	else if (num == 8) {
		SEG_A;
		SEG_B;
		SEG_C;
		SEG_D;
		SEG_E;
		SEG_F;
		SEG_G;
	}
	else if (num == 9) {
		SEG_A;
		SEG_B;
		SEG_C;
		SEG_D;
		SEG_F;
		SEG_G;
	}

}

void displayTime(int hours, int minutes, int seconds, bool dot) {

	int s_tenths = seconds / 10;
	int s_ones = seconds - s_tenths * 10;

	int m_tenths = minutes / 10;
	int m_ones = minutes - m_tenths * 10;

	int h_tenths = hours / 10;
	int h_ones = hours - h_tenths * 10;

	clearPin(PORTD,5);
	clearPin(PORTD,6);
	clearPin(PORTB,6);
	setPin(PORTB,7);
	displayNumber(m_ones,false);
	_delay_ms(SW_DELAY);

	clearPin(PORTD,5);
	clearPin(PORTD,6);
	setPin(PORTB,6);
	clearPin(PORTB,7);
	displayNumber(m_tenths,false);
	_delay_ms(SW_DELAY);

	clearPin(PORTB,6);
	clearPin(PORTB,7);
	clearPin(PORTD,5);
	setPin(PORTD,6);
	displayNumber(h_ones,dot);
	_delay_ms(SW_DELAY);

	clearPin(PORTB,6);
	clearPin(PORTB,7);
	setPin(PORTD,5);
	clearPin(PORTD,6);
	displayNumber(h_tenths,false);
	_delay_ms(SW_DELAY);
}

void initKeys(void) {
	// Configure debouncing routines
	KEY_DDR &= ~ALL_KEYS; // configure key port for input
	KEY_PORT |= ALL_KEYS; // and turn on pull up resistors
	TCCR0B = (1 << CS02) | (0 << CS01) | (1 << CS00); // divide by 1024
	TCNT0 = (uint8_t) (int16_t) -(F_CPU / 1024 * 10e-3 + 0.5); // preload for 10ms
	TIMSK0 |= 1 << TOIE0; // enable timer interrupt
}

int main(void) {

	DDRB = 0xFF;
	DDRC = 0xFF;

	// Alarm LED config
	setPin(DDRD,3);
	// Alarm switch config
	clearPin(DDRD,4);
	// pull-up for alarm switch
	setPin(PORTD,4);

	setPin(DDRD,5);
	setPin(DDRD,6);

	// alarm trigger pin
	setPin(DDRD,7);

	initKeys();

	sei();
	timer1_start_ctc(31250);

	while (1) {

		if (PIND & (1<<PD4)) {
			alarmEnable = true;
			setPin(PORTD,3);
		}
		else {
			alarmEnable = false;
			alarmRunning = false;
			clearPin(PORTD,3);
			clearPin(PORTD,7);
		}

		if (displayMode == TIME) {
			displayTime(hour,minute,second,withDot);
		}
		else {
			displayTime(al_hour,al_minute,al_second,withDot);
		}

		if (get_key_rpt(1 << KEY0) || get_key_short(1 << KEY0)) {

			if (displayMode == TIME) {
				if (hour < 23) {
					hour++;
				}
				else {
					hour = 0;
				}
			}
			else {
				if (al_hour < 23) {
					al_hour++;
				}
				else {
					al_hour = 0;
				}
			}
		}
		else if (get_key_rpt(1 << KEY1) || get_key_short(1 << KEY1)) {
			if (displayMode == TIME) {
				if (minute < 59) {
					minute++;
				}
				else {
					minute = 0;
				}
			}
			else {
				if (al_minute < 59) {
					al_minute++;
				}
				else {
					al_minute = 0;
				}
			}
		}
		else if(get_key_long(1 << KEY2)) {
			if (displayMode == TIME) {
				displayMode = ALARM;
			}
			else {
				displayMode = TIME;
			}
		}

		if (alarmEnable && alarmRunning) {
			setPin(PORTD,7);
		}

	}

	return 0;
}
