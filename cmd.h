/*
 * cmd.h
 *
 *  Created on: Apr 15, 2013
 *      Author: popai
 */

#ifndef CMD_H_
#define CMD_H_
#define DEBUG 0

int CfgCmd(char *buffer);
void ReadEprom(char* str_citit, int const adresa);
void DellEprom();
//void DellPass();
void Config(char *nrtel, char *inmsg);
void Comand(char *nrtel, char *inmsg);
//void StareIN(char *nrtel);
void VerificIN();
int8_t FON_INIT();
//void StareALL(char *nrtel);

/*
 #define sbi(x,y) x |= _BV(y) //set bit
 #define cbi(x,y) x &= ~(_BV(y)) //clear bit
 #define tbi(x,y) x ^= _BV(y) //toggle bit
 #define is_high(x,y) ((x & _BV(y)) == _BV(y)) //check if the input pin is high
 #define is_low(x,y) ((x & _BV(y)) == 0) //check if the input pin is low
 */



#endif /* CMD_H_ */
