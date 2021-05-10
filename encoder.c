#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>


#include "project.h"
#include "encoder.h"

extern volatile unsigned char new_state, old_state;
extern volatile unsigned char changed;  // Flag for state change
extern volatile int count;		// Count to display
extern volatile unsigned char a, b;
extern char state;
extern int freq_index;

void encoder_init(void) {
	PORTB |= (1 << PB3) | (1 << PB4);
	PCICR |= (1 << PCIE0);
	PCMSK0 |= (1 << PCINT3);
	PCMSK0 |= (1 << PCINT4);
	changed = 0;
	sei();
	
	unsigned char tmp = PINB & 0xff;
    a = tmp & (1 << 1);
	b = tmp & (1 << 5);
	
	if (!b && !a)
		old_state = 0;
    else if (!b && a)
		old_state = 1;
    else if (b && !a)
		old_state = 2;
    else
		old_state = 3;

    new_state = old_state;
}



ISR(PCINT0_vect) {
	unsigned char tmp = PINB & 0xff;
    a = tmp & (1 << 3);
	b = tmp & (1 << 4);
	
	if (old_state == 0) {
	    // Handle A and B inputs for state 0
		if (a != 0) {
			new_state = 1;
			if (state == 0) {
				count--;
			}
			else {
				freq_index--;
			}
		}
		else if (b != 0) {
			new_state = 3;
			if (state == 0) {
				count++;
			}
			else {
				freq_index++;
			}
		}
	}
	else if (old_state == 1) {
	    // Handle A and B inputs for state 1
	    if (b != 0) {
	    	new_state = 2;
	    	if (state == 0) {
				count--;
			}
			else {
				freq_index--;
			}
	    }
	    else if (a == 0) {
	    	new_state = 0;
			if (state == 0) {
				count++;
			}
			else {
				freq_index++;
			}
	    }
	}
	else if (old_state == 2) {
	    // Handle A and B inputs for state 2
	    if (a == 0) {
	    	new_state = 3;
			if (state == 0) {
				count--;
			}
			else {
				freq_index--;
			}
	    }
	    else if (b == 0) {
	    	new_state = 1;
			if (state == 0) {
				count++;
			}
			else {
				freq_index++;
			}
	    }
	}
	else {   // old_state = 3
	    // Handle A and B inputs for state 3
	    if (a != 0) {
	    	new_state = 2;
			if (state == 0) {
				count++;
			}
			else {
				freq_index++;
			}
	    }
	    else if (b == 0) {
	    	new_state = 0;
			if (state == 0) {
				count--;
			}
			else {
				freq_index--;
			}
	    }
	}

	// If state changed, update the value of old_state,
	// and set a flag that the state has changed.
	if (new_state != old_state) {
	    changed = 1;
	    old_state = new_state;
	    if (count < 60) {
	    	count = 60;
	    }
	    if (count > 100) {
	    	count = 100;
	    }
	    if (freq_index > 24) {
	    	freq_index = 24;
	    }
	    if (freq_index < 0) {
	    	freq_index = 0;
	    }
	    eeprom_update_byte((void *) 100, freq_index);
		eeprom_update_byte((void *) 200, count);
	}
}