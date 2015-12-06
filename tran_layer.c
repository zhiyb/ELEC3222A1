#include <stdio.h>
#include <uart0.h>
#include <avr/io.h>
#include <tran_layer.h>
#include <net_layer.h>

#include <task.h>

#define STRUCT_SIZE 7
#define DATA_SIZE 114
#define LISTEN_PORT 1
#define RETRY_TIME 300
#define RETRY_LIMIT 3

struct package{
	uint8_t control[2];
	uint8_t src_port;
	uint8_t dest_port;
	uint8_t length;
	uint8_t app_data[DATA_SIZE];
	uint16_t checksum;
};

/*

void trans(void)
{
	get_soc();
	pack();
}


void get_soc(struct socket *a)
{
	soc = *a;
}
*/
void pack(struct package pck, uint8_t len, char *str)
{
	uint8_t length = len + STRUCT_SIZE;
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
			pck.length = len - (i * DATA_SIZE);
			pck.checksum += pck.src_port;
			pck.checksum += pck.dest_port;
			pck.checksum += pck.length;
			pck.checksum += control[1];
			pck.checksum += control[2];
			net_tx(soc.address, length, &pck);
		}
		else
		{
			for(uint8_t j = i * DATA_SIZE; j < (i + 1) * DATA_SIZE; j++)
			{
				uint8_t tem = 0;
				pck.appdata[tem] = str[j];
				pck.checksum += str[j];
			}
			pck.length = DATA_SIZE;
			pck.checksum += pck.srcport;
			pck.checksum += pck.dest_port;
			pck.checksum += pck.length;
			pck.checksum += control[1];
			pck.checksum += control[2];
			net_tx(soc.address, length, &pck);
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
void tran_tx(struct socket soc, uint8_t len, const void *data)
{
//	uint8_t length = STRUCT_SIZE + len;
	struct package buf;
	while(1)
	{
		buf = pvPortMalloc(length);
		if(buffer == NULL)
		{
#if NET_DEBUG >= 0
			fputs("TRAN-BUFFER-FAILED;", stdout);
#endif
			vTaskDelay(RETRY_TIME);
			continue;			
		} else
			break;
	}
	pack(buf, len, data);
		
}
void tran_rx_task(void *param)
{
	uint8_t src_addr;
	uint8_t *buffer;
	uint8_t *data;
	struct net_packet_t *tf;
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

void tran_init()
{
	while ((tran_rx = xQueueCreate(2, sizeof(struct net_packet_t))) == NULL);
#if TRAN_DEBUG > 0
	while (xTaskCreate(tran_rx_task, "TRAN RX", 160, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#else
	while (xTaskCreate(tran_rx_task, "TRAN RX", 120, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
}

