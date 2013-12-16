/*
 * fbus.c
 *
 * Create on: Jul 31, 2013
 * 		Author: popai
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
//#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "../usart/usart.h"
#include "fbus.h"

uint8_t table[]PROGMEM =
{
/* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
'@', 0xa3, '$', 0xa5, 0xe8, 0xe9, 0xf9, 0xec, 0xf2, 0xc7, '\n', 0xd8, 0xf8,
		'\r', 0xc5, 0xe5, '?', '_', '?', '?', '?', '?', '?', '?', '?', '?', '?',
		'?', 0xc6, 0xe6, 0xdf, 0xc9, ' ', '!', '\"', '#', 0xa4, '%', '&', '\'',
		'(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4', '5',
		'6', '7', '8', '9', ':', ';', '<', '=', '>', '?', 0xa1, 'A', 'B', 'C',
		'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
		'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0xc4, 0xd6, 0xd1, 0xdc,
		0xa7, 0xbf, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
		'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		0xe4, 0xf6, 0xf1, 0xfc, 0xe0 };

static uint8_t bcd(uint8_t *dest, const char *s);
static uint8_t pack7(uint8_t *dest, const char *s);
static void unbcd_phonenum(char *data, char *phonenum_buf);
static void unpack7_msg(char *data, uint8_t len, char *msg_buf);
//static void print(const uint8_t *buf, uint8_t len) __attribute__ ( ( always_inline ) );
//static inline void print(const uint8_t *buf, uint8_t len)


inline static void print(const uint8_t *buf, uint8_t len)
{
	while (len--)
	{
		//uart_putchar(*buf++, NULL);
		UWriteData(*buf++);
		//UWriteInt(*buf++);
		//UWriteData(' ');
	}
}

void fbus_init(void)
{
	uint8_t i;
	for (i = 0; i < 128; i++)
	{
		//uart_putchar('U', NULL);
		UWriteData('U');
		_delay_ms(3);
	}
	_delay_ms(300);
}

void sendframe(uint8_t type, uint8_t *data, uint8_t size)
{
	uint8_t buf[200];
	uint8_t at; //, len, n;
	//uint16_t check;
	//uint16_t *p;
	//PORTB = (1 << PB4);
	at = 0;

	// build header
	buf[at++] = 0x1e; // message startbyte
	buf[at++] = 0x00; // dest: phone
	buf[at++] = 0x0c; // source: PC
	buf[at++] = type; /* Type */
	buf[at++] = 0x00; /* Length */
	buf[at++] = size; /* Length */

	/* Copy in data if any. */
	if (size != 0)
	{
		memcpy(buf + at, data, size);
		at += size;
	}

	/* If the message length is odd we should add pad byte 0x00 */
	if (size % 2)
		buf[at++] = 0x00;

	/* calculate checksums
	 check = 0;
	 p = (uint16_t *) buf;
	 len = at / 2;
	 for (n = 0; n < len; ++n)
	 check ^= p[n];
	 p[n] = check;
	 at += 2;
	 */

	/* Now calculate checksums over entire message and append to message. */
	/* Odd bytes */
	unsigned char checksum = 0;
	checksum = 0;
	int count;
	for (count = 0; count < at; count += 2)
		checksum ^= buf[count];

	buf[at++] = checksum;

	/* Even bytes */

	checksum = 0;
	for (count = 1; count < at; count += 2)
		checksum ^= buf[count];

	buf[at++] = checksum;

	// send the message!
	print(buf, at);
	_delay_ms(500);
	//PORTB &= ~(1 << PB4);
}

void uart_sendsms(const char *smsc_num, const char *num, const char *ascii)
//void uart_sendsms(const char *smsc_num, const char *num, uint16_t *ascii)
{
	uint8_t buf[160];
	//memset(buf, 0, sizeof(buf));
	//sendack(0x02, 0x02);

	uint8_t len = 0;
	PORTB |= (1 << PB4);
	PORTB &= ~(1 << PB5);
	buf[len++] = 0x00;
	buf[len++] = 0x01;
	buf[len++] = 0x00; //SMS frame header
	buf[len++] = 0x01;
	buf[len++] = 0x02;
	buf[len++] = 0x00; //send SMS message

	memset(buf + len, 0, 12);
	//buf[len++] = strlen(smsc_num);
	buf[len++] = 0x07; //SMS Centre number length.
	buf[len++] = 0x91; //SMSC number type e.g. 0x81-unknown 0x91-international 0xa1-national
	//len += bcd(buf + len, "+40744946000"); // + 1; //include the type-of-address
	len += bcd(buf + len, smsc_num);
	//len += 10;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;

	buf[len++] = 0x11; //0x15; //SMS Submit, Reject Duplicates, Validity Indicator present

	buf[len++] = 0x00; //message reference
	buf[len++] = 0x00; //protocol id
	buf[len++] = 0x00; //data coding scheme 0x08 - ne comprimat;

	buf[len++] = strlen(ascii); //data len

	memset(buf + len, 0, 12);
	//buf[len++] = strlen(num);
	buf[len++] = 0x0B;
	buf[len++] = 0x91;
	len += bcd(buf + len, num);
	//len += 10;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;

	buf[len++] = 0xa9; //validity period, 4 days

	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	buf[len++] = 0x00;
	/*
	 buf[len++] = 0x00;
	 buf[len++] = 0xEF;
	 buf[len++] = 0x00;
	 buf[len++] = 0xBB;
	 buf[len++] = 0x00;
	 buf[len++] = 0xBF;
	 */

	len += pack7(buf + len, ascii); //This is the SMS message packed into 7 bit characters.
	/*
	 for (int i = 0; i < strlen(ascii); i++)
	 {
	 buf[len++] = 0x00;
	 buf[len++] = ascii[i];
	 }
	 */
	buf[len++] = 0x01; //terminator. Always 0x00 ????
	buf[len++] = 0x41; //seq num			????

	/*if (len % 2)
	 {
	 buf[len] = 0x00; //padding, but don't increment len
	 }*/

	//cli();	//make Atomic execurion

	fbus_init();
	uint8_t init[] =
	{ 0x00, 0x01, 0x64, 0x01, 0x01, 0x40 };
	sendframe(0x40, init, 6);
	sendack(0x14, 0x00);

	sendframe(TYPE_SMS, buf, len);
	//sendack(0x02, 0x07);

	uint8_t init_sms[] =
	{ 0x00, 0x01, 0x00, 0x07, 0x02, 0x01, 0x01, 0x64, 0x01, 0x47 };
	sendframe(0x02, init_sms, 0x0a);
	sendack(0x14, 0x00);

	//sei();	//eand Atomic execurion

	PORTB &= ~(1 << PB4);

}

/**
 * @breaf mentain fbus comunication
 *
 * @param no parametrs
 */
void fbus_ack()
{
	fbus_init();
	uint8_t init_sms[] =
	{ 0x00, 0x01, 0x00, 0x07, 0x02, 0x01, 0x01, 0x64, 0x01, 0x47 };
	sendframe(0x02, init_sms, 0x0a);
	sendack(0x14, 0x00);

}


void sendack(uint8_t type, uint8_t seqnum)
{
	uint8_t buf[2];

	buf[0] = type;
	buf[1] = seqnum;

	sendframe(TYPE_ACK, buf, 2); //sizeof(buf) / sizeof(buf[0]));
}

enum fbus_frametype fbus_readframe(char *phonenum_buf, char *msg_buf)
{

	char buf[RECEIVE_BUFF_SIZE];
	memset(buf, 0, sizeof(buf));
	SerialRead(buf, RECEIVE_BUFF_SIZE);

	char *start;
	start = buf;
	uint8_t i = strlen(buf) - 10;
	while (i--)
	{
		if (start[0] == 0x1e && start[1] == 0x0c && start[2] == 0x00)
		{
			PORTB |= (1 << PB4);
			PORTB &= ~(1 << PB5);

			if (start[3] == TYPE_SMS)
			{
				if (start[5] > 128)
				{
					PORTB |= (1 << PB5);
					return FRAME_UNKNOWN;
				}
				uint8_t seq_no = start[start[5] - 1];
				sendack(TYPE_SMS, seq_no & 0x0f);
				if (start[9] == 0x10)
				{
					unbcd_phonenum(start + 29, phonenum_buf);
					unpack7_msg(start + 48, start[28], msg_buf);
					fbus_delete_sms(start[10], start[11]);
					PORTB &= ~(1 << PB4);
					return FRAME_SMS_RECV;
				}
				else start++;
			}
		else
				start++;
		}
		else
			start++;
	}
	if (strlen(buf) != 0)
		PORTB |= (1 << PB5);	//eror
	return FRAME_UNKNOWN;
}


enum fbus_frametype fbus_readack(char *smsc_nr)
{

	char buf[64];
	memset(buf, 0, sizeof(buf));
	SerialRead(buf, 64);

	char *start;
	start = buf;
	uint8_t i = strlen(buf) - 10;
	while (i--)
	{
		if (start[0] == 0x1e && start[1] == 0x0c && start[2] == 0x00)
		{
            //UWriteString(" DA ");
            uint8_t type = start[3];
            uint8_t len = start[5];
            if (len > 128)
            {
                    PORTB |= (1 << PB5);
                    return FRAME_UNKNOWN;
            }
            if (type == TYPE_SMS && start[9] == 0x10)
            {
                    return FRAME_SMS_RECV;
            }
            else if (type == TYPE_SMS && start[9] == 0x02)
            {
                    return FRAME_SMS_SENT;
            }
            else if (type == TYPE_SMS_MGMT && start[9] == 0x09)
            {
                    return FRAME_SMS_MGMT;
            }
            else if (type == TYPE_SMS && start[9] == 0x03)
            {
                    return FRAME_SMS_ERROR;
            }
            else if (type == TYPE_ACK)
            {
                    return FRAME_ACK;
            }
            else if (type == TYPE_ID)
            {
                    return FRAME_ID;
            }
            else if (type == TYPE_NET_STATUS)
            {
                    return FRAME_NET_STATUS;
            }
		}
		else
			start++;
	}
	if (strlen(buf) != 0)
		PORTB |= (1 << PB5);	//eror
	return FRAME_UNKNOWN;

}

void fbus_delete_sms(uint8_t memory_type, uint8_t storage_loc)
{
	uint8_t del_sms[] =
	{ 0x00, 0x01, 0x00, 0x0a, memory_type, storage_loc, 0x01, 0x41 };
	sendframe(TYPE_SMS_MGMT, del_sms, sizeof(del_sms));
}

void fbus_getsmsc(char *smsc_nr)
{
	uint8_t buf[] =
	{ 0x00, 0x01, 0x00, 0x33, 0x64, 0x01, 0x01, 0x47 };
	UFlushBuffer();
	//sendack(TYPE_SMS, 0x0C);
	sendframe(0x02, buf, 8);

	char ackbuf[64];
	memset(ackbuf, 0, sizeof(ackbuf));
	SerialRead(ackbuf, 64);

	/*
	 char ackbuf[] = {0x1E, 0x0C, 0x00, 0x7F, 0x00, 0x02, 0x02, 0x07, 0x1C, 0x76, 0x1E, 0x0C, 0x00, 0x02, 0x00, 0x2F,
	 0x01, 0x08, 0x00, 0x34, 0x01, 0xFD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x91, 0x04, 0x47, 0x94, 0x64, 0x00, 0xF0, 0x00, 0x00, 0x00,
	 0x00, 0x4F, 0x72, 0x61, 0x6E, 0x67, 0x65, 0x20, 0x53, 0x4D, 0x53, 0x43, 0x00, 0x01, 0x41, 0x00,
	 0x64, 0x11};
	 */

	char *start;
	start = ackbuf;
	/*
	 if (ackbuf[3] == 0x7f)
	 start += 10;
	 */
	uint8_t i = strlen(ackbuf) - 10;
	while (i--)
	{
		if (start[0] == 0x1e && start[1] == 0x0c && start[2] == 0x00)
		{
			//PORTD |= (1 << PD2);
			if (start[3] == TYPE_SMS && start[9] == 0x34)
			{
				//PORTD |= (1 << PD3);
				start[27] = 0x0b;
				unbcd_phonenum(start + 27, smsc_nr);
				return;
			}
			else
				start++;

		}
		else
			start++;
	}
	PORTB |= (1 << PB5); //eror
}

void BatteryState()
{
	uint8_t getbatt[] =
	{ 0x00, 0x01, 0x00, 0x02, 0x01, 0x63 };
	UFlushBuffer();
	sendframe(0x17, getbatt, 6);

	char ackbuf[64];
	memset(ackbuf, 0, sizeof(ackbuf));
	_delay_ms(600);
	SerialRead(ackbuf, 64);

	char *start;
	start = ackbuf;
	if (ackbuf[3] == 0x7f)
		start += 10;

	//unsigned char s1[50];
	uint8_t Battery = 0;

	if (start[0] == 0x1e && start[1] == 0x0c && start[2] == 0x00)
	{
		if (start[3] == 0x17 && start[9] == 0x03)
		{
			// Battery charge
			Battery = start[11];
			//Battery = itoa((int) Battery, s1, 10);
			//printf("Battery     %s Percent\r\n", itoa((int) Battery, s1, 10));
			if (Battery == 98)
			{
				PORTD |= (1 << PD7);
				return;
			}

			else
			{
				PORTD &= ~(1 << PD7);
				return;
			}

		}
	}
}

static uint8_t bcd(uint8_t *dest, const char *s)
{
	uint8_t x, y, n, hi, lo;

	x = 0;
	y = 0;
	if (*s == '+')
	{
		s++;
	}
	while (s[x])
	{
		lo = s[x++] - '0';
		if (s[x])
		{
			hi = s[x++] - '0';
		}
		else
		{
			hi = 0x0f;
		}

		n = (hi << 4) + lo;
		dest[y++] = n;
		//UWriteInt(n);
		//UWriteData(' ');
	}
	//UWriteData(0x0D);
	//UWriteData(0x0A);
	return y;
}

//char phonenum_buf[16];
static void unbcd_phonenum(char *data, char *phonenum_buf)
{
	uint8_t len, n, x, at;
	char *endptr = phonenum_buf;

	len = data[0];

	if (data[1] == 0x6f || data[1] == 0x91)
	{
		*endptr++ = '+';
	}

	at = 2;
	for (n = 0; n < len; ++n)
	{
		x = data[at] & 0x0f;
		if (x < 10)
		{
			*endptr++ = '0' + x;
		}
		n++;
		if (n >= len)
		{
			break;
		}
		x = (data[at] >> 4) & 0x0f;
		if (x < 10)
		{
			*endptr++ = '0' + x;
		}
		at++;
	}
	*endptr = '\0';
}

static uint8_t escaped(uint8_t c)
{
	switch (c)
	{
	case 0x0a:
		return '\n';
	case 0x14:
		return '^';
	case 0x28:
		return '{';
	case 0x29:
		return '}';
	case 0x2f:
		return '\\';
	case 0x3c:
		return '[';
	case 0x3d:
		return '~';
	case 0x3e:
		return ']';
	case 0x40:
		return '|';
	default:
		return '?';
	}
}

//char msg_buf[64];
static void unpack7_msg(char *data, uint8_t len, char *msg_buf)
{
	uint16_t *p, w;
	uint8_t c;
	uint8_t n;
	uint8_t shift = 0;
	uint8_t at = 0;
	uint8_t escape = 0;
	char *endptr = msg_buf;

	for (n = 0; n < len; ++n)
	{
		p = (uint16_t *) (data + at);
		w = *p;
		w >>= shift;
		c = w & 0x7f;

		shift += 7;
		if (shift & 8)
		{
			shift &= 0x07;
			at++;
		}

		if (escape)
		{
			*endptr++ = escaped(c);
			escape = 0;
		}
		else if (c == 0x1b)
		{
			escape = 1;
		}
		else
		{
			//*endptr++ = table[c];
			*endptr++ = pgm_read_byte(&table[ c ]);
		}
	}
	*endptr = '\0';
}

static uint8_t gettrans(uint8_t c)
{
	uint8_t n;
	if (c == '?')
		return 0x3f;
	for (n = 0; n < 128; ++n)
	{
		//if (table[n] == c){
		if (pgm_read_byte(&table[n]) == c)
		{
			return n;
		}
	}
	return 0x3f;
}

static uint8_t pack7(uint8_t *dest, const char *s)
{
	uint8_t len;
	uint16_t *p, w;
	uint8_t at;
	uint8_t shift;
	uint8_t n, x;

	len = strlen(s);
	x = ((uint16_t) len * 8) / 7;
	for (n = 0; n < x; ++n)
		dest[n] = 0;

	shift = 0;
	at = 0;
	w = 0;
	for (n = 0; n < len; ++n)
	{
		p = (uint16_t *) (dest + at);
		w = gettrans(s[n]) & 0x7f;
		w <<= shift;

		*p |= w;

		shift += 7;
		if (shift >= 8)
		{
			shift &= 7;
			at++;
		}
	}
	return len;
}

/*
 uint8_t fbus_sendsms(const char *num, const char *msg)
 {
 enum fbus_frametype f;
 int i = 0;
 uart_sendsms(num, num, msg);
 for (;;)
 {
 ++i;
 f = fbus_readack();
 if (f == FRAME_SMS_SENT)
 return 1;
 else if (i == 4)
 return 0;
 _delay_ms(500);
 }
 return 0;
 }


 enum fbus_frametype fbus_heartbeat(void)
 {
 uint8_t get_info[] =
 { 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x60 };
 enum fbus_frametype type;

 for (;;)
 {
 sendframe(TYPE_GETID, get_info, sizeof(get_info));
 type = fbus_readack();
 if (type != FRAME_READ_TIMEOUT)
 {
 return type;
 }
 else
 {
 PORTB = (1 << PB5);
 //_delay_ms(1000);
 }
 }
 return 0;
 }
 */

