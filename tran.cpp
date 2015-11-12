#include <stdio.h>
#include <uart0.h>
#include <avr/io.h>

#define DATA_SIZE 114
#define PORT 1
char str[255];
uint8_t *package
struct package{
	uint8_t control[2];
	uint8_t src_port;
	uint8_t dest_port;
	uint8_t length;
	uint8_t app_data[DATA_SIZE];
	uint16_t checksum;
} pck

struct socket{
	uint16_t address;
	uint16_t port;
	uint16_t frame;
} soc
int main()
{
	get_soc();
	pck.checksum = 0;
redo:	if(sizeof(str) > DATA_SIZE)
	{
		if(sizeof(str) > 2 * DATA_SIZE)
		{
			for(uint8_t i = 0; i < 3; i++)
			{
				pck.control[1] = i;
				pck.control[2] = (i == 2);	
				if(i = 2)
				{
					for(uint8_t j = (2 * DATA_SIZE - 1); j < sizeof(str); j++)
					{
						pck.app_data[j + 1 - 2 * DATA_SIZE] = str[j];
						pck.checksum += str[j];
					}
					//pck.src_port = PORT;
					//pck.dest_port = soc.port;
					pck.length = sizeof(pck.app_data);
					pck.checksum += pck.src_port;
					pck.checksum += pck.dest_port;
					pck.checksum += pck.length;
					pck.checksum += pck.control[1];
					pck.checksum += pck.control[2];
					//send_net(&pck);					
				}
				else
				{
					for(uint8_t j = 0; j < DATA_SIZE; j++)
					{
						pck.app_data[j] = str[j];
						pck.checksum += str[j];
					}
					pck.src_port = PORT;
					pck.dest_port = soc.port;
					pck.length = DATA_SIZE;
					pck.checksum += pck.src_port;
					pck.checksum += pck.dest_port;
					pck.checksum += pck.length;
					pck.checksum += pck.control[1];
					pck.checksum += pck.control[2];
					//send_net(&pck);					
				}
				pck.checksum = 0;
			}	
		}
		else
		{
			for(uint8_t i = 0; i < 2; i++)
			{
				pck.control[1] = i;
				pck.control[2] = (i == 1);	
				if(i = 1)
				{
					for(uint8_t j = (DATA_SIZE - 1); j < sizeof(str); j++)
					{
						pck.app_data[j + 1 - DATA_SIZE] = str[j];
						pck.checksum += str[j];
					}
					//pck.src_port = PORT;
					//pck.dest_port = soc.port;
					pck.length = sizeof(pck.app_data);
					pck.checksum += pck.src_port;
					pck.checksum += pck.dest_port;
					pck.checksum += pck.length;
					pck.checksum += pck.control[1];
					pck.checksum += pck.control[2];
					//send_net(&pck);					
				}
				else
				{
					for(uint8_t j = 0; j < DATA_SIZE; j++)
					{
						pck.app_data[j] = str[j];
						pck.checksum += str[j];
					}
					pck.src_port = PORT;
					pck.dest_port = soc.port;
					pck.length = DATA_SIZE;
					pck.checksum += pck.src_port;
					pck.checksum += pck.dest_port;
					pck.checksum += pck.length;
					pck.checksum += pck.control[1];
					pck.checksum += pck.control[2];
					//send_net(&pck);					
				}
				pck.checksum = 0;
			}				
		}
	}
	else
	{
		pck.control[1] = i;
		pck.control[2] = (i == 2);	
		if(i = 2)
		{
			for(uint8_t j = (2 * DATA_SIZE - 1); j < sizeof(str); j++)
			{
				pck.app_data[j + 1 - 2 * DATA_SIZE] = str[j];
				pck.checksum += str[j];
			}
			//pck.src_port = PORT;
			//pck.dest_port = soc.port;
			pck.length = sizeof(pck.app_data);
			pck.checksum += pck.src_port;
			pck.checksum += pck.dest_port;
			pck.checksum += pck.length;
			pck.checksum += pck.control[1];
			pck.checksum += pck.control[2];
			//send_net(&pck);					
		}

	}
	goto redo;
	return 1;	
}
