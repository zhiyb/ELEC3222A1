#include <stdio.h>
#include <uart0.h>
#include <avr/io.h>
#include <tran_layer.h>
#include <net_layer.h>

#define DATA_SIZE 114
#define PORT 1
char *str;
uint8_t len;
struct package{
	uint8_t control[2];
	uint8_t src_port;
	uint8_t dest_port;
	uint8_t length;
	uint8_t app_data[DATA_SIZE];
	uint16_t checksum;
} pck;

struct socket{
	uint16_t address;
	uint16_t port;
	uint16_t frame;
} soc;

int trans(void)
{
	//get_soc();
	//get_data(*str, len);
	//pck.checksum = 0;
	//pack();
	while(1)
	{
		get_soc();
		pack();
	}
	return 1;	
}


void get_soc(struct socket *a)
{
	soc = *a;
}

void pack(void)
{
	pck.src_port = PORT;
	pck.dest_port = soc.dest_port;
	for(uint8_t i = 0; i < len / DATA_SIZE + 1; i++)
	{
		pck.checksum = 0;
		pck.control[1] = i;
		pck.control[2] = (i == len / DATA_SIZE);
		if(pck.control[2] == 1)
		{
			for(uint8_t j = i * DATA_SIZE; j < len; j++)
			{
				uint8_t tem = 0;
				pck.appdata[tem] = str[j];
				pck.checksum += str[j];
			}
			//pck.src_port = PORT;
			//pck.dest_port = soc.port;
			pck.length = len - (i * DATA_SIZE);
			pck.checksum += pck.src_port;
			pck.checksum += pck.dest_port;
			pck.checksum += pck.length;
			pck.checksum += control[1];
			pck.checksum += control[2];
			//send
			//pck.checksum = 0;
		}
		else
		{
			for(uint8_t j = i * DATA_SIZE; j < (i + 1) * DATA_SIZE; j++)
			{
				uint8_t tem = 0;
				pck.appdata[tem] = str[j];
				pck.checksum += str[j];
			}
			//pck.src_port = PORT;
			//pck.dest_port = soc.port;
			pck.length = DATA_SIZE;
			pck.checksum += pck.srcport;
			pck.checksum += pck.dest_port;
			pck.checksum += pck.length;
			pck.checksum += control[1];
			pck.checksum += control[2];
			//send
		}
	}
}

void unpack(void)
{
	uint8_t datalen = net_read(&pck);
}
