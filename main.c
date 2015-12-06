// Author: Yubo Zhi (normanzyb@gmail.com)

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <uart0.h>
#include "phy_layer.h"
#include "mac_layer.h"
#include "llc_layer.h"
#include "net_layer.h"

#include <task.h>

static TaskHandle_t appTask, rxTask;

// Task notify event bits
#define EVNT_QUEUE_RX	0x00000001
#define EVNT_UART_RX	0x00000002

ISR(USART0_RX_vect)
{
	BaseType_t xTaskWoken = pdFALSE;
	xTaskNotifyFromISR(appTask, EVNT_UART_RX, eSetBits, &xTaskWoken);
	// Disable UART0 RXC interrupt
	uart0_interrupt_rxc(0);

	if (xTaskWoken)
		taskYIELD();
}

void app_rx_task(void *param)
{
	static struct net_packet_t pkt;
	puts_P(PSTR("\e[96mAPP RX task initialised."));

loop:
	while (xQueueReceive(net_rx, &pkt, 0) != pdTRUE);
	uint8_t *ptr = pkt.payload;
	uint8_t len = pkt.len;

	uart0_lock();
	printf_P(PSTR("\e[92mReceived from %02x: "), pkt.addr);
	while (len--) {
		uint8_t c = *ptr++;
		if (isprint(c))
			putchar(c);
		else
			printf("\\x%02x", c);
	}
	putchar('\n');
	uart0_unlock();
	vPortFree(pkt.ptr);

	goto loop;
}

void app_report(void)
{
	uint16_t total = configTOTAL_HEAP_SIZE;
	uint16_t free = xPortGetFreeHeapSize();
	uint16_t usage = (float)(total - free) * 100. / configTOTAL_HEAP_SIZE;
	uart0_lock();
	printf_P(PSTR("\e[97mReport: heap (free: %u, total: %u, usage: %u%%)\n"), free, total, usage);
	uart0_unlock();
}

void app_task(void *param)
{
	static char string[] = "Station ?, No ??????. Hello, world! This is ELEC3222-A1 group. The DLL frame is 32 bytes maximum, but NET packet can be 128 bytes";
	static uint8_t dest = 0;
	static char buffer[7];
	uint8_t report = 0;
	uint16_t count = 0;
	uint32_t notify;
	string[8] = net_address();
	// Enable UART0 RXC interrupt
	uart0_interrupt_rxc(1);
	printf_P(PSTR("\e[96mAPP task initialised (%02x/%02x), hello world!\n"), net_address(), mac_address());

poll:
	// No event notify received
	if (xTaskNotifyWait(0, ULONG_MAX, &notify, 20) != pdTRUE) {
		// Start transmission
		if (!(PINC & _BV(2)) && llc_written()) {
			sprintf(buffer, "%6u", count);
			memcpy(string + 8 + 6, buffer, 6);
			uint8_t len;
			len = sizeof(string) > 121 ? 121 : sizeof(string);

			uint8_t status = 0;
			do {
				status = net_tx(dest, len, (uint8_t *)string);

				uart0_lock();
				printf_P(PSTR("\e[91mStation %02x/%02x, "), net_address(), mac_address());
				printf_P(PSTR("sent %u(DEST: %02x, SIZE: %u): "), count, dest, len);
				puts_P(status ? PSTR("SUCCESS") : PSTR("\e[31mFAILED"));
				uart0_unlock();
			} while (!status);
			count++;
		}

		// Report memory usage
		if (report == 0)
			app_report();
		report = report == 9 ? 0 : report + 1;
		goto poll;
	}

	// Received 1 character from UART
	if (notify & EVNT_UART_RX) {
		int c = uart0_read_unblocked();
		uart0_interrupt_rxc(1);
		uint8_t num = 0xff;
		switch (c) {
		case -1:
			break;
		case '0' ... '9':
			num = c - '0';
			break;
		case 'A' ... 'F':
			num = c - 'A' + 10;
			break;
		case 'a' ... 'f':
			num = c - 'a' + 10;
			break;
		case 'r':
		case 'R':
			app_report();
			break;
		}
		// Update destination address
		if (num != 0xff)
			dest = (dest << 4) | num;
	}

	goto poll;
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
	llc_init();
	net_init();
	sei();
}

int main()
{
	init();

	while (xTaskCreate(app_rx_task, "APP RX", 180, NULL, tskAPP_PRIORITY, &rxTask) != pdPASS);
	while (xTaskCreate(app_task, "APP task", 180, NULL, tskAPP_PRIORITY, &appTask) != pdPASS);

	vTaskStartScheduler();
	return 1;
}
