// Author: Yubo Zhi (normanzyb@gmail.com)

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include <ctype.h>
#include <uart0.h>
#include <rfm12_config.h>
#include <rfm12.h>
#include "phy_layer.h"
#include "dll_layer.h"

void dll_data_handler(const uint8_t data)
{
	static uint8_t first = 1;
	if (PINC & _BV(5)) {
		if (data == 0xce) {	// End of data
			phy_receive();
			first = 1;
		}
		return;
	}
	if (first) {
		fputs_P(PSTR("\e[92m"), stdout);
		first = 0;
	}
	if (isprint(data))
		putchar(data);
	else
		putchar('#');
	if (data == 0xce) {	// End of data
		phy_receive();
		putchar('\n');
		first = 1;
	}
}

uint8_t dll_data_request(uint8_t *buffer)
{
	static char string[] = "\xecHello, \xaaworld!\xce";
	static uint8_t offset = 0;
	if (offset == sizeof(string)) {
		offset = 0;
		return 0;
	} else {
		*buffer = *(string + offset++);
		return 1;
	}
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

	rfm12_init();
	sei();
}

int main()
{
	init();
	puts_P(PSTR("\x0c\e[96mInitialised, hello world!"));

	uint16_t count = 0;
poll:
	if (!(PINC & _BV(2))) {
		printf_P(PSTR("\e[91m%d: %d\n"), count++, ctrl.mode);
		while (!phy_free())
			_delay_ms(1);
		phy_transmit();
		_delay_ms(50);
	}
	goto poll;
	return 1;
}
