/*
 nrf24l01 lib sample

 copyright (c) Davide Gironi, 2012

 Released under GPLv3.
 Please refer to LICENSE file for licensing information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <uart.h>

#include <nrf24l01.h>

#include "debugprint.h"

static uint8_t transfer_buf[64];

int bytesRead = 0;
uint8_t channel = -1;
uint8_t* msg;
int phase = 0;

int main(void) {

	msg = transfer_buf;

	uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(57600, F_CPU));

	nrf24l01_init();
	nrf24l01_settxaddr((uint8_t*) "OWRF0");
	nrf24l01_setrxaddr(0, (uint8_t*) "OWRF0");
	nrf24l01_setrxaddr(1, (uint8_t*) "OWRF1");
	nrf24l01_setrxaddr(2, (uint8_t*) "OWRF2");
	nrf24l01_setrxaddr(3, (uint8_t*) "OWRF3");
	nrf24l01_setrxaddr(4, (uint8_t*) "OWRF4");
	nrf24l01_setrxaddr(5, (uint8_t*) "OWRF5");
	nrf24l01_enabledynamicpayloads();

	sei();

	for (;;) {
		int databyte;
		switch (phase) {
		case 0: {
			if (uart_available()) {
				databyte = uart_getc();
				if (bytesRead == 0) {
					if (databyte == 0xDD)
						transfer_buf[bytesRead++] = databyte;
				} else {
					transfer_buf[bytesRead++] = databyte;
				}
			}
			if (bytesRead == 4) {
				channel = transfer_buf[1];
				msg = transfer_buf + 2;
				debugPrint(
						"received command channel=%02X cmd=%02X len=%02X\r\n",
						channel, msg[0], msg[1]);
				bytesRead = 0;
				if (msg[1] == 0) {
					phase = 4;
				} else {
					phase = 3;
				}
			}
		}
			break;
		case 3: {
			if (bytesRead == msg[1]) {
				phase = 4;
			} else {
				if (uart_available() > 0) {
					databyte = uart_getc();
					*(msg + bytesRead + 2) = databyte;
					bytesRead++;
				}
			}
		}
			break;
		case 4: {
			if (channel >= 0) {
				uint8_t tosend[32];
				uint8_t len = msg[1] + 2;
				memcpy(tosend, msg, len);
				for (int i = 0; i < len; i++) {
					debugPrint(">%02X", tosend[i]);
				}
				debugPrint("\r\n");
				char* addr = "OWRF1";
				addr[4] += channel;
				debugPrint("send to %s %d:", addr, len);
				nrf24l01_settxaddr((uint8_t*)addr);
//				nrf24l01_setrxaddr(0, (uint8_t*)addr);
				uint8_t writeret = nrf24l01_write(tosend, len);
//				nrf24l01_setrxaddr(0, (uint8_t*) "OWRF0");
//				nrf24l01_setrxaddr(2, (uint8_t*) "OWRF0");
				if (writeret == 1) {
					debugPrint("OK\r\n");
					phase = 5;
				} else {
					debugPrint("FAIL\r\n");
					phase = 0;
					bytesRead = 0;
					msg = transfer_buf;
				}
			}
		}
			break;
		case 5: {
//			phase = 0;
//			bytesRead = 0;
//			msg = transfer_buf;
//			nrf24l01_settxaddr((uint8_t*) "OWRF0");
			uint8_t pipe = 0xff;
			uint8_t timeout = 10;
			while (!nrf24l01_readready(&pipe) && (timeout > 0)) {
				_delay_ms(5);
				timeout--;
			}
			if (timeout > 0) {
				int len = nrf24l01_getdynamicpayloadsize();
				nrf24l01_read(transfer_buf, len);
//				uart_putc(pipe);
				debugPrint("channel = %d outlen=%d\r\n", pipe, len);
				for (int i = 0; i < len; i++) {
					uart_putc(transfer_buf[i]);
					debugPrint("<%02X", transfer_buf[i]);
				}
				debugPrint("\r\n");
				uart_flush();
			} else {
				debugPrint("TIMEOUT\r\n");
			}
			phase = 0;
			bytesRead = 0;
			msg = transfer_buf;
		}
			break;
		}
	}
}

