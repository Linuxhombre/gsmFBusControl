/*
 * gsmFBusControl.c
 *
 *  Created on: Apr 14, 2013
 *      Author: popai
 *
 * Fuse bits:
 * BootLock12 	setat
 * BootLock11 	setat
 * BOOTSZ0 		setat
 * BOOTRST 		setat
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#include "lib/fbus/fbus.h"
#include "lib/usart/usart.h"
#include "lib/adc/adc.h"
#include "pinDef.h"
#include "cmd.h"

void setup();
void loop();

int main(void)
{
	setup();
	loop();
}

void setup()
{

// Add your initialization code here
	pinSetUp();
	PORTB = (1 << PB4);
	InitADC();
	//USARTInit(103);	//bud rate 9600
	USARTInit(8); //bud rate 115200
	_delay_ms(200);
	UWriteString("Program pornit");
	UWriteData(0x0D);
	UWriteData(0x0A);

	FON_INIT();
	_delay_ms(500);
	UFlushBuffer();
	PORTB &= ~(1 << PB4);
}

void loop()
{
	int8_t config = 0, cfgpc = 0;
	//uart_sendsms("+40744946000", "+40745183841", "Modul Pornit");
	//uart_sendsms("+40766000510", "+40765445976", "Modul Pornit");
	//uart_sendsms("+40766000510", "+40745183841", "Modul Pornit");
	if ((PINC & (1 << PC3)) == 0)
	{
		config = 1;
	}

	if ((PINC & (1 << PC4)) == 0)
	{
		DellEprom();
		return;
	}


	if ((PINC & (1 << PC5)) == 0)
	{
		cfgpc = 1;
	}
	char read[32];
	char teln[18];
	enum fbus_frametype ftype;
	memset(read, 0, sizeof(read));
	memset(teln, 0, sizeof(teln));
	//uint16_t contor = 500;

	while (1)
	{
		_delay_ms(300);
		/*
		 if (contor == 500)
		 {
		 contor = 0;
		 BateryFull();
		 }
		 ++contor;
		 */

		if (cfgpc)
		{
			SerialRead(read, 32);
			UWriteString(read);
			if (strlen(read) != 0)
			{
				if (strstr_P(read, PSTR("citeste")) != 0)
				//if (strstr(read, "citeste") != 0)
				{
					for (int i = 0; i <= 512; i += 18)
					{
						ReadEprom(teln, i);
						sprintf(read, "%d: %s%c%c", i, teln, 0x0D, 0x0A);
						UWriteString(read);
					}
				}
				else
				{
					int8_t adresa = CfgCmd(read);
					if (adresa)
					{
						sprintf_P(read, PSTR("%d: OK%c%c"), adresa, 0x0D, 0x0A);
						UWriteString(read);
					}
					else
					{
						sprintf_P(read, PSTR("%d: ERROR%c%c"), adresa, 0x0D, 0x0A);
						UWriteString(read);

					}
				}
				*read = 0x00;
			}
		}
		else if (config)
		{
			_delay_ms(100);
			ftype = fbus_readframe(teln, read);
			if (ftype == FRAME_SMS_RECV)
			{
				Config(teln, read);
				//_delay_ms(250);
			}
			*read = 0x00;
			*teln = 0x00;
		}
		else
		{
			VerificIN();
			//UWriteString("bau");
			/*
			 uart_sendsms("+40744946000", "+40745183841", "alarma port1");
			 SerialRead(read);
			 //delay_ms(20);
			 if (strlen(read) != 0)
			 {
			 UWriteString(read);
			 UWriteData(0x0D);
			 UWriteData(0x0A);
			 }

			 //uart_sendsms("+40745183841", "Hi All. This message was sent through F-Bus. Cool!!");
			 */
			ftype = fbus_readframe(teln, read);
			if (ftype == FRAME_SMS_RECV)
			{
				PORTD |= (1 << PD2);
				Comand(teln, read);
				//Comand(pfonnr, "test 1");
				//_delay_ms(250);
			}
			*read = 0x00;
			*teln = 0x00;
		}
	}
}

