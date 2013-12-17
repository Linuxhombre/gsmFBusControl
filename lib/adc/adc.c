/*
 * adc.c
 *
 * Create on: Apr 15, 2013
 * 		Author: popai
 *
 */

/**
* @file adc.c
*
* @brief This module is used for interfacing AVR's Inbuilt ADC(Analog To Digital Convertor).
*
* Contains common routines used to red analog avlues.
*
*/

#include <avr/io.h>

#include "adc.h"

void InitADC()
{
/*
	//sets the reference voltage to AVCC
	ADMUX |= (1 << REFS0);

	//enable the ADC and Prescaler at 128 so we have an 125Khz clock source
	ADCSRA = (1 << ADEN) | (7 << ADPS0);
*/
	 ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));    //16Mhz/128 = 125Khz the ADC reference clock
	 ADMUX |= (1<<REFS0);                //Voltage reference from Avcc (5v)
	 ADCSRA |= (1<<ADEN);                //Turn on ADC
	 ADCSRA |= (1<<ADSC);                //Do an initial conversion because this one is the slowest and to ensure that everything is up and running
}

uint16_t ReadADC(uint8_t ch)
{
	//Select ADC Channel ch must be 0-7
	ch = ch & 0b00000111;
	ADMUX &= 0b11100000;
	ADMUX |= ch;

	//Start Single conversion
	ADCSRA |= (1 << ADSC);

	//Wait for conversion to complete
	while (!(ADCSRA & (1 << ADIF)))
		;

	//Clear ADIF by writing one to it
	ADCSRA |= (1 << ADIF);

	return (ADC);
}
