// Author: Yubo Zhi (normanzyb@gmail.com)

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
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

void test_tx_task(void *param)
{
	static char string[] = "Station ?, No ??????: Hi!";
	static char buffer[NET_PACKET_MAX_SIZE];
	const uint16_t strlength = sizeof(string);
	string[8] = mac_address();
	printf_P(PSTR("\e[96mTest TX task initialised (%02x), hello world!\n"), mac_address());
	uint16_t count = 0;

loop:
	sprintf(buffer, "%6u", count);
	memcpy(string + 8 + 6, buffer, 6);
	if (!(PINC & _BV(2))) {
		uart0_lock();
		printf_P(PSTR("\e[91mStation %02x, sent %u(%u bytes)\e[0m\n"), mac_address(), count++, strlength);
		uart0_unlock();
		mac_write(PINA, (void *)string, strlength);
	}
	while (!mac_written());
	vTaskDelay(10);	// Waiting for data reception
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
	ptr = data.payload;
	uart0_lock();
	printf_P(PSTR("\e[92mReceived from %02x: "), data.addr);
	uint8_t len = data.len;
	while (len--) {
		uint8_t c = *ptr++;
		if (isprint(c))
			putchar(c);
		else
			printf("\\x%02x", c);
	}
	printf_P(PSTR("\e[0m\n"));
	uart0_unlock();
	vPortFree(data.ptr);
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

	DDRA = 0x00;
	PORTA = 0xff;

	phy_init();
	mac_init();
	sei();
}

int main()
{
	init();

	xTaskCreate(test_tx_task, "Test TX", 180, NULL, tskAPP_PRIORITY, NULL);
	xTaskCreate(test_rx_task, "Test RX", 180, NULL, tskAPP_PRIORITY, NULL);
	vTaskStartScheduler();
	return 1;
}
