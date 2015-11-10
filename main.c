// Author: Yubo Zhi (normanzyb@gmail.com)

#include <stdio.h>
#include <uart0.h>

FILE *uart1;

void init()
{
	uart0_init();
	stdout = uart0_fd();
	stdin = uart0_fd();
	uart1 = uart1_fd();
}

int main()
{
	init();
	puts("Initialised, hello world!");
	fputs("Initialised, hello world!\n", uart1);
	printf(uart1, "Initialised, hello world!\n");

poll:
	goto poll;
	return 1;
}
