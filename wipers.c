#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#define CLOCK_PRESCALE (_BV(CLKPS3))                                   // divide by 256 -> 64.45 kHz
#define TIMER_PRESCALE (_BV(CS10) | _BV(CS11) | _BV(CS12) | _BV(CS13)) // divide by 16384 -> 3.93 Hz -> 0.25s

#define DEFAULT_PERIOD (30) // 30 timer ticks -> 7.5 seconds
#define MINIMUM_PERIOD (10) // 10 timer ticks -> 2.5 seconds
#define PULSE_DURATION  (2) //  2 timer ticks -> 0.5 seconds

int main() {
	cli();                  // disable interrupts
	DDRB  = _BV(DDB1);      // set PB1 as digital output, all others as inputs
	PORTB = _BV(PORTB2) |   // enable internal pullup on PB2
	        _BV(PORTB5);    // enable internal pullup on PB5
	GIMSK = _BV(PCIE);      // enable pin-change interrupts
	PCMSK = _BV(PCINT0);    // enable pin-change interrupt on PB0
	TCNT1 = DEFAULT_PERIOD; // set default period for first use
	OCR1A = PULSE_DURATION; // set PWM pulse duration
	PRR   = _BV(PRADC) |    // power down ADC
	        _BV(PRUSI) |    // power down USI
	        _BV(PRTIM0);    // power down TIMER0
	ACSR  = _BV(ACD);       // disable analog comparator
	MCUCR = _BV(SE) |       // enable sleep
	        _BV(SM1);       // set sleep mode to power-down
	CLKPR = _BV(CLKPCE);    // enable clock prescale change
	CLKPR = CLOCK_PRESCALE; // set clock prescale
	sei();                  // enable interrupts
	for (;;)                // sleep when not handling interrupts
		asm volatile("sleep");
}

// pin-change ISR for PB0
ISR(PCINT0_vect) {
	TCCR1 = 0; // disable timer
	if (PINB & _BV(PINB0)) {
		// PB0 high => intermittent wiper switch released
		PORTB &= ~_BV(PORTB1);  // turn off PB1
		TIFR  = _BV(TOV1);      // clear timer overflow
		TIMSK = _BV(TOIE1);     // enable timer overflow interrupt
		TCCR1 = TIMER_PRESCALE; // start timer with prescale
	} else {
		// PB0 low => intermittent wiper switch engaged
		PORTB |= _BV(PORTB1);   // turn on PB1
		uint8_t period = TCNT1; // get timed period
		if (period < MINIMUM_PERIOD)
			period = MINIMUM_PERIOD;
		OCR1C = period;         // set PWM period
		TCNT1 = 0;              // clear timer counter
		TIMSK = 0;              // disable timer overflow interrupt
		GTCCR = _BV(PSR1);      // reset timer prescaler
		TCCR1 = _BV(PWM1A) |    // set PWM mode
		        _BV(COM1A1) |   // set PWM output polarity
		        TIMER_PRESCALE; // start PWM with prescale
	}
	MCUCR = _BV(SE); // enable sleep, set sleep mode to idle
}

// timer overflow ISR
ISR(TIM1_OVF_vect) {
	// timer overflow => maximum timed period exceeded
	TCCR1 = 0;              // disable timer
	TIMSK = 0;              // disable timer overflow interrupt
	TCNT1 = DEFAULT_PERIOD; // set default PWM period
	MCUCR = _BV(SE) |       // enable sleep
	        _BV(SM1);       // set sleep mode to power-down
}
