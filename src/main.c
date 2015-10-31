#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "int_ctrl.h"
#include "timer_ctrl.h"
#include "buttons.h"
#include "typedefs.h"

#define setPin(PORT,PIN) PORT |= (1 << PIN)
#define clearPin(PORT,PIN) PORT &= ~(1 << PIN)

#define SEG_A clearPin(PORTB, 4)
#define SEG_B clearPin(PORTC, 5)
#define SEG_C clearPin(PORTC, 3)
#define SEG_D clearPin(PORTC, 2)
#define SEG_E clearPin(PORTB, 2)
#define SEG_F clearPin(PORTB, 3)
#define SEG_G clearPin(PORTC, 4)

#define SW_DELAY 2

volatile uint8_t al_hour = 0;
volatile uint8_t al_minute = 0;
volatile uint8_t al_second = 0;

volatile boolean withDot = false;
volatile boolean alarmEnable = false;
volatile boolean alarmRunning = false;

volatile boolean sync = false;
volatile boolean current_bit_high = false;

volatile uint8_t current_bit = 0;

volatile date_t tmp_date;
volatile date_t current_date;
volatile date_t dcf_date;

volatile uint32_t time = 0;
volatile boolean clk_run = false;
volatile uint32_t current_bit_time = 0;

enum Mode {
	TIME,
	ALARM
};

volatile enum Mode displayMode = TIME;

void eval_dcf(void) {

	if (current_bit >= 21 && current_bit <= 27) {

		if (current_bit >= 21 && current_bit <= 24) {
			tmp_date.minute |= current_bit_high << (current_bit - 21);
		}
		else if (current_bit == 25) {
			if (current_bit_high)
				tmp_date.minute += 10;
		}
		else if (current_bit == 26) {
			if (current_bit_high)
				tmp_date.minute += 20;
		}
		else if (current_bit == 27) {
			if (current_bit_high)
				tmp_date.minute += 40;
		}

	}
	else if (current_bit >= 29 && current_bit <= 34) {

		if (current_bit >= 29 && current_bit <= 32) {
			tmp_date.hour |= current_bit_high << (current_bit - 29);
		}
		else if (current_bit == 33) {
			if (current_bit_high)
				tmp_date.hour += 10;
		}
		else if (current_bit == 34) {
			if (current_bit_high)
				tmp_date.hour += 20;
		}

	}
	else if (current_bit >= 36 && current_bit <= 41) {
		if (current_bit >= 36 && current_bit <= 39) {
			tmp_date.day |= current_bit_high << (current_bit - 36);
		}
		else if (current_bit == 40) {
			if (current_bit_high)
				tmp_date.day += 10;
		}
		else if (current_bit == 41) {
			if (current_bit_high)
				tmp_date.day += 20;
		}

	}
	else if (current_bit >= 42 && current_bit <= 44) {
		tmp_date.day_of_week |= current_bit_high << (current_bit - 42);
	}
	else if (current_bit >= 45 && current_bit <= 49) {
		if (current_bit >= 45 && current_bit <= 48) {
			tmp_date.month |= current_bit_high << (current_bit - 45);
		}
		else if (current_bit == 49) {
			if (current_bit_high)
				tmp_date.month += 10;
		}
	}
	else if (current_bit >= 50 && current_bit <= 57) {

		if (current_bit >= 50 && current_bit <= 53) {
			tmp_date.year |= current_bit_high << (current_bit - 50);
		}
		else if (current_bit == 54) {
			if (current_bit_high)
				tmp_date.year += 10;
		}
		else if (current_bit == 55) {
			if (current_bit_high)
				tmp_date.year += 20;
		}
		else if (current_bit == 56) {
			if (current_bit_high)
				tmp_date.year += 40;
		}
		else if (current_bit == 57) {
			if (current_bit_high)
				tmp_date.year += 80;
		}

	}

}

ISR (TIMER0_OVF_vect) {
	time++;
}

ISR (TIMER1_COMPA_vect) {

	if (current_date.second < 59) {
		current_date.second++;
		if (displayMode == TIME) {
			withDot = !withDot;
		}
		else {
			withDot = true;
		}
	}
	else {
		current_date.second = 0;

		if (current_date.minute < 59) {
			current_date.minute++;
		}
		else {
			current_date.minute = 0;

			if (current_date.hour < 23) {
				current_date.hour++;
			}
			else {
				current_date.hour = 0;
			}
		}
	}

	if (al_hour == current_date.hour && al_minute == current_date.minute && al_second == current_date.second) {
		alarmRunning = true;
	}

}

ISR (INT1_vect) {

	EIFR |= (1 << INTF1); // clear external interrupt flag

	/**
	 * If we are in sync, we stop the timer each rising edge
	 * and check how long it took from the last edge
	 */
	if (!sync) {
		timer0_stop();

		float f_time = ((float) (1 / ((float) F_CPU / 256))) * time;

		/**
		 * Took longer than 1.5 seconds, thus it must be the beginning
		 * of a new time frame. Houston we are in sync
		 */
		if (f_time > 1.5) {
			sync = true;
			clk_run = true;
			// now we need to detect any edge and measure how long it takes
			int1_select_any_edge();
		}

		/**
		 * Now reset the timer and start it normal
		 */
		timer0_reset();
		time = 0;
		timer0_start_normal();
	}

	if (sync) {
		/**
		 * Currently we are in sync,
		 */
		if (current_bit < 58) {
			// rising edge
			if (PIND & (1 << PIND3)) {
				time = 0;
				timer0_reset();
				timer0_start_normal();
			}

			else {
				/**
				 *  falling edge, now stop timer0 and check how long  it took, if it was longer than
				 *  8000 ticks we have a 1 and 0 else
				 */
				timer0_stop();
				current_bit_time = time;
				// low value means a low carrier for 100ms +/- 20ms and high means low carrier for 200ms +/- 40
				// thus we must check if the time is larger than 120ms.
				// the timer0 overflow happens every 32us that means the counter must overflow at least 3750 times
				// with a crystal frequency of MHz. To be abolutely sure, we take a little bit more time.
				if (time > 4000) {
					current_bit_high = true;
				}
				else {
					current_bit_high = false;
				}
				/**
				 * Measurement complete eval dcf time
				 */
				eval_dcf();
				current_bit++;
			}
		}
		else {
			/**
			 * Time frame complete, switch INT1 back to rising edge
			 * copy properties, clear all temp values and set sync flag to false;
			 */
			int1_select_rising_edge();
			current_bit = 0;
			current_date.second = 0;
			current_date.minute = tmp_date.minute;
			tmp_date.minute = 0;
			current_date.hour = tmp_date.hour;
			tmp_date.hour = 0;
			dcf_date.day = tmp_date.day;
			dcf_date.month = tmp_date.month;
			dcf_date.year = tmp_date.year;
			dcf_date.day_of_week = tmp_date.day_of_week;
			tmp_date.day_of_week = 0;
			tmp_date.day = 0;
			tmp_date.month = 0;
			tmp_date.year = 0;
			sync = false;
		}

	}
}

void displayNumber(uint8_t num, boolean withDot) {


	// turn all segments off
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

void displayTime(int hours, int minutes, int seconds, boolean dot) {

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
	TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // divide by 1024
	TCNT2 = (uint8_t) (int16_t) -(F_CPU / 1024 * 10e-3 + 0.5); // preload for 10ms
	TIMSK2 |= 1 << TOIE2; // enable timer interrupt
}

int main(void) {

	DDRB = 0xFF;
	DDRC = 0xFF;

	// Alarm switch config
	clearPin(DDRD,4);
	// pull-up for alarm switch
	setPin(PORTD,4);

	setPin(DDRD,5);
	setPin(DDRD,6);

	// alarm trigger pin
	setPin(DDRD,7);

	initKeys();

	int1_enable();
	int1_select_rising_edge();

	sei();

	timer1_start_ctc(31250);
	timer0_enable_overflow_isr();
	timer0_start_normal();

	while (1) {

		if (PIND & (1<<PD4)) {
			alarmEnable = true;
			setPin(PORTB,0);
		}
		else {
			alarmEnable = false;
			alarmRunning = false;
			clearPin(PORTB,0);
			clearPin(PORTD,7);
		}

		if (displayMode == TIME) {
			displayTime(current_date.hour,current_date.minute,current_date.second,withDot);
			// displayTime(hour,minute,second,withDot);
		}
		else {
			displayTime(al_hour,al_minute,al_second,withDot);
		}

		if (get_key_rpt(1 << KEY0) || get_key_short(1 << KEY0)) {

			if (displayMode == TIME) {
				if (current_date.hour < 23) {
					current_date.hour++;
				}
				else {
					current_date.hour = 0;
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
				if (current_date.minute < 59) {
					current_date.minute++;
				}
				else {
					current_date.minute = 0;
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
