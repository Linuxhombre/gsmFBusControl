/*
 * fbus.h
 *
 *  Created on: Jul 31, 2013
 *      Author: popai
 */

#ifndef FBUS_H_
#define FBUS_H_

#define TYPE_SMS_MGMT 0x14
#define TYPE_SMS 0x02
#define TYPE_ACK 0x7f
#define TYPE_GETID 0xd1
#define TYPE_ID  0xd2
#define TYPE_NET_STATUS 0x0a

void sendframe(uint8_t type, uint8_t *data, uint8_t size);
enum fbus_frametype fbus_readframe(char*, char*);
uint8_t fbus_sendsms(const char *num, const char *msg);
enum fbus_frametype fbus_heartbeat(void);
void fbus_delete_sms(uint8_t memory_type, uint8_t storage_loc);
void uart_sendsms(const char *smsc_num, const char *num, const char *ascii);
enum fbus_frametype fbus_readack();
void sendack(uint8_t type, uint8_t seqnum);
void fbus_getsmsc(char *smsc_nr);
void fbus_init(void);
void fbus_ack();

enum fbus_frametype {
    FRAME_READ_TIMEOUT,
    FRAME_SMS_SENT,
    FRAME_SMS_RECV,
    FRAME_SMS_MGMT,
    FRAME_SMS_ERROR,
    FRAME_ACK,
    FRAME_ID,
    FRAME_NET_STATUS,
    FRAME_UNKNOWN,
};

#endif
