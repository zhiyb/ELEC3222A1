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
#include <colours.h>
#include "socket.h"
#include "tran_layer.h"
#include "phy_layer.h"
#include "mac_layer.h"
#include "llc_layer.h"
#include "net_layer.h"

#include <task.h>

// Task notify event bits
#define EVNT_QUEUE_RX	0x00000001
#define EVNT_UART_RX	0x00000002

#define PORT_TEXT	233
#define PORT_GPIO	123

static TaskHandle_t appTask, rxTask;
static uint8_t sid_text;

ISR(USART0_RX_vect)
{
	BaseType_t xTaskWoken = pdFALSE;
	xTaskNotifyFromISR(appTask, EVNT_UART_RX, eSetBits, &xTaskWoken);
	// Disable UART0 RXC interrupt
	uart0_interrupt_rxc(0);

	if (xTaskWoken)
		taskYIELD();
}

// Convert a character to data (Hexadecimal)
uint8_t c2hex(uint8_t c)
{
	uint8_t num = 0xff;
	switch (c) {
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
	return num;
}

void app_rx_task(void *param)
{
	puts_P(PSTR("\e[96mAPP RX task initialised."));
	static uint8_t buf[120];
	static uint8_t addr, port;
	static uint8_t len;
	uint8_t *ptr;
loop:
	len = sizeof(buf);
	soc_recvfrom(sid_text, buf, &len, &addr, &port);
	//buf = pkt;
	uart0_lock();
	ptr = buf;
	printf_P(PSTR(ESC_GREEN "Received from %02x:%02x: "), addr, port);
	while (len--) {
		uint8_t c =  *ptr++;
		if (isprint(c))
			putchar(c);
		else
			printf("\\x%02x", c);
	}
	putchar('\n');
	uart0_unlock();

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
	static uint8_t dest = 1;
	static char buffer[7];
	uint8_t report = 0;
	uint16_t count = 0;
	uint32_t notify;
	// Send socket
	//uint8_t sendfd = soc_socket();
	// UART related
	static char uartBuffer[64], *uartPtr;
	static uint8_t uartOffset = 0, uartSending = 0;
	enum UARTStatus {UARTSettings = 0, UARTMessage};
	uint8_t uartStatus = UARTSettings;

	//soc_bind(sid, 233);
	string[8] = net_address();
	// Enable UART0 RXC interrupt
	uart0_interrupt_rxc(1);
	printf_P(PSTR("\e[96mAPP task initialised (%02x/%02x), hello world!\n"), net_address(), mac_address());

poll:
	// No event notify received
	if (xTaskNotifyWait(0, ULONG_MAX, &notify, 100) != pdTRUE) {
		// Start UART message transmission
		if (uartSending) {
			//sendfd = soc_socket();

			uint8_t status = soc_sendto(sid_text, uartBuffer, uartOffset, dest, 233);

			uart0_lock();
			printf_P(PSTR("\e[91mStation %02x/%02x, "), net_address(), mac_address());
			printf_P(PSTR("UART message (DEST: %02x, SIZE: %u): "), dest, uartOffset);
			puts_P(status ? PSTR("SUCCESS") : PSTR("\e[31mFAILED"));
			uart0_unlock();

			if (status)
				uartSending = 0;
		}

		if (uartStatus != UARTSettings)
			goto poll;

		// Start test string transmission
		if (!(PINC & _BV(2))) {
			//puts_P(PSTR("hello"));
			sprintf(buffer, "%6u", count);
			memcpy(string + 8 + 6, buffer, 6);
			uint8_t len;
			len = sizeof(string) > 10 ? 10 : sizeof(string);
			
				
			//printf_P(PSTR("the socket id is %d\n"), sendfd);
			//puts_P(PSTR("socket no problem"));
			uint8_t status = soc_sendto(sid_text, string, len, dest, 233);
			//puts_P(PSTR("no problem"));
			uart0_lock();
			printf_P(PSTR("\e[91mStation %02x/%02x, "), net_address(), mac_address());
			printf_P(PSTR("sent %u (DEST: %02x, SIZE: %u): "), count, dest, len);
			puts_P(status ? PSTR("SUCCESS") : PSTR("\e[31mFAILED"));
			uart0_unlock();

			if (status)
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
		switch (uartStatus) {
		case UARTSettings:
			num = c2hex(c);
			// Update destination address
			if (num != 0xff) {
				dest = (dest << 4) | num;
				uart0_lock();
				printf_P(PSTR("\e[96mDEST update: %02x\n"), dest);
				uart0_unlock();
				break;
			}

			switch (c) {
			case -1:
				break;
			case 'r':
			case 'R':
				app_report();
				break;
			case 'm':
			case 'M':
				if (uartSending) {
					uart0_lock();
					fputs_P(PSTR("\e[96mUART not ready.\n"), stdout);
					uart0_unlock();
				} else {
					uartStatus = UARTMessage;
					uartOffset = 0;
					uartPtr = uartBuffer;
					uart0_lock();
					fputs_P(PSTR("\e[96mUART message input mode:\n"), stdout);
					uart0_unlock();
				}
				break;
			}
			break;

		case UARTMessage:
		default:
			if (c == '\r')
				c = '\n';
			uart0_lock();
			putchar(c);
			uart0_unlock();
			if (c == 127 && uartOffset != 0) {	// Backspace
				uartOffset--;
				uartPtr--;
				break;
			}

			*uartPtr++ = c;
			uartOffset++;
			if (c == '\n' || uartOffset == sizeof(uartBuffer)) {
				*uartPtr = '\0';
				uart0_lock();
				fputs_P(PSTR("\e[96mUART message recorded:\n"), stdout);
				fputs(uartBuffer, stdout);
				uart0_unlock();
				uartStatus = UARTSettings;
				uartSending = 1;
			}
			break;
		}
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

	PORTA = 0x00;
	DDRA = 0xff;

	//rgbLED_init();
//	uint8_t i = RGBLED_NUM;
//	while (i--)
//		rgbLED[i] = 0;

	uart0_init();
	stdout = uart0_fd();
	stdin = uart0_fd();
	puts_P(PSTR("\x0c\e[96mStarting up..."));

	socket_init();
	phy_init();
	mac_init();
	llc_init();
	net_init();
	tran_init();
	sei();

	sid_text = soc_socket();
	soc_bind(sid_text, PORT_TEXT);
}

int main()
{
	init();

	while (xTaskCreate(app_rx_task, "APP RX", 180, NULL, tskAPP_PRIORITY, &rxTask) != pdPASS);
	while (xTaskCreate(app_task, "APP task", 240, NULL, tskAPP_PRIORITY, &appTask) != pdPASS);

	vTaskStartScheduler();
	return 1;
}
