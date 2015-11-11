// Author: Yubo Zhi (normanzyb@gmail.com)

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include <ctype.h>
#include <uart0.h>
#include "phy_layer.h"
#include "dll_layer.h"

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

void testRX(void *param)
{
	uint8_t data;
loop:
	while (xQueueReceive(phy_rx, &data, portMAX_DELAY) != pdTRUE);
	if (isprint(data))
		putchar(data);
	else
		printf("\\x%02x", data);
	if (data == 0xce) {	// End of data
		phy_receive();
		putchar('\n');
	}
	goto loop;
}

void testTX(void *param)
{
	static char string[] = "\xecHello, \xaaworld! PHY test string, very long? 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\xce";
	char *ptr;
loop:
	vTaskDelay(configTICK_RATE_HZ / 5);
	puts_P(PSTR("\e[91mSending...\e[92m"));
	for (ptr = string; *ptr != '\0'; ptr++)
		xQueueSendToBack(phy_tx, ptr, portMAX_DELAY);
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

	phy_init();
	sei();
}

int main()
{
	init();
	puts_P(PSTR("\x0c\e[96mInitialised, hello world!"));

	xTaskCreate(testRX, "Test RX", 160, NULL, tskAPP_PRIORITY, NULL);
	xTaskCreate(testTX, "Test TX", configMINIMAL_STACK_SIZE, NULL, tskAPP_PRIORITY, NULL);
	puts_P(PSTR("\e[96mTasks created, start scheduler."));
	vTaskStartScheduler();
	return 1;
}
