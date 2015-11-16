// Author: Yubo Zhi (normanzyb@gmail.com)

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <uart0.h>
#include "phy_layer.h"
#include "mac_layer.h"
#include "net_layer.h"

#include <task.h>

static EEMEM uint8_t NVStation = 'A' + ADDRESS;
uint8_t station;

void test_tx_task(void *param)
{
	static char string[] = "Station ?, No ??????: Hi!";
	static char buffer[NET_PACKET_MAX_SIZE];
	const uint16_t strlength = sizeof(string);
	string[8] = station;
	printf_P(PSTR("\e[96mTest TX task initialised (%c), hello world!\n"), station);
	uint16_t count = 0;

loop:
	sprintf(buffer, "%6u", count);
	memcpy(string + 8 + 6, buffer, 6);
	if (!(PINC & _BV(2))) {
		printf_P(PSTR("\e[91mStation %c, sent %u(%u bytes)\e[0m\n"), station, count++, strlength);
		mac_write((void *)string, strlength);
	}
	while (!mac_written());
	_delay_ms(10);	// Waiting for data reception
	goto loop;

}

void test_rx_task(void *param)
{
	struct mac_frame data;
	uint8_t *ptr;
	puts_P(PSTR("\e[96mMAC RX task initialised."));

loop:
	// Receive 1 frame from queue
	while (xQueueReceive(mac_rx, &data, portMAX_DELAY) != pdTRUE);
	ptr = data.ptr;
	printf_P(PSTR("\e[92m"));
	uint8_t len = data.len;
	while (len--) {
		uint8_t c = *ptr++;
		if (isprint(c))
			putchar(c);
		else
			printf("\\x%02x", c);
	}
	vPortFree(data.ptr);
	printf_P(PSTR("\e[0m\n"));
	goto loop;
}

void init()
{
	// Disable JTAG
	MCUCR |= 0x80;
	MCUCR |= 0x80;
	DDRC &= ~(_BV(5) | _BV(2));
	PORTC |= (_BV(5) | _BV(2));

	uart0_init();
	stdout = uart0_fd();
	stdin = uart0_fd();
	puts_P(PSTR("\x0c\e[96mStarting up..."));

	phy_init();
	mac_init();
	sei();

	station = eeprom_read_byte(&NVStation);
}

int main()
{
	init();

	xTaskCreate(test_tx_task, "Test TX", 180, NULL, tskAPP_PRIORITY, NULL);
	xTaskCreate(test_rx_task, "Test RX", 180, NULL, tskAPP_PRIORITY, NULL);
	vTaskStartScheduler();
	return 1;
}
