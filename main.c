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
#include "net_layer.h"

#include <task.h>

static TaskHandle_t appTask, rxTask;
static struct mac_frame frame;

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

void test_rx_task(void *param)
{
	puts_P(PSTR("\e[96mAPP RX task initialised."));

loop:
	// Get 1 frame from RX queue
	while (xQueuePeek(mac_rx, &frame, portMAX_DELAY) != pdTRUE);
	// Tell APP to receive data
	xTaskNotify(appTask, EVNT_QUEUE_RX, eSetBits);
	// Waiting for APP receiving data
	while (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 0);
	goto loop;
}

void app_task(void *param)
{
	static char string[] = "Station ?, No ??????: Hi!";
	uint8_t dest = MAC_BROADCAST;
	uint8_t report = 0;
	uint16_t count = 0;
	uint32_t notify;
	string[8] = mac_address();
	// Enable UART0 RXC interrupt
	uart0_interrupt_rxc(1);

	printf_P(PSTR("\e[96mAPP task initialised (%02x), hello world!\n"), mac_address());

poll:
	// No event notify received
	if (xTaskNotifyWait(0, ULONG_MAX, &notify, 10) != pdTRUE) {
		// Start transmission
		if (!(PINC & _BV(2)) && mac_written()) {
			sprintf(string + 8 + 6, "%6u: Hi!", count);
			mac_tx(dest, (void *)string, sizeof(string));

			printf_P(PSTR("\e[91mStation %02x, sent %u(%u bytes)\n"), mac_address(), count++, sizeof(string));
		}

		// Report memory usage
		if (report == 0) {
			uint16_t total = configTOTAL_HEAP_SIZE;
			uint16_t free = xPortGetFreeHeapSize();
			uint16_t used = total - free;
			uint16_t usage = (float)used * 100. / configTOTAL_HEAP_SIZE;
			printf_P(PSTR("\e[97mReport: heap (free: %u, total: %u, usage: %u%%)\n"), free, total, usage);
		}
		report = report == 99 ? 0 : report + 1;
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
		}
		// Update destination address
		if (num != 0xff)
			dest = (dest << 4) | num;
	}

	// Items in RX queue ready for receive
	if (notify & EVNT_QUEUE_RX) {
		while (xQueueReceive(mac_rx, &frame, 0) == pdTRUE) {
			uint8_t *ptr = frame.payload;
			uint8_t len = frame.len;

			printf_P(PSTR("\e[92mReceived from %02x: "), frame.addr);
			while (len--) {
				uint8_t c = *ptr++;
				if (isprint(c))
					putchar(c);
				else
					printf("\\x%02x", c);
			}
			vPortFree(frame.ptr);
			putchar('\n');
		}
		xTaskNotifyGive(rxTask);
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
	sei();
}

int main()
{
	init();

	xTaskCreate(test_rx_task, "Test RX", configMINIMAL_STACK_SIZE, NULL, tskAPP_PRIORITY, &rxTask);
	xTaskCreate(app_task, "APP task", 180, NULL, tskAPP_PRIORITY, &appTask);
	while (rxTask == NULL || appTask == NULL);

	vTaskStartScheduler();
	return 1;
}
