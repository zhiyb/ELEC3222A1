#include <stdio.h>
#include <uart0.h>
#include <avr/io.h>
#include <string.h>
#include "tran_layer.h"
#include "net_layer.h"
#include "socket.h"
#include <task.h>

#define STRUCT_SIZE 7
#define DATA_SIZE 114
#define LISTEN_PORT 1
#define RETRY_TIME 300
#define RETRY_LIMIT 3

void pack(struct socket_t soc, uint8_t len, uint8_t *str, uint8_t addr)
{
	uint8_t length;
	uint8_t i;
	uint8_t j;
	struct package pck;
	pck.src_port = soc.sport;
	pck.dest_port = soc.port;
	for(i = 0; i < len / DATA_SIZE + 1; i++)
	{
		pck.checksum = 0;
		pck.control[0] = i;
		pck.control[1] = (i == len / DATA_SIZE);
		if(pck.control[0] == 1)
		{
			
			if ((pck.app_data = pvPortMalloc(DATA_SIZE)) == 0)
				return;
			for(j = i * DATA_SIZE; j < len; j++)
			{
				uint8_t tem = 0;
				pck.app_data[tem] = str[j];
				pck.checksum += str[j];
			}
			pck.length = len - (i * DATA_SIZE);
			pck.checksum += pck.src_port;
			pck.checksum += pck.dest_port;
			pck.checksum += pck.length;
			pck.checksum += pck.control[0];
			pck.checksum += pck.control[1];
			length = pck.length + STRUCT_SIZE;
			net_tx(addr, length, &pck);
	
			vPortFree(pck.app_data);
			
		}
		else
		{
			
			if ((pck.app_data = pvPortMalloc(len - i * DATA_SIZE)) == 0)
				return;
			for(j = 0; j < 114; j++)
			{
				uint8_t tem = 0;
				pck.app_data[tem] = str[j];
				pck.checksum += str[j];
			}
			pck.length = DATA_SIZE;
			pck.checksum += pck.src_port;
			pck.checksum += pck.dest_port;
			pck.checksum += pck.length;
			pck.checksum += pck.control[0];
			pck.checksum += pck.control[1];
			length = pck.length + STRUCT_SIZE;
			net_tx(addr, length, &pck);
			vPortFree(pck.app_data);
		}
	}
}

void tran_tx(struct socket_t soc, uint8_t len, const void *data, uint8_t addr)
{
//	uint8_t length = STRUCT_SIZE + len;
	uint8_t *buf;

	while(1)
	{
		buf = pvPortMalloc(len);
		if(buf == NULL)
		{
#if NET_DEBUG >= 0
			fputs("TRAN-BUFFER-FAILED;", stdout);
#endif
			vTaskDelay(RETRY_TIME);
			continue;			
		} else
			break;
	}
	memcpy(buf, data, len);
	pack(soc, len, buf, addr);
	vPortFree(buf);		
}

void tran_rx_task(void *param)
{
	uint8_t src_addr;
	//uint8_t *buffer;
	//uint8_t *data;
	static struct net_packet_t tf;
	struct package *pck;
	struct app_packet apck;
	uint8_t len;
	uint8_t i;
#if TRAN_DEBUG > 0
	puts_P(PSTR("\e[96mTRAN RX task initialised.]"));
#endif
loop: 
	while(xQueueReceive(net_rx, &tf, portMAX_DELAY) != pdTRUE);
	//tf = buffer
	pck = tf.payload;
	len = tf.len;
	if (len != pck -> length + STRUCT_SIZE)
	{
#if TRAN_DEBUG > 1
		fputs_P(PSTR("\e[90mTRAN-LEN-FAILED;]", stdout);
#endif
		goto drop;
	}
#if 0
	if(pck.dest_port == LISTEN_PORT)
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
	}
#endif
	for(i = 0; i != MAX_SOCKETS; i++)
	{

		if((sockets[i].status == SOCKET_ACTIVE) && (sockets[i].type == SOCKET_DATAGRAM))
		{
			if(pck -> dest_port == sockets[i].port)
			{
				src_addr = tf.addr;
				if(src_addr == sockets[i].address)
				{
#if TRAN_DEBUG > 2					
					fputs_P(PSTR("\e[90mTRAN-SOCKET-FOUND;]", stdout);
#endif

					//if(pck -> control[2] == 1)
					//{
						
						if ((apck.data = pvPortMalloc(sizeof(pck -> app_data))) == 0)
							return;
						memcpy(apck.data, pck -> app_data, pck -> length);
						apck.addr = sockets[i].address;
						apck.len = pck -> length;
							while(xQueueSendToBack(sockets[i].queue, apck.data, portMAX_DELAY) != pdTRUE);
#if TRAN_DEBUG > 3					
						fputs_P(PSTR("\e[90mTRAN-TRANSFER-DATA;]", stdout);
#endif
					//}
					//else
					//{
					//	memcpy(apck.data[len], pck -> data, pck -> length);
					//	apck.len += pck -> length;
					//	goto loop;
					//}
				}
				else
				{
					i++;
				}
			}

			else
			{
#if TRAN_DEBUG > 1
				fputs_P(PSTR("\e[90mTRAN-PORT-FAILED;]", stdout);
#endif
				i++;
			}	
		}
		else
		{
			i++;
		}
	}		
	
	
drop:
	if(tf.ptr)

		vPortFree(tf.ptr);
	goto loop;
}

void tran_init()
{
//	while ((tran_rx = xQueueCreate(2, sizeof(struct net_packet_t))) == NULL);
#if TRAN_DEBUG > 0
	while (xTaskCreate(tran_rx_task, "TRAN RX", 160, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#else
	while (xTaskCreate(tran_rx_task, "TRAN RX", 160, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#endif
	printf("Tran RX init\n");
}

uint8_t rec_from(uint8_t sid, void *buf, uint8_t len, uint8_t addr)
{
	struct app_packet app_tem;
	while(xQueueReceive(sockets[sid].queue, &app_tem, 0) != pdTRUE);
	len = app_tem.len;
	addr = app_tem.addr;
	buf = app_tem.data;
	return len;
}

uint8_t sendto(uint8_t sid, void *buf, uint8_t len, uint8_t addr)
{
	//if(socket[i])
	sockets[sid].sport = 1;
	sockets[sid].port = 233;
	sockets[sid].type = SOCKET_DATAGRAM;	
	tran_tx(sockets[sid], len, buf, addr);
	sockets[sid].status = SOCKET_FREE;
	return sid;	
}	
