#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart0.h"

static SemaphoreHandle_t uart0_semaphore;

void uart0_init()
{
	// GPIO initialisation
	DDRD &= ~(_BV(0) | _BV(1));
	PORTD |= _BV(0) | _BV(1);

	// UART 0 initialisation
	#define BAUD	UART0_BAUD
	#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0A = USE_2X << U2X0;
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);

	uart0_semaphore = xSemaphoreCreateMutex();
	while (uart0_semaphore == NULL);
}

int uart0_read_unblocked()
{
	if (!uart0_available())
		return -1;
	return UDR0;
}

// Read 1 character, blocking
char uart0_read()
{
	while (!uart0_available());
	return UDR0;
}

int uart0_write_unblocked(const char data)
{
	portENTER_CRITICAL();
	if (!uart0_ready()) {
		portEXIT_CRITICAL();
		return -1;
	}
	UDR0 = data;
	portEXIT_CRITICAL();
	return data;
}

void uart0_write(const char data)
{
loop:
	while (!uart0_ready());
	portENTER_CRITICAL();
	if (!uart0_ready()) {
		portEXIT_CRITICAL();
		goto loop;
	}

	UDR0 = data;
	portEXIT_CRITICAL();
}

// For fdevopen
static int putch(char ch, FILE *stream)
{
	if (ch == '\n')
		putch('\r', stream);
	uart0_write(ch);
	return ch;
}

// For fdevopen
static int getch(FILE *stream)
{
	return uart0_read();
}

FILE *uart0_fd()
{
	static FILE *dev = NULL;
	if (dev == NULL)
		dev = fdevopen(putch, getch);
	return dev;
}

void uart0_lock()
{
	while (xSemaphoreTake(uart0_semaphore, portMAX_DELAY) != pdTRUE);
}

void uart0_unlock()
{
	xSemaphoreGive(uart0_semaphore);
}
