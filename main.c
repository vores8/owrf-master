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

#include <debugprint.h>
#include <owrfreadwrite.h>

static uint8_t transfer_buf[255];

int bytesRead = 0;
uint8_t channel = -1;
int phase = 0;

int main(void) {

	uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(57600, F_CPU));

	nrf24l01_init();
	nrf24l01_settxaddr((uint8_t*) "OWRF0");
	nrf24l01_setrxaddr(0, (uint8_t*) "OWRF0");
	nrf24l01_setrxaddr(1, (uint8_t*) "OWRF1");
	nrf24l01_setrxaddr(2, (uint8_t*) "OWRF2");
	nrf24l01_setrxaddr(3, (uint8_t*) "OWRF3");
	nrf24l01_setrxaddr(4, (uint8_t*) "OWRF4");
	nrf24l01_setrxaddr(5, (uint8_t*) "OWRF5");
//	nrf24l01_enabledynamicpayloads();

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
				debugPrint(
						"received command channel=%02X cmd=%02X len=%02X\r\n",
						channel, transfer_buf[2], transfer_buf[3]);
				bytesRead = 0;
				if (transfer_buf[3] == 0) {
					phase = 4;
				} else {
					phase = 3;
				}
			}
		}
			break;
		case 3: {
			if (bytesRead == transfer_buf[3]) {
				phase = 4;
			} else {
				if (uart_available() > 0) {
					databyte = uart_getc();
					*(transfer_buf + bytesRead + 4) = databyte;
					bytesRead++;
				}
			}
		}
			break;
		case 4: {
			if (channel >= 0) {
				uint8_t len = transfer_buf[3] + 3;
				char* addr = "OWRF1";
				addr[4] += channel;

				debugPrint("send to %s %d\r\n", addr, len);
				for (int i = 0; i < len; i++) {
					debugPrint(">%02X", transfer_buf[i + 1]);
				}
				debugPrint("\r\n");

//				nrf24l01_settxaddr((uint8_t*)"OWRF0");
				if (owrf_write(transfer_buf + 1, len ) == 1) {
					debugPrint("OK\r\n");
					phase = 5;
				} else {
					debugPrint("FAIL\r\n");
					phase = 6;
				}
			}
		}
			break;
		case 5: {
//			nrf24l01_setrxaddr(0, (uint8_t*) "OWRF1");

			int length = -1;
			length = owrf_read(transfer_buf);
//			nrf24l01_setrxaddr(0, (uint8_t*) "OWRF0");
			if (length != -1) {
				for (int i = 0; i < length; i++) {
					uart_putc(transfer_buf[i]);
					debugPrint("<%02X", transfer_buf[i]);
				}
				debugPrint("\r\n");
				uart_flush();
				phase = 7;
			} else {
				debugPrint("TIMEOUT\r\n");
				phase = 6;
			}
		}
			break;
		case 6: {
			uart_putc(channel);
			uart_putc(0xFF);
			uart_putc(0x00);
			uart_flush();
			phase = 0;
			bytesRead = 0;
		}
			break;
		case 7: {
			phase = 0;
			bytesRead = 0;
		}
			break;
		}
	}
}

