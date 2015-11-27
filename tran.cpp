#include <stdio.h>
#include <uart0.h>
#include <avr/io.h>
#include <tran_layer.h>
#include <net_layer.h>

#include <task.h>

#define STRUCT_SIZE 7
#define DATA_SIZE 114
#define LISTEN_PORT 1
char *str;
uint8_t len;
struct package{
	uint8_t control[2];
	uint8_t src_port;
	uint8_t dest_port;
	uint8_t length;
	uint8_t app_data[DATA_SIZE];
	uint16_t checksum;
};

struct tran_frame{
	uint8_t tran_len;
	uint8_t ip;
	struct package *tem;
} ;


void trans(void)
{
	get_soc();
	pack();

//	tran_write((uint8_t *)&pck, pck.len + STRUCT_SIZE);
//	net_tran(void);	
}


void get_soc(struct socket *a)
{
	soc = *a;
}

void pack(struct package pck)
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

void unpack(uint8_t len_data, uint8_t *data)
{
	len_data = net_read(&pck);
	data = pck.appdata;	
}

void sync_len(uint8_t a)
{
	len = a;
}
void tran_tx_task(void *param)
{
	struct package pck;
	struct tran_frame tf;
	static struct package pkg;
	printf("TX task init");
loop:
	while (xQueueReceive(tran_tx, str, portMAX_DELAY) != pdTRUE);
	if(len == 0)
		goto loop;
//	get_soc();
	pack(pck);
	uint8_t data_len = len + STRUCT_SIZE;
	tf.tran_len = data_len;
	tf.tem = &pck;
	while((data_len + 1)--)
	{
		while(xQueueSendToBack(net_tx, &tf, portMAX_DELAY) != pdTRUE);	
	}
	goto loop;
}
void tran_rx_task(void *param)
{
	uint8_t src_addr;
	uint8_t *buffer;
	uint8_t *data;
	struct tran_frame *tf;
	struct package pck;
	uint8_t i;
loop: 
	while(xQueueReceive(net_rx, buffer, portMAX_DELAY) != pdTRUE);
	tf = buffer;
	&pck = tf->tem;
	/*if(pck.dest_port == LISTEN_PORT)
	{	
		src_addr = tf->ip;
		for(i = 0; i != MAX_SOCKETS; i++)
		{
			if((sockets[i].status == SOCKET_ACTIVE) && (sockets[i].type == SOCKET_LISTEN))
			{
				if(src_addr == sockets[i].address)
				{
					while(xQueueSendToBack(sockets[i].queue, pck, portMAX_DELAY) != pdTRUE);
				}
			}
		}
	}*/
	if(pck.dest_port == LISTEN_PORT_UDP)
	{
		src_addr = tf->ip;
		if((sockets[i].status == SOCKET_ACTIVE) && (sockets[i].type == SOCKET_DATAGRAM))
		{
			for(i = 0; i != MAX_SOCKETS; i++)
			{
				if(src_addr == sockets[i].address)
				{
					while(xQueueSendToBack(sockets[i].queue, pck, portMAX_DELAY) != pdTRUE);
				}
			}
		}
	}

	goto loop;
}


