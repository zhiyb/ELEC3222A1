#include <stdio.h>
#include <uart0.h>

bool recieved(void);
void send(char* str);
char *bufptr;
uint8_t len;
//uint8_t address;
struct socket
{
	uint8_t address;
	uint8_t dest_port;
	uint8_t frame;
}soc

void init()
{
	uart0_init();
	stdout = uart0_fd();
	stdin = uart0_fd();
}

int main(void)
{
	init();
	char *str;	//the string want to send
	uint16_t p = 0;	//the position of string
	uint16_t addr = 0;
	uint16_t port = 0;
	char c;
	uint8_t i = 0;
	len = 0;
	puts("The system is init.");
	while(1)
	{
		if (recieved());
		{
			// dump buffer contents to uart			
			for (uint8_t i=0;i<len;i++)
			{
				putchar(bufptr[i]);
			}
			printf("\n");
			printf("The data is from %d \n", soc.address);
		}
		// If something is received through UART
		if ((c = uart_read_unblocked()) != -1) {
			putchar(c);
			*str++ = c;
			i++;
			if (c == '\n' || c == '\r') {
				*str++ = '\n';
				*str = 0;
				sync_len(i);
				tran();
				//complete sync the len of the data, and then transfer string into transport layer
				i = 0;
			}
			else if(c == ':')
			{
				puts("Please enter the dest address");
				puts("For example:0x01:sb doubi");
		rescan_addr:	if(scanf("%i", &addr) != 0)
				{
					printf("The dest address is set to %i", addr);
					soc.address = addr;
				}
				else
				{	
					puts("you type the wrong address");
					goto rescan_addr;
				}
			}
			else if (c == '\'')
			{
				puts("Please enter the dest Port");
				puts("For example:0x01:sb doubi");
		rescan_port:	if(scanf("%i", &port) != 0)
				{
					printf("The dest port is set to %i", port);
					//send_port(port);
					soc.destport = port; 
				}
				else
				{
					puts("you type the wrong port");
					goto rescan_port;
				}
			}
		}
	}
	return 1;
}

bool recieve(bool re)
{
	if(re)
		return true;
	else
		return false;
}
