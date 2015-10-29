// Author: Yubo Zhi (normanzyb@gmail.com)

#include <stdio.h>
#include <uart0.h>

void init()
{
	uart0_init();
	stdout = uart0_fd();
	stdin = uart0_fd();
}

int main()
{
	init();
	puts("Initialised, hello world!");

poll:
	goto poll;
	return 1;
}
