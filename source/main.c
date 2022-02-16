/*	Author: Daniel Ko
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Final Project
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <timer.h>
#include <io.h>
#include <scheduler.h>
#include <avr/eeprom.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void set_PWM(double frequency) {
    static double current_frequency;

    if (frequency != current_frequency) {
        if (!frequency) { TCCR3B &= 0x08; }
        else { TCCR3B |= 0x03; }

        if (frequency < 0.954) { OCR3A = 0xFFFF; }
        else if (frequency > 31250) { OCR3A = 0x0000; }
        else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

        TCNT3 = 0;
        current_frequency = frequency;
    }
}

void PWM_on() {
    TCCR3A = 1 << COM3A0;
    TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);

    set_PWM(0);
}

void PWM_off() {
    TCCR3A = 0x00;
    TCCR3B = 0x00;
}

int i = 0;
unsigned char j = 0;
float* k = 1;
unsigned short notes[26] = { 329.63, 0, 293.66, 0, 261.63, 0, 293.66, 0,
                             329.63, 0, 329.63, 0, 329.63, 0, 293.66, 0,
                             293.66, 0, 293.66, 0, 329.63, 0, 329.63, 0,
                             329.63, 0 };
unsigned char hold_time[26] = { 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 6,
                                2, 1, 2, 1, 2, 6, 2, 1, 2, 1, 2, 1 };

const short melody1[17] = { 392.00, 587.33, 523.25, 493.88, 440.00, 739.99,
                            659.25, 587.33, 783.99, 659.25, 739.99, 587.33,
                            659.25, 587.33, 523.25, 493.88, 392.00 };
unsigned char hold_time2[17] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
			         1, 1, 1 };

unsigned char curr_key = 0x00;
enum PlayPianoStates { play_start, piano_pressed, piano_released, play_demo } PlayState;
void Piano_Tick(unsigned char tmpA) {
    curr_key = tmpA;
    switch (PlayState) {
	case play_start:
	    PlayState = piano_released;
	    break;

	case piano_pressed:
	    if (tmpA == 0x00) {
		PlayState = piano_released;
	    }
	    else {
		PlayState = piano_pressed;
	    }
	    break;

	case piano_released:
	    PORTB = 0x00;
	    curr_key = 0x00;
	    if ((~PIND & 0x20) >= 0x01) {
		PlayState = play_demo;
	    }
	    else if (tmpA >= 0x01) {
		PlayState = piano_pressed;
	    }
	    else { 
		PlayState = piano_released;
	    }
	    break;

	case play_demo:
	    PORTB = 0x01;
	    if (i > 16) {
		PlayState = piano_released;
	    }
	    else {
		PlayState = play_demo;
	    }
	    break;
    }	    

    switch (PlayState) {
	case play_start:
	    break;

	case piano_pressed:
	    if (tmpA == 0x01) {
		set_PWM(783.99);
	    }
	    else if (tmpA == 0x02) {
		set_PWM(739.99);
	    }
	    else if (tmpA == 0x04) {
		set_PWM(659.25);
	    }
	    else if (tmpA == 0x08) {
		set_PWM(587.33);
	    }
	    else if (tmpA == 0x10) {
		set_PWM(523.25);
	    }
	    else if (tmpA == 0x20) {
		set_PWM(493.88);
	    }
	    else if (tmpA == 0x40) {
		set_PWM(440.00);
	    }
	    else if (tmpA == 0x80) {
		set_PWM(392.00);
	    }
	    break;

	case piano_released:
	    set_PWM(0);
	    i = 0;
	    j = 0;
	    k = 1;
	    break;

	case play_demo:
	    if (j <= hold_time2[i]) {
		set_PWM(eeprom_read_float(k));
		j++;
	    }
	    else {
		j = 0;
		k++;
		i++;
	    }
	    break;
    }
}

unsigned char keys[] = " G A B C D E f G";
unsigned char key_cursor = 0;
enum LCDStates { lcd_start, lcd_init, show_piano, show_menu } LCDState;
void LCD_Tick(unsigned char tmpA) {
    switch (LCDState) {
	case lcd_start:
	    LCD_init();
	    LCDState = lcd_init;
	    break;

	case lcd_init:
	    LCDState = show_piano;
	    break;

	case show_piano:
	    LCDState = show_piano;
	    break;

	case show_menu:
	    break;
    }

    switch (LCDState) {
	case lcd_start:
	    break;

	case lcd_init:
	    break;

	case show_piano:
	    for (unsigned char j = 1; j <= sizeof(keys) - 1; ++j) 
	    {
		LCD_Cursor(j + 16);
		LCD_WriteData(keys[j - 1]);	
	    }

	    if (tmpA == 0x01) 
	    {
		LCD_WriteCommand(0x8F);
    		LCD_WriteData(1);
	    }
	    else if (tmpA == 0x02)
	    {
    		LCD_WriteCommand(0x8D);
    		LCD_WriteData(2);
	    }
	    else if (tmpA == 0x04)
	    {
		LCD_WriteCommand(0x8B);
		LCD_WriteData(1);
	    }
	    else if (tmpA == 0x08)
	    {
		LCD_WriteCommand(0x89);
		LCD_WriteData(1);
	    }
	    else if (tmpA == 0x10)
	    {
		LCD_WriteCommand(0x87);
		LCD_WriteData(1);
	    }
	    else if (tmpA == 0x20)
	    {
		LCD_WriteCommand(0x85);
		LCD_WriteData(1);
	    }
	    else if (tmpA == 0x40)
	    {
		LCD_WriteCommand(0x83);
		LCD_WriteData(1);	
	    }
	    else if (tmpA == 0x80)
	    {
		LCD_WriteCommand(0x81);
		LCD_WriteData(1);
	    }
	    else 
	    {
		LCD_WriteCommand(0x81);
		LCD_WriteData(3);
		LCD_WriteCommand(0x83);
		LCD_WriteData(3);
		LCD_WriteCommand(0x85);
                LCD_WriteData(3);
                LCD_WriteCommand(0x87);
                LCD_WriteData(3);
		LCD_WriteCommand(0x89);
                LCD_WriteData(3);
                LCD_WriteCommand(0x8B);
                LCD_WriteData(3);
		LCD_WriteCommand(0x8D);
                LCD_WriteData(3);
                LCD_WriteCommand(0x8F);
                LCD_WriteData(3);
	    }
	    break;

	case show_menu:
	    break;
    }
}

/*
enum RecordStates { record_init, record_wait, record_start, record_stop } RecordState;
void Record_Tick(unsigned char tmpD) {
    switch (RecordState) {
	case record_init:
	    
    }

    switch (RecordState) {

    }
}
*/

/*
const float melody1[] = { 392.00, 587.33, 523.25, 493.88, 440.00, 739.99, 
	                    659.25, 587.33, 783.99, 659.25, 739.99, 587.33, 
			    659.25, 587.33, 523.25, 493.88, 392.00 };
const float hold_time1[] = { 2, 2
*/

int main(void) {
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xC0; PORTD = 0x3F;

    /*
    static task task1, task2;
    task *tasks[] = { &task1, &task2 };

    const unsigned short numTasks = sizeof(tasks) / sizeof(task*);

    task1.state = 0;
    task1.period = 5;
    task1.elapsedTime = 0;
    task1.TickFct = &Piano_Tick;

    task2.state = 0;
    task2.period = 5;
    task2.elapsedTime = 0;
    task2.TickFct = &LCD_Tick;
    */

    TimerSet(50);
    TimerOn();
    PWM_on();

    PlayState = play_start;
    LCDState = lcd_start;

    LCD_init();

    /*
    eeprom_write_float(1, notes[0]);
    while (eeprom_read_float(1) == 392.00) {
	PORTB = 0x01;
    }
    */

    float* j = 1;
    for (int i = 0; i < 25; i++) {
	eeprom_write_float(j, melody1[i]);
	j++;
    }

    /*
    float *k;
    float note;
    for (k = 1; k < 25; k++) {
	note = &k;

    float* k = 1;
    set_PWM(eeprom_read_float(k));	
    */

    unsigned char clear[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char whole_key[] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
    unsigned char half_key[] = { 0x0E, 0x0E, 0x0E, 0x0E, 0xE, 0xE, 0x0E, 0x0E };

    CreateCustomCharacter(whole_key, 1);
    CreateCustomCharacter(half_key, 2);
    CreateCustomCharacter(clear, 3);

    while (1) {
	Piano_Tick(~PINA & 0xFF);
	LCD_Tick(~PINA & 0xFF);

	/*
	for (unsigned short i = 0; i < numTasks; ++i) 
	{
	    if (tasks[i]->elapsedTime == tasks[i]->period)
	    {
		tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
		tasks[i]->elapsedTime = 0;
	    }

	    tasks[i]->elapsedTime += 5;
	}
	*/

	while(!TimerFlag);
	TimerFlag = 0;
    }

    return 1;
}



