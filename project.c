/********************************************
 *
 *  Name: Carson Mogush
 *  Email: cmogush@usc.edu
 *  Section: Wed 12:30pm Weber
 *  Assignment: Final Project
 *
 ********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>


#include "lcd.h"
#include "ds18b20.h"
#include "encoder.h"
#include "project.h"

//states for what encoder should refer too 0 = threshhold, 1 = note

enum TMPSTATES {cool, warm, hot};

volatile unsigned char new_state, old_state;
volatile unsigned char changed;  // Flag for state change
volatile int count;		// Count to display
volatile unsigned char a, b;
long temp;
volatile int tmpColor = 0;
char state;
char tmpState;
int freq_index;
int buzzer_count;
int buzzer_limit = 0;
int buzzer_state = 1;

#define NUM_TONES 25

unsigned int note_freq[NUM_TONES] =
{ 131, 139, 147, 156, 165, 176, 185, 196, 208, 220, 233, 247,
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523};
  
char notenames[NUM_TONES][3] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ","C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ", "C " };


//Change settings for buzzer and find proper period
void buzzer_init() {
	TCCR0A |= (1 << WGM01);
	
	TIMSK0 |= (1 << OCIE0A);
	
	// MAx Count change
	unsigned long period = 1000000 / note_freq[freq_index];
	float tmp_period = period/1000000.0; //turn to seconds
	unsigned long cycles = 16000000 * (tmp_period/2.0);
	long max = cycles/1024;
	OCR0A = max;
	
	//set prescalar
	TCCR0B |= (1 << CS02);
	TCCR0B |= (1 << CS00);
	
	buzzer_limit = .5/(tmp_period/2.0);
	buzzer_count = 0;
}

//Print note to play to screen
void printNote() {
	if (freq_index < 12) {
		lcd_moveto(1, 14);
		lcd_stringout("3");	
	}
	else if (freq_index > 11 && freq_index < 24) {
		lcd_moveto(1, 14);
		lcd_stringout("4");		
	}
	else {
		lcd_moveto(1, 14);
		lcd_stringout("5");
	}
	lcd_moveto(0, 14);
	lcd_stringout(notenames[freq_index]);
}  

// initialize the temperature state
void tmpStateInit() {
	if (temp < count) {
		tmpState = cool;
		PORTD |= (1 << PD3);
		PORTD &= ~(1 << PD2);
		tmpState = cool;
		lcd_moveto(0, 7);
		lcd_stringout("cool");
	}
	else if (temp > count && temp < count+2) {
		TIMSK1 |= (1 << OCIE1A);
		PORTD &= ~(1 << PD3);
		tmpState = warm;
		lcd_moveto(0, 7);
		lcd_stringout("warm");
	}
	else {
		tmpState = hot;
		PORTD &= ~(1 << PD3);
		PORTD |= (1 << PD2);
		lcd_moveto(0, 7);
		lcd_stringout("hot ");
	}
}

int main(void) {
	lcd_init();
	unsigned char t[2];	
	
	if (ds_init() == 0) {
		// problem
		lcd_moveto(0, 3);
		lcd_stringout("Problem!");
		_delay_ms(2000);
	}
	
	// btns settings
	PORTC |= (1 << PC1);
	PORTC |= (1 << PC2);
	
	//rotary encoder settings
	encoder_init();
	
	//LED Settings
	DDRD |= (1 << PD2) | (1 << PD3);
	
	//Counter/Timer initialization
	//initialize CTC
	TCCR1B |= (1 << WGM12);
	
	// MAx Count
	OCR1A = 62500;
	
	//set prescalar
	TCCR1B |= (1 << CS12);
	
	//get threshold from eeprom
	count = eeprom_read_byte((void *) 200);

	if (count < 60 || count > 100) {
		count = 80;
	}
	
	//get note index from eeprom
	freq_index = eeprom_read_byte((void *) 100);
	
	if (freq_index < 0 || freq_index > 24) {
		freq_index = 0;
	}
		
	// Splash screen
	lcd_moveto(0, 0);
	lcd_stringout("    Project       ");
	lcd_moveto(1, 0);
	lcd_stringout("  Carson Mogush   ");
	
	_delay_ms(2000);
	lcd_moveto(0, 0);
	lcd_stringout("                   ");
	lcd_moveto(1, 0);
	lcd_stringout("                   ");
	
	// print threshhold
	lcd_moveto(1, 0);
	char buf[8];
	snprintf(buf, 8, "%d ", count);
	lcd_stringout(buf);
	
	printNote();
	
	ds_convert();
	lcd_moveto(1, 3);
	lcd_stringout("<");
	state = 0;
	
	tmpStateInit();
	//buzzer
	DDRC |= (1 << DDC5);
	
	while(1) {

		//check buttons
		if ((PINC & (1 << PC1)) == 0) {
			lcd_moveto(1, 3);
			lcd_stringout(" ");
			lcd_moveto(1, 12);
			lcd_stringout(">");
			state = 1;
		} 	
		if ((PINC & (1 << PC2)) == 0) {
			lcd_moveto(1, 12);
			lcd_stringout(" ");
			lcd_moveto(1, 3);
			lcd_stringout("<");
			state = 0;
		}
		
		//check for encoder changes
		if (changed) {
			changed = 0;
			if (state == 0) {
				lcd_moveto(1, 0);
				char buf[8];
				snprintf(buf, 8, "%d ", count);
				lcd_stringout(buf);
			}
			else {
				printNote();
			}
			
		}
		//manage outputs and states
		if (tmpState == cool) {
			if (temp > count && temp < count+2) {
				//turn on ISR timer
				TIMSK1 |= (1 << OCIE1A);
				PORTD &= ~(1 << PD3);
				tmpState = warm;
				lcd_moveto(0, 7);
				lcd_stringout("warm");
			}
			else if (temp > count+2) {
				tmpState = hot;
				PORTD &= ~(1 << PD3);
				PORTD |= (1 << PD2);
				lcd_moveto(0, 7);
				lcd_stringout("hot ");
				buzzer_init();
			}
		}
		else if (tmpState == warm) {
			if (temp < count) {
				PORTD |= (1 << PD3);
				PORTD &= ~(1 << PD2);
				tmpState = cool;
				//turn off ISR timer
				TIMSK1 &= ~(1 << OCIE1A);
				lcd_moveto(0, 7);
				lcd_stringout("cool");
			}
			else if (temp > count+2) {
				tmpState = hot;
				PORTD |= (1 << PD2);
				TIMSK1 &= ~(1 << OCIE1A);
				lcd_moveto(0, 7);
				lcd_stringout("hot ");
				buzzer_init();
			}
		}
		else {
			if (temp < count) {
				PORTD |= (1 << PD3);
				PORTD &= ~(1 << PD2);
				tmpState = cool;
				lcd_moveto(0, 7);
				lcd_stringout("cool");
			}
			else if (temp > count && temp < count+2) {
				//turn on ISR Timer
				TIMSK1 |= (1 << OCIE1A);
				tmpState = warm;
				lcd_moveto(0, 7);
				lcd_stringout("warm");
			}
		}
		
		//check temperature changes
		if (ds_temp(t)) {
			
			long t10 = t[1] << 8;
			t10 += t[0];
			temp = t10*10;
			temp = temp/16;
			temp = temp*9;
			temp = temp/5;
			temp = temp + 320;
			int dec = temp%10;
			temp = temp/10;
			
			lcd_moveto(0, 0);
			char buf[16];
			snprintf(buf, 16, "%ld.%d", temp, dec);
			lcd_stringout(buf);
			
			
			ds_convert();
		}
		
	}
}

ISR(TIMER1_COMPA_vect) {
	if (tmpColor % 2 == 0) {
		PORTD |= (1 << PD2);
	}
	else {
		PORTD &= ~(1 << PD2);
	}
	tmpColor++;
}

ISR(TIMER0_COMPA_vect) { 
	if (buzzer_state == 0) {
		PORTC |= (1 << PC5);
		buzzer_state = 1;
	}
	else {
		PORTC &= ~(1 << PC5);
		buzzer_state = 0;
	}
	buzzer_count++;
	if (buzzer_count == buzzer_limit) {
		TIMSK0 &= ~(1 << OCIE0A);
		PORTC &= ~(1 << PC5);
	}
}