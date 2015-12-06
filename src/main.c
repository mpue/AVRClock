#define F_CPU 8000000UL

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "int_ctrl.h"
#include "timer_ctrl.h"
#include "buttons.h"
#include "typedefs.h"
#include "uart.h"

#define CHECK_INTERVAL_SECONDS 60
#define UART_BAUD_RATE      9600

// maximum command length from serial
#define MAX_LEN 16

// #define DCF77_ENABLED

#define setPin(PORT,PIN) PORT |= (1 << PIN)
#define clearPin(PORT,PIN) PORT &= ~(1 << PIN)

#define SEG_A clearPin(PORTB, 5)
#define SEG_B clearPin(PORTC, 1)
#define SEG_C clearPin(PORTB, 2)
#define SEG_D clearPin(PORTB, 1)
#define SEG_E clearPin(PORTB, 3)
#define SEG_F clearPin(PORTB, 4)
#define SEG_G clearPin(PORTC, 0)

#define SEG_A_OFF setPin(PORTB, 5)
#define SEG_B_OFF setPin(PORTC, 1)
#define SEG_C_OFF setPin(PORTB, 2)
#define SEG_D_OFF setPin(PORTB, 1)
#define SEG_E_OFF setPin(PORTB, 3)
#define SEG_F_OFF setPin(PORTB, 4)
#define SEG_G_OFF setPin(PORTC, 0)

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

volatile uint32_t time = 0;
volatile boolean clk_run = false;
volatile uint32_t current_bit_time = 0;

volatile uint8_t checkInterval = 0;

char buffer[16];
uint8_t charNum = 0;

char num[3];

boolean timeReceived = false;

enum Mode {
	TIME,
	ALARM,
	DATE
};

volatile enum Mode displayMode = TIME;

boolean isValidTime(date_t time) {

	if (time.hour < 24 && time.minute < 60 && time.second < 60 &&
		time.day < 32 && time.month < 13)
		return true;

	return false;
}

#ifdef DCF77_ENABLED

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

#endif

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

#ifdef DCF77_ENABLED
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

			// set the time only if a valid time has been received
			if (isValidTime(tmp_date)) {
				current_date.second = 0;
				current_date.minute = tmp_date.minute;
				current_date.hour = tmp_date.hour;
				current_date.day = tmp_date.day;
				current_date.month = tmp_date.month;
				current_date.year = tmp_date.year;
				current_date.day_of_week = tmp_date.day_of_week;
				timeReceived = true;
			}

			tmp_date.minute = 0;
			tmp_date.hour = 0;
			tmp_date.day_of_week = 0;
			tmp_date.day = 0;
			tmp_date.month = 0;
			tmp_date.year = 0;

			// now resync
			sync = false;
		}

	}

}
#endif

void displayNumber(uint8_t num, boolean withDot) {

	// turn all segments off
	SEG_A_OFF;
	SEG_B_OFF;
	SEG_C_OFF;
	SEG_D_OFF;
	SEG_E_OFF;
	SEG_F_OFF;
	SEG_G_OFF;

	if (withDot) {
		clearPin(PORTB, 0);
	}
	else {
		setPin(PORTB,0);
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

	clearPin(PORTC,2);
	clearPin(PORTC,3);
	clearPin(PORTC,4);
	setPin(PORTC,5);
	displayNumber(m_ones,false);
	_delay_ms(SW_DELAY);

	clearPin(PORTC,2);
	clearPin(PORTC,3);
	setPin(PORTC,4);
	clearPin(PORTC,5);
	displayNumber(m_tenths,false);
	_delay_ms(SW_DELAY);

	clearPin(PORTC,2);
	setPin(PORTC,3);
	clearPin(PORTC,4);
	clearPin(PORTC,5);
	displayNumber(h_ones,dot);
	_delay_ms(SW_DELAY);

	setPin(PORTC,2);
	clearPin(PORTC,3);
	clearPin(PORTC,4);
	clearPin(PORTC,5);
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

void parseCommand(char* cmd, uint8_t len) {

	if (strncmp(cmd,"time",4) == 0) {

		char* t = &buffer[5];

		uint16_t time   = atoi(t);

		int hour = time / 100;
		int minute = time - ((time / 100) * 100);

		tmp_date.hour = hour;
		tmp_date.minute = minute;
		tmp_date.month = 1;
		tmp_date.year = 0;
		tmp_date.day = 1;

		if (isValidTime(tmp_date)) {
			current_date.hour = hour;
			current_date.minute = minute;
		}

	}
	else if(strncmp(cmd,"gettime",7) == 0) {
		sprintf(buffer,"\r\n%02d:%02d",current_date.hour,current_date.minute);
		uart_puts(buffer);
	}

}


void getUart() {
    /*
     * Get received character from ringbuffer
     * uart_getc() returns in the lower byte the received character and
     * in the higher byte (bitmask) the last receive error
     * UART_NO_DATA is returned when no data is available.
     *
     */
    char c = uart_getc();

    if ( c & UART_NO_DATA )
    {
        /*
         * no data available from UART
         */
    }
    else
    {
        /*
         * new data available from UART
         * check for Frame or Overrun error
         */
        if ( c & UART_FRAME_ERROR )
        {
            /* Framing Error detected, i.e no stop bit detected */
            uart_puts_P("UART Frame Error: ");
        }
        if ( c & UART_OVERRUN_ERROR )
        {
            /*
             * Overrun, a character already present in the UART UDR register was
             * not read by the interrupt handler before the next character arrived,
             * one or more received characters have been dropped
             */
            uart_puts_P("UART Overrun Error: ");
        }
        if ( c & UART_BUFFER_OVERFLOW )
        {
            /*
             * We are not reading the receive buffer fast enough,
             * one or more received character have been dropped
             */
            uart_puts_P("Buffer overflow error: ");
        }

        if (charNum < 16) {

            if ((unsigned char)c == '\r') {

            	parseCommand(buffer,charNum);

            	uart_puts("\r\n");
            	uart_puts("Ok.\r\n");

            	charNum = 0;
            }
            else {
            	if (isalnum(c) || isblank(c)) {
					uart_putc(c);
					buffer[charNum] = (unsigned char)c;
					charNum++;
            	}
            }


        }

    }

}

int main(void) {

	charNum = 0;

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

#ifdef EXTERNAL_BUTTONS
	initKeys();
#endif

#ifdef DCF77_ENABLED
	int1_enable();
	int1_select_rising_edge();
#endif

    /*
     *  Initialize UART library, pass baudrate and AVR cpu clock
     *  with the macro
     *  UART_BAUD_SELECT() (normal speed mode )
     *  or
     *  UART_BAUD_SELECT_DOUBLE_SPEED() ( double speed mode)
     */
    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );

	sei();

	timer1_start_ctc(31250);

#ifdef DCF77_ENABLED
	timer0_enable_overflow_isr();
	timer0_start_normal();
#endif

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
		}
		else if (displayMode == ALARM){
			displayTime(al_hour,al_minute,al_second,withDot);
		}
#ifdef DCF77_ENABLED
		else {
			displayTime(current_date.day,current_date.month,current_date.year,true);
		}
#endif

#ifdef EXTERNAL_BUTTONS
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
		else if(get_key_short(1 << KEY2)) {
			if (displayMode == TIME) {
				displayMode = ALARM;
			}
#ifdef DCF77_ENABLED
			else if (displayMode == ALARM){
				displayMode = DATE;
			}
#endif
			else {
				displayMode = TIME;
			}
		}
#endif
		getUart();

		if (alarmEnable && alarmRunning) {
			setPin(PORTD,7);
			_delay_ms(200);
			clearPin(PORTD,7);
			alarmRunning = false;
		}

	}

	return 0;
}
