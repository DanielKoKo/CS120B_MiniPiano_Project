/*	Author: Daniel Ko
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Miniature Piano Final Project
 *	Exercise Description: [optional - include for your own benefit]
 *  Complexity 1 (eeprom, custom lcd characters): https://youtu.be/QwFSXZ6Jds0
 *  Complexity 2 (joystick): https://youtu.be/jh2mRpE3iTU
 *  Complexity 3 (microphone): https://youtu.be/xWpH7MeLUec
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <timer.h>
#include <io.h>
#include <avr/eeprom.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void ADC_init() {
    ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

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

//melodies to be stored into eeprom
const short melody1[18] = { 392.00, 587.33, 523.25, 493.88, 440.00, 739.99,
                            659.25, 587.33, 783.99, 659.25, 739.99, 587.33,
                            659.25, 587.33, 523.25, 493.88, 392.00, 1.00 };
const short melody2[18] = { 329.00, 392.00, 440.00, 392.00, 523.25, 493.88,
		                    392.00, 392.00, 440.00, 392.00, 587.33, 523.25,
						    440.00, 523.25, 493.88, 523.25, 329.00, 1.00 };

unsigned char keys[] = " G A B C D E f G"; //keys to display on LCD

unsigned char curr_key = 0x00;  //tracks current key being pressed
unsigned char curr_melody = 0;  //current selected melody
unsigned char menu_cursor = 0;  //current cursor position
unsigned char selected = 0;     //checks whether menu option is selected 
unsigned char demo = 0;         //checks whether a melody is being played 
unsigned char finished = 0;     //checks whether the melody being played is finished
unsigned char bass = 0;		    //checks if lowest note on piano is played
unsigned short myADC = 0;	    //for microphone ADC
unsigned short curr_ADC = 0;    //gets current ADC reading
unsigned char first = 1;	    //checks if it's the first ADC reading
unsigned char j = 0;			//counts how long to hold a note in a melody
float* k = 1;					//reads a note from eeprom at position k

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
	    if (menu_cursor == 1 && selected) {
		PlayState = play_demo;
		demo = 1;
		finished = 0;
	    }
	    else if (tmpA >= 0x01) {
		PlayState = piano_pressed;
	    }
	    else { 
		PlayState = piano_released;
		selected = 0;
	    }
	    break;

	case play_demo:
	    if (finished) {
		PlayState = piano_released;
		menu_cursor = 0;
		demo = 0;
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
	    if (tmpA == 0x02) {
		set_PWM(783.99);
	    }
	    else if (tmpA == 0x04) {
		set_PWM(739.99);
	    }
	    else if (tmpA == 0x08 && !bass) {
		set_PWM(659.25);
	    }
	    else if (tmpA == 0x10) {
		set_PWM(587.33);
	    }
	    else if (tmpA == 0x20) {
		set_PWM(523.25);
	    }
	    else if (tmpA == 0x40) {
		set_PWM(493.88);
	    }
	    else if (tmpA == 0x80) {
		set_PWM(440.00);
	    }
	    else if (tmpA == 0x08 && bass) {
		set_PWM(392.00);
	    }
	    break;

	case piano_released:
	    set_PWM(0);
	    j = 0;
	    if (curr_melody + 1 == 1) {
	        k = 1;
	    }
	    else if (curr_melody + 1 == 2) {
		k = 100;
	    }
	    break;

	case play_demo:
	    if (eeprom_read_float(k) == 1.00) //1.00 represents the end of the current melody
	    {
			finished = 1;
	    }
	    else
	    {
	        if (j <= 1) //if hold time is not reached, keep playing current note in melody
			{
	            set_PWM(eeprom_read_float(k));
				j++;
	        }
	        else 
			{
	    	    j = 0; //reset hold time
				k++;   //go to next note in melody
	        }
	    }

	    break;
    }
}

//menu displays
unsigned char menu1[] = "SHOW KEYBOARD";
unsigned char menu2[] = "PLAY MELODY ";
unsigned char menu3[] = "CURR MELODY: ";
unsigned char playback[] = "PLAYING MELODY ";

unsigned char key_cursor = 0;	 //stores key cursor row
unsigned char menu_on = 0;		 //checks if menu display is on
unsigned char mismatch_cnt = 0;	 //checks if there is mismatch between ADC and current microphone reading
unsigned char third_option = 0;  //checks if menu's third option is being displayed

enum LCDStates { lcd_start, lcd_init, show_piano, show_playback, show_menu, menu_held } LCDState;
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
	    if ((~PIND & 0x10) == 0x10)
	    {
			LCDState = menu_held;
	    }
	    else
	    {
			LCDState = show_piano;
	    }
	    break;

	case show_playback:
	    if (demo)
	    {
			PORTB = 0x01;
			LCDState = show_playback;
	    }
	    else
	    {
			for (unsigned char j = 1; j <= 16; ++j) //clears first row on LCD
            {
                LCD_Cursor(j);
                LCD_WriteData(3);
            }

		PORTB = 0x00;
		LCDState = show_piano;
	    }
	    break;

	case menu_held:
	    menu_cursor = 0;
	    if ((~PIND & 0x10) == 0x10) //clears menu screen upon menu selection button press
	    {
		LCD_ClearScreen();
		LCDState = menu_held;
	    }
	    else
	    {
		if (menu_on)
		{
		    LCDState = show_menu;
		    menu_on = 0;
		}
		else if (demo)
		{
		    LCDState = show_playback;
		    menu_on = 0;
		}
		else
		{
		    LCDState = show_piano;
		    menu_on = 1;
		}
	    }
	    break;

	case show_menu:
	    if ((~PIND & 0x20) == 0x20) //checks if a menu option is selected
	    {
			selected = 1;
			LCD_ClearScreen();
			LCDState = menu_held;
	    }
	    break;
    }

    switch (LCDState) {
	case lcd_start:
	    break;

	case lcd_init:
	    break;

	case show_piano:
	    for (unsigned char j = 1; j <= sizeof(keys) - 1; ++j) //displays piano keys on second row
	    {
		LCD_Cursor(j + 16);
		LCD_WriteData(keys[j - 1]);	
	    }

		//checks what piano key to display on the first row based on what note is being played
	    if (tmpA == 0x02) 
	    {
		LCD_WriteCommand(0x8F);
    		LCD_WriteData(1);
	    }
	    else if (tmpA == 0x04)
	    {
    		LCD_WriteCommand(0x8D);
    		LCD_WriteData(2);
	    }
	    else if (tmpA == 0x08 && !bass)
	    {
			LCD_WriteCommand(0x8B);
			LCD_WriteData(1);
	    }
	    else if (tmpA == 0x10)
	    {
			LCD_WriteCommand(0x89);
			LCD_WriteData(1);
	    }
	    else if (tmpA == 0x20)
	    {
			LCD_WriteCommand(0x87);
			LCD_WriteData(1);
	    }
	    else if (tmpA == 0x40)
	    {
			LCD_WriteCommand(0x85);
			LCD_WriteData(1);
	    }
	    else if (tmpA == 0x80)
	    {
			LCD_WriteCommand(0x83);
			LCD_WriteData(1);	
	    }
	    else if (tmpA == 0x08 && bass)
	    {
			LCD_WriteCommand(0x81);
			LCD_WriteData(1);
	    }
	    else //clears first row
	    {
			LCD_WriteCommand(0x80);
			LCD_WriteData(3);
			LCD_WriteCommand(0x81);
			LCD_WriteData(3);
			LCD_WriteCommand(0x82);
            LCD_WriteData(3);
			LCD_WriteCommand(0x83);
			LCD_WriteData(3);
			LCD_WriteCommand(0x84);
            LCD_WriteData(3);
			LCD_WriteCommand(0x85);
            LCD_WriteData(3);
			LCD_WriteCommand(0x86);
            LCD_WriteData(3);
            LCD_WriteCommand(0x87);
            LCD_WriteData(3);
			LCD_WriteCommand(0x88);
            LCD_WriteData(3);
			LCD_WriteCommand(0x89);
            LCD_WriteData(3);
			LCD_WriteCommand(0x8A);
            LCD_WriteData(3);
            LCD_WriteCommand(0x8B);
            LCD_WriteData(3);
			LCD_WriteCommand(0x8D);
            LCD_WriteData(3);
            LCD_WriteCommand(0x8F);
            LCD_WriteData(3);
	    }
	    break;

	case show_playback:
		//displays melody playback message
	    for (unsigned char j = 1; j <= sizeof(playback) - 1; ++j)
	    {
		LCD_Cursor(j);
		LCD_WriteData(playback[j - 1]);
	    }
	    break;

	case show_menu:
	    if ((~PIND & 0x01) == 0x01)
	    {
			if (menu_cursor < 3) //if on the first menu screen, move to second screen
			{
				menu_cursor++;
			}
	    }
	    else if ((~PIND & 0x02) == 0x02) //if on the second menu screen, move to first screen
	    {
		if (menu_cursor > 0)
		{
		    menu_cursor--;
		}
	    }

		//prints corresponding menu options
	    if (menu_cursor == 0 || menu_cursor == 2)
	    {
		LCD_Cursor(17);
		LCD_WriteData(3);
		LCD_Cursor(1);
		LCD_WriteData('>');
	    }
	    else if (menu_cursor == 1)
	    {
		LCD_Cursor(1);
		LCD_WriteData(3);
		LCD_Cursor(17);
		LCD_WriteData('>');
	    }

	    if (menu_cursor < 2)
	    {
		third_option = 0;
                LCD_Cursor(15);
                LCD_WriteData(3);

	        for (unsigned char j = 1; j <= sizeof(menu1) - 1; ++j)
                {
                    LCD_Cursor(j + 1);
                    LCD_WriteData(menu1[j - 1]);
                }
	        for (unsigned char j = 1; j <= sizeof(menu2) - 1; ++j)
                {
                    LCD_Cursor(j + 17);
                    LCD_WriteData(menu2[j - 1]);
		}

		LCD_WriteData(curr_melody + 1 + '0');
	    }
	    else
	    {
		third_option = 1;
		for (unsigned char j = 17; j <= 32; ++j)
		{
		    LCD_Cursor(j);
		    LCD_WriteData(3);
		}
		for (unsigned char j = 1; j <= sizeof(menu3) - 1; ++j)
		{
		    LCD_Cursor(j + 1);
		    LCD_WriteData(menu3[j - 1]);
		}

		LCD_WriteData(curr_melody + 1 + '0');
	    }

		//microphone input
	    myADC = ADC;
        if (first)
        {
            curr_ADC = myADC;
            first = 0;
        }
        else if (!first && third_option)
        {
            if (curr_ADC != myADC)
            {
				mismatch_cnt++;
		    
				if (mismatch_cnt > 2)
				{
					mismatch_cnt = 0;
                    PORTB = PORTB | 0x01;
                    if (curr_melody < 1)
                    {
                        curr_melody++;
                    }
                    else
						curr_melody--;
                }
		    }

            curr_ADC = myADC;
        }
        else
        {
            PORTB = PORTB & 0x00;
        }

            curr_ADC = myADC;
     }

	    break;
     }
}

int main(void) {
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xC0; PORTD = 0x3F;

    TimerSet(50);
    TimerOn();
    PWM_on();

    PlayState = play_start;
    LCDState = lcd_start;

    ADC_init();
    LCD_init();
    LCD_ClearScreen();

    //first melody
    float* j = 1;
    for (int i = 0; i <= sizeof(melody1) - 1; ++i) {
		eeprom_write_float(j, melody1[i]);
		j++;
    }
    
    //second melody
    float* h = 100;
    for (int m = 0; m <= sizeof(melody2) - 1; ++m) {
		eeprom_write_float(h, melody2[m]);
		h++;
    }

	//custom lcd characters
    unsigned char clear[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char whole_key[] = { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F };
    unsigned char half_key[] = { 0x0E, 0x0E, 0x0E, 0x0E, 0xE, 0xE, 0x0E, 0x0E };

    CreateCustomCharacter(whole_key, 1);
    CreateCustomCharacter(half_key, 2);
    CreateCustomCharacter(clear, 3);

    unsigned char tmpA = 0x00;

    while (1) {
	if ((~PIND & 0x08) == 0x08) //if bass note is pressed, set tmpA to piano key on PIND. Otherwise, set to PINA
	{
	    tmpA = ~PIND & 0x08;
	    bass = 1;
	}
	else
	{
	    tmpA = ~PINA & 0xFE;
	    bass = 0;
	}

	Piano_Tick(tmpA);
	LCD_Tick(tmpA);

	while(!TimerFlag);
	TimerFlag = 0;
    }

    return 1;
}




