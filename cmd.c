/*
 * cmd.c
 *
 *  Created on: Apr 15, 2013
 *      Author: popai
 */
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "cmd.h"
#include "pinDef.h"
#include "lib/fbus/fbus.h"
#include "lib/usart/usart.h"
#include "lib/adc/adc.h"

static float Thermistor(int RawADC);
static void StareOUT(char *smsc_nr, char *nrtel);
static void StareTMP(char *smsc_nr, char *nrtel);
static void SetPort();

//"Flash store comands etc are strings to store
const prog_char IN1[] PROGMEM = "IN1"; //adresa 18*2
const prog_char IN2[] PROGMEM = "IN2"; //adresa 18*3
const prog_char IN3[] PROGMEM = "IN3"; //adresa 18*4
const prog_char IN4[] PROGMEM = "IN4"; //adresa 18*5
const prog_char OUT1H[] PROGMEM = "OUT1L"; //adresa 18*6
const prog_char OUT1L[] PROGMEM = "OUT1H"; //adresa 18*7
const prog_char OUT2H[] PROGMEM = "OUT2L"; //adresa 18*8
const prog_char OUT2L[] PROGMEM = "OUT2H"; //adresa 18*9
const prog_char OUT3H[] PROGMEM = "OUT3L"; //adresa 18*10
const prog_char OUT3L[] PROGMEM = "OUT3H"; //adresa 18*11
const prog_char OUT4H[] PROGMEM = "OUT4L"; //adresa 18*12
const prog_char OUT4L[] PROGMEM = "OUT4H"; //adresa 18*13
const prog_char OUT5H[] PROGMEM = "OUT5L"; //adresa 18*14
const prog_char OUT5L[] PROGMEM = "OUT5H"; //adresa 18*15
const prog_char TMP1[] PROGMEM = "TMP1"; //adresa 18*16
const prog_char TMP2[] PROGMEM = "TMP2"; //adresa 18*17
const prog_char TMP3[] PROGMEM = "TMP3"; //adresa 18*18
const prog_char LOGIN[] PROGMEM = "LOGIN"; //adresa 18*19
const prog_char SMSC[] PROGMEM = "SMSC"; //adresa 360
const prog_char STARE_OUT[]PROGMEM = "STARE OUT"; //adresa 18*21
const prog_char STARE_TMP[] PROGMEM = "STARE TMP"; //adresa 18*22
const prog_char STARE_ALL[]PROGMEM = "STARE ALL"; //adresa 18*23

const prog_char CMD_ERR[] PROGMEM = "Comanda ne scrisa";

// The table to refer to my strings.
PROGMEM const char *comenzi[] =
{ IN1, IN2, IN3, IN4, OUT1H, OUT1L, OUT2H, OUT2L, OUT3H, OUT3L, OUT4H, OUT4L,
		OUT5H, OUT5L, TMP1, TMP2, TMP3, LOGIN, SMSC };

int8_t in1 = 1, in2 = 1, in3 = 1, in4 = 1;

/**
 * @breaf initialize the pfon comunication
 *
 * @param no parametrs
 */
int8_t FON_INIT()
{
	enum fbus_frametype ftype;
	char buffer[24];
	memset(buffer, 0, sizeof(buffer));

	//write mesage in EEPPROM
	/*
	 eeprom_read_block(buffer, (int*) 486, 18);
	 if (strlen(buffer) == 0)
	 {
	 strcpy_P(buffer, CMD_ERR);
	 eeprom_write_block(buffer, (int*) 486, 18);
	 //strcpy_P(buffer, 0x00);
	 }
	 */

	SetPort();
	strcpy_P(buffer, CMD_ERR);
	eeprom_update_block(buffer, (int*) 486, 18);

	uint8_t init[] =
	{ 0x00, 0x01, 0x64, 0x01, 0x01, 0x40 };
	fbus_init();
	sendframe(0x40, init, 6);
	ftype = fbus_readack();
	if (ftype == FRAME_UNKNOWN)
	{
		sendframe(0x40, init, 6);
		ftype = fbus_readack();
		if (ftype == FRAME_UNKNOWN)
		{
			sendframe(0x40, init, 6);
			ftype = fbus_readack();
			if (ftype == FRAME_UNKNOWN)
			{
				PORTB |= (1 << PB5); //eror
				return 0;
			}
		}
	}

	//save smsc number if is not saved
	char smsc_nr[18];
	memset(smsc_nr, 0, sizeof(smsc_nr));
	_delay_ms(1000);
	fbus_getsmsc(smsc_nr);

	/*
	 eeprom_read_block(buffer, (int*) 360, 18);
	 if ((strcmp(smsc_nr, buffer) != 0) && (strlen(smsc_nr) != 0))
	 eeprom_write_block(smsc_nr, (int*) 360, 18);
	 */
	if (strlen(smsc_nr) != 0)
		eeprom_update_block(smsc_nr, (int*) 360, 18);

	sendack(0x02, 0x04);

	uint8_t init_sms[] =
	{ 0x00, 0x01, 0x00, 0x07, 0x02, 0x01, 0x01, 0x64, 0x01, 0x47 };
	sendframe(0x02, init_sms, 0x0a);
	//fbus_readack();
	sendack(0x14, 0x00);
	//uart_sendsms("+40744946000", "+40745183841", smsc_nr);
	return 1;
}

//write data on eeprom
int CfgCmd(char *inbuffer)
{
	int adresa = 18;
	//char *scriu;
	char comanda[7];

	for (int8_t i = 0; i < 19; ++i)
	{
		strcpy_P(comanda, (char*) pgm_read_word(&(comenzi[i]))); // Necessary casts and dereferencing, just copy.
		if (strstr(inbuffer, comanda) != 0)
		{

			inbuffer += strlen(comanda) + 1;
			adresa = 18 * (2 + i);
#if DEBUG
			//Serial.print("Scriu la adresa ");
			UWriteInt(adresa);
			UWriteString(": ");
			UWriteString(inbuffer);
			UWriteData(0x0D);
			UWriteData(0x0A);
#endif
			eeprom_write_block(inbuffer, (int*) adresa, 18);
			return adresa;
		}
	}
	return 0;
}

//read data from eeprom on specified adres
void ReadEprom(char* str_citit, int const adresa)
{
	//char str_citit[18];
	eeprom_read_block(str_citit, (int*) adresa, 18);
	/*
	 #if DEBUG
	 //Serial.print("citesc la adresa: ");
	 UWriteInt(adresa);
	 UWriteString(str_citit);
	 UWriteData(0x0D);
	 UWriteData(0x0A);
	 #endif
	 */
}

void DellEprom()
{
	for (int i = 0; i < 512; i++)
		eeprom_write_byte((uint8_t*) i, 0);
}
/*
 void DellPass()
 {
 for (int i = 18 * 19; i < 18 * 20; i++)
 EEPROM.update(i, 0);
 }
 */

/**
 * @breaf set the port on startup
 *
 * @param none
 */
void SetPort()
{
	int i;
	int8_t byte;
	for (i = 379; i < 384; i++)
	{
		byte = eeprom_read_byte((uint8_t*) i);
		switch (i)
		{
		case 379:
			if (byte)
				PORTD |= (1 << PD2);
			else
				PORTD &= ~(1 << PD2);
			break;

		case 380:
			if (byte)
				PORTD |= (1 << PD3);
			else
				PORTD &= ~(1 << PD3);
			break;

		case 381:
			if (byte)
				PORTD |= (1 << PD4);
			else
				PORTD &= ~(1 << PD4);
			break;

		case 382:
			if (byte)
				PORTD |= (1 << PD5);
			else
				PORTD &= ~(1 << PD5);
			break;

		case 383:
			if (byte)
				PORTD |= (1 << PD6);
			else
				PORTD &= ~(1 << PD6);
			break;

		default: break;

		}

	}
}

/**
 * @breaf execut sms commands
 *
 * @param nrtel: pfon number
 * 		  inmsg: sms command
 */
void Comand(char *nrtel, char *inmsg)
{
	char buffer[18];
	memset(buffer, 0, sizeof(buffer));
	char OK[3] = "OK";
	char smsc_num[18];
	memset(smsc_num, 0, sizeof(smsc_num));
	//bool cmdok = false;
	ReadEprom(buffer, 18);
	ReadEprom(smsc_num, 360);

	if (strcmp(nrtel, buffer) == 0)
	{

		ReadEprom(buffer, 18 * 6);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD1, LOW);
			PORTD &= ~(1 << PD2);
			eeprom_update_byte((uint8_t*) 379, 0);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 7);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD1, HIGH);
			PORTD |= (1 << PD2);
			eeprom_update_byte((uint8_t*) 379, 1);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 8);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD2, LOW);
			PORTD &= ~(1 << PD3);
			eeprom_update_byte((uint8_t*) 380, 0);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 9);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD2, HIGH);
			PORTD |= (1 << PD3);
			eeprom_update_byte((uint8_t*) 380, 1);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 10);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD3, LOW);
			PORTD &= ~(1 << PD4);
			eeprom_update_byte((uint8_t*) 381, 0);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 11);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD3, HIGH);
			PORTD |= (1 << PD4);
			eeprom_update_byte((uint8_t*) 381, 1);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 12);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD4, LOW);
			PORTD &= ~(1 << PD5);
			eeprom_update_byte((uint8_t*) 382, 0);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 13);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD4, HIGH);
			PORTD |= (1 << PD5);
			eeprom_update_byte((uint8_t*) 382, 1);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 14);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD5, LOW);
			PORTD &= ~(1 << PD6);
			eeprom_update_byte((uint8_t*) 383, 0);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		ReadEprom(buffer, 18 * 15);
		if (strcasecmp(buffer, inmsg) == 0)
		{
			//digitalWrite(outD5, HIGH);
			PORTD |= (1 << PD6);
			eeprom_update_byte((uint8_t*) 383, 1);
			uart_sendsms(smsc_num, nrtel, OK);
			return;
		}

		//strcpy_P(buffer, (char*) pgm_read_word(&(comenzi[18])));
		//if (strcasecmp(buffer, inmsg) == 0)
		if (strcasecmp_P(inmsg, STARE_OUT) == 0)
		{
			StareOUT(smsc_num, nrtel);
			return;
		}
		//strcpy_P(buffer, (char*) pgm_read_word(&(comenzi[19])));
		//if (strcasecmp(buffer, inmsg) == 0)
		if (strcasecmp_P(inmsg, STARE_TMP) == 0)
		{
			StareTMP(smsc_num, nrtel);
			return;
		}

		//strcpy_P(buffer, (char*) pgm_read_word(&(comenzi[20])));
		//if (strcasecmp(buffer, inmsg) == 0)
		if (strcasecmp_P(inmsg, STARE_ALL) == 0)
		{
			StareTMP(smsc_num, nrtel);
			StareOUT(smsc_num, nrtel);
			return;
		}

		ReadEprom(buffer, 486);
		uart_sendsms(smsc_num, nrtel, buffer);
		return;
	}
	else
	{
		ReadEprom(buffer, 18 * 19);
		if (strcmp(buffer, inmsg) == 0)
		{
			eeprom_write_block(nrtel, (int*) 18, 18);
			strcpy_P(buffer, LOGIN);
			uart_sendsms(smsc_num, nrtel, buffer);
		}
	}
}

//write the sms string for commands
void Config(char *nrtel, char *inmsg)
{
	char buffer[18];
	memset(buffer, 0, sizeof(buffer));
	int adr = 18;

	if ((strlen(nrtel) != 0) && (strlen(inmsg) != 0))
	{

		//strcpy_P(buffer, (char*) pgm_read_word(&(comenzi[17])));
		if (strstr_P(inmsg, LOGIN) != 0)
		{
			eeprom_write_block(nrtel, (int*) adr, 18);
			CfgCmd(inmsg);

		}
		else
		{
			ReadEprom(buffer, 18);
			if (strcmp(nrtel, buffer) == 0)
				CfgCmd(inmsg);
		}
	}
}

static void StareOUT(char *smsc_nr, char *nrtel)
{
	char mesage1[60];
	char mesage2[60];
	char buffer[18];
	memset(mesage1, 0, sizeof(mesage1));
	memset(mesage2, 0, sizeof(mesage2));
	memset(buffer, 0, sizeof(buffer));

	//int i = 108;
	//if (digitalRead(outD1) == LOW)
	if ((PIND & (1 << PD2)) == 0)
	{
		ReadEprom(buffer, 108);
		if (strlen(buffer) != 0)
		{
			strcat(mesage1, buffer);
			strcat(mesage1, "\n\r");
		}

		//i += 18;
	}
	else
	{
		ReadEprom(buffer, 126);
		if (strlen(buffer) != 0)
		{
			strcat(mesage1, buffer);
			strcat(mesage1, "\n\r");
		}
	}

	//if (digitalRead(outD2) == LOW)
	if ((PIND & (1 << PD3)) == 0)
	{
		ReadEprom(buffer, 144);
		if (strlen(buffer) != 0)
		{
			strcat(mesage1, buffer);
			strcat(mesage1, "\n\r");
		}
	}
	else
	{
		ReadEprom(buffer, 162);
		if (strlen(buffer) != 0)
		{
			strcat(mesage1, buffer);
			strcat(mesage1, "\n\r");
		}
	}

	//if (digitalRead(outD3) == LOW)
	if ((PIND & (1 << PD4)) == 0)
	{
		ReadEprom(buffer, 180);
		if (strlen(buffer) != 0)
		{
			strcat(mesage1, buffer);
			strcat(mesage1, "\n\r");
		}
	}
	else
	{
		ReadEprom(buffer, 198);
		if (strlen(buffer) != 0)
		{
			strcat(mesage1, buffer);
			strcat(mesage1, "\n\r");
		}
	}

	//if (digitalRead(outD4) == LOW)
	if ((PIND & (1 << PD5)) == 0)
	{
		ReadEprom(buffer, 216);
		if (strlen(buffer) != 0)
		{
			strcat(mesage2, buffer);
			strcat(mesage2, "\n\r");
		}
	}
	else
	{
		ReadEprom(buffer, 234);
		if (strlen(buffer) != 0)
		{
			strcat(mesage2, buffer);
			strcat(mesage2, "\n\r");
		}
	}

	//if (digitalRead(outD5) == LOW)
	if ((PIND & (1 << PD6)) == 0)
	{
		ReadEprom(buffer, 252);
		if (strlen(buffer) != 0)
		{
			strcat(mesage2, buffer);
			strcat(mesage2, "\n\r");
		}
	}
	else
	{
		ReadEprom(buffer, 270);
		if (strlen(buffer) != 0)
		{
			strcat(mesage2, buffer);
			//strcat(mesage, "\n");
		}
	}
	if (strlen(mesage1) != 0)
		uart_sendsms(smsc_nr, nrtel, mesage1);
	if (strlen(mesage2) != 0)
		uart_sendsms(smsc_nr, nrtel, mesage2);
}

/**
 * @breaf calculate the temperature in grads Celsius for a 10K termistor
 *
 * @param	Tpin analog input pin wher temp is read
 */
static float Thermistor(int Tpin)
{
	float Vo;
	float logRt, Rt, T;
	float R = 9870; // fixed resistance, measured with multimeter
	// c1, c2, c3 are calibration coefficients for a particular thermistor
	float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
	//Vo = analogRead(Tpin);
	Vo = ReadADC(Tpin);
	Rt = R * (1024.0 / Vo - 1.0);
	logRt = log(Rt);
	T = (1.0 / (c1 + c2 * logRt + c3 * logRt * logRt * logRt)) - 273.15;
	return T;
}

/**
 * @breaf read the temperature on analog pins (10K termistor) and send sms with
 * teaded temperature
 *
 * @param smsc_nr: sms center number
 * 		  nrtel: number hu recive sms
 */
static void StareTMP(char *smsc_nr, char *nrtel)
{
	char mesage[64];
	char buffer[18];
	char tmpe[32];
	memset(mesage, 0, sizeof(mesage));
	memset(buffer, 0, sizeof(buffer));
	memset(tmpe, 0, sizeof(tmpe));
	int tmp, tmp1, tmp2;

	tmp = Thermistor(PC0);
	_delay_ms(30);
	tmp1 = Thermistor(PC0);
	_delay_ms(30);
	tmp2 = Thermistor(PC0);
	tmp = (tmp + tmp1 + tmp2) / 3;

	ReadEprom(buffer, 18 * 16);
	if (strlen(buffer) != 0)
	{
		sprintf(tmpe, "%s: %d %s", buffer, tmp, "C\r\n");
		strcat(mesage, tmpe);
	}

	tmp = Thermistor(PC1);
	_delay_ms(30);
	tmp1 = Thermistor(PC1);
	_delay_ms(30);
	tmp2 = Thermistor(PC1);
	tmp = (tmp + tmp1 + tmp2) / 3;

	ReadEprom(buffer, 18 * 17);
	if (strlen(buffer) != 0)
	{
		sprintf(tmpe, "%s: %d %s", buffer, tmp, "C\r\n");
		strcat(mesage, tmpe);
	}

	tmp = Thermistor(PC2);
	_delay_ms(30);
	tmp1 = Thermistor(PC2);
	_delay_ms(30);
	tmp2 = Thermistor(PC2);
	tmp = (tmp + tmp1 + tmp2) / 3;

	ReadEprom(buffer, 18 * 18);
	if (strlen(buffer) != 0)
	{
		sprintf(tmpe, "%s: %d %c", buffer, tmp, 'C');
		strcat(mesage, tmpe);
	}
	if (strlen(mesage) != 0)
		uart_sendsms(smsc_nr, nrtel, mesage);

}

void VerificIN()
{
	char smsc_nr[18];
	char nrtel[18];
	char buffer[18];
	memset(nrtel, 0, sizeof(nrtel));
	memset(buffer, 0, sizeof(buffer));
	memset(smsc_nr, 0, sizeof(smsc_nr));

	if ((PINB & (1 << PB0)) == 0)
	{
		if (in1)
		{
			ReadEprom(smsc_nr, 360);
			ReadEprom(nrtel, 18);
			in1 = 0;
			ReadEprom(buffer, 18 * 2);
			if (strlen(buffer) != 0)
			uart_sendsms(smsc_nr, nrtel, buffer);
		}
	}
	else
		in1 = 1;

	//if (digitalRead(inD2) == LOW && in2)
	if ((PINB & (1 << PB1)) == 0)
	{
		if (in2)
		{
			ReadEprom(smsc_nr, 360);
			ReadEprom(nrtel, 18);
			in2 = 0;
			ReadEprom(buffer, 18 * 3);
			if (strlen(buffer) != 0)
				uart_sendsms(smsc_nr, nrtel, buffer);
		}
	}
	else
		in2 = 1;

	//if (digitalRead(inD3) == LOW && in3)
	if ((PINB & (1 << PB2)) == 0)
	{
		if (in3)
		{
			ReadEprom(smsc_nr, 360);
			ReadEprom(nrtel, 18);
			in3 = 0;
			ReadEprom(buffer, 18 * 4);
			if (strlen(buffer) != 0)
				uart_sendsms(smsc_nr, nrtel, buffer);
		}
	}
	else
		in3 = 1;

	//if (digitalRead(inD4) == LOW && in4)
	if ((PINB & (1 << PB3)) == 0)
	{
		if (in4)
		{
			ReadEprom(smsc_nr, 360);
			ReadEprom(nrtel, 18);
			in4 = 0;
			ReadEprom(buffer, 18 * 5);
			if (strlen(buffer) != 0)
				uart_sendsms(smsc_nr, nrtel, buffer);
		}
	}
	else
		in4 = 1;

}

