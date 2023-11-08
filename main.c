/*
 * GccApplication2.c
 *
 * Created: 04.10.2023 15:42:34
 * Author : student
 */ 
#define LEFT 0b01111111
#define RIGHT 0b10111111
#define CHANGING 0b11011111
#define CHANGING_UP 0b11111101
#define CHANGING_DOWN 0b11111110	


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "LCD.h"
#include "TIME.h"
#include "DATA.h"

int positionOnDisplay = 0;
int shiftCursorOnDisplay;
bool isFirstLine;
bool isGoingLeft = true;
bool stop_flag = false;
bool isMovingToTheRIght;
int flagMoovingSpeed = 0;

int main(void)
{
	initStopButton();
	initTimerCounter();
	LCDInit();
	nullifyTimeArray();
	nullifyDataArray();

	while (1)
	{
		displayData(positionOnDisplay);
		displayTime(positionOnDisplay);
		if (!stop_flag)
		{
			computePosition();
		}
		else
		{
			if (PIND == CHANGING_UP)
			{	
				cli();
				GICR &= (0<<INT0);
				changingMode();				
			}
		}
	}
	return 0;
 }
 
 
void changingMode()
{
	isFirstLine = true;
	shiftCursorOnDisplay = 3;
	isMovingToTheRIght = true;
	while(1)
	{
		togglePosition();
		switch(PIND)
		{
			case CHANGING: 
				sei();
				initStopButton();
				return 0;
				break;
			case LEFT:
				isMovingToTheRIght = false;
				shiftCursorOnDisplay--;
				checkCursorPosition();
				_delay_ms(300);
				break;
			case RIGHT:
				isMovingToTheRIght = true;
				shiftCursorOnDisplay++;
				checkCursorPosition();
				_delay_ms(300);
				break;
			case CHANGING_UP:
				if (isFirstLine)
					changePositionTime(shiftCursorOnDisplay - 3, 1);
				else
					changePositionData(shiftCursorOnDisplay, 1);
				_delay_ms(300);
				break;
			case CHANGING_DOWN:
				if (isFirstLine)
					changePositionTime(shiftCursorOnDisplay - 3, -1);
				else
					changePositionData(shiftCursorOnDisplay, -1);
				_delay_ms(300);
				break;
			default:break;
		}
		
	
	}
}

void togglePosition()
{
	if (!isFirstLine && shiftCursorOnDisplay > 2 && shiftCursorOnDisplay < 6)
	{
		toggleMonth();
		return;
	}
	LCDKursorPosihin(!isFirstLine, shiftCursorOnDisplay + positionOnDisplay);
	LCDWriteByte(LCD_DR, ' ');
	_delay_ms(150);
	if (isFirstLine)
		displayTime(positionOnDisplay);
	else
		displayData(positionOnDisplay);
	_delay_ms(150);
} 

void checkCursorPosition()
{
	if (isFirstLine)
	{
		switch(shiftCursorOnDisplay-3)
		{
			case -1:
				changeLine();
				break;
			case 2:
				shiftFromColon();
				break;
			case 5:
				shiftFromColon();
				break;
			case 8:
				changeLine();
				break;
			default:
				return;
		}
	}
	else 
		switch(shiftCursorOnDisplay)
		{
			case -1:
				changeLine();
				break;
			case 2:
				shiftFromColon();
				break;
			case 4:
				shiftFromMonth();
				break;
			case 5:
				shiftFromMonth();
				break;
			case 6:
				shiftFromColon();
				break;
			case 11:
				changeLine();
				break;
			default:
				return; 
		}
}


void toggleMonth()
{
	LCDKursorPosihin(1, positionOnDisplay+3);
	for (int i = 0; i < 3; i++)
		LCDWriteByte(LCD_DR, ' ');
	_delay_ms(200);
	displayData(positionOnDisplay);
	_delay_ms(200);
}

void changeLine()
{
	isFirstLine = !isFirstLine;
	if (isFirstLine)
	{
		if (isMovingToTheRIght)
		{
			shiftCursorOnDisplay = 3;
			return;
		}
		shiftCursorOnDisplay = 10;
	}
	else
	{
		if (isMovingToTheRIght)
		{
			shiftCursorOnDisplay = 0;
			return;
		}
		shiftCursorOnDisplay = 10;
	}
}

void shiftFromMonth()
{
	if (isMovingToTheRIght)
		shiftCursorOnDisplay += 3;
	else
		shiftCursorOnDisplay -= 3;
}

void shiftFromColon()
{
	if (isMovingToTheRIght)
		shiftCursorOnDisplay++;
	else
		shiftCursorOnDisplay--;
}
void computePosition()
{
	switch(PIND)
	{
		case CHANGING:
			flagMoovingSpeed = 0;
			break;
		case RIGHT:
			flagMoovingSpeed = 1;
			break;
		case LEFT:
			flagMoovingSpeed = 2;
			break;
	}
	
	switch(flagMoovingSpeed)
	{
		case 0:
			_delay_ms(150);
			break;
		case 1:
			_delay_ms(500);
			break;
		case 2:
			_delay_ms(1000);		
			break;	
	}
	if (positionOnDisplay == 5 || positionOnDisplay == 0)
		isGoingLeft = !isGoingLeft;

	isGoingLeft ? positionOnDisplay-- : positionOnDisplay++;
}


void initStopButton()
{
	DDRD = 0; 
	MCUCR = (1<<ISC01) | (1<<ISC00);
	GICR |= (1<<INT0);
	GIFR = (1<<INTF0);
}

void initTimerCounter() // инициализация таймер
{
	TCNT1 = 0;
	TCCR1B |= (1<<CS10) | (1<<CS11) | (0<<CS12);
	TIMSK |= (1 << OCIE1B);
	OCR1BH = 0b00111101;
	OCR1BL = 0b00001001;

	//fast mode: 1 second = 1 hour
	//OCR1BH = 0b00000001;
	//OCR1BL = 0b00000100;
	sei();
}

ISR(INT0_vect){ // прерывание кнопки
	stop_flag = !stop_flag;
	_delay_ms(60);
}

ISR(TIMER1_COMPB_vect) { // прерывание таймер-счётчика
	computeTimeArray();
	TCNT1 = 0;
}
