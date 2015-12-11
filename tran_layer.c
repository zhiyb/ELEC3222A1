#include <stdio.h>

#include <string.h>
#include "tran_layer.h"
#include "net_layer.h"
#include "socket.h"

#ifndef SIMULATION
#include <task.h>
#include <uart0.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#else
#include "simulation.h"
#endif

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
	struct package *pck;
	uint8_t ctrl[2];
	uint16_t checksum;

	//puts_P(PSTR("pack init no problem"));
	for(i = 0; i < len / DATA_SIZE + 1; i++)
	{
		checksum = 0;
		ctrl[0] = i;
		ctrl[1] = ((len < (i + 1) * DATA_SIZE) | (len == (i + 1) * DATA_SIZE));
		if(ctrl[1] == 1)
		{
			if ((pck = pvPortMalloc(len - (i * DATA_SIZE) + STRUCT_SIZE)) == 0)
				return;
			pck -> length = len % DATA_SIZE;
		}
		else
		{
			if ((pck = pvPortMalloc(DATA_SIZE + STRUCT_SIZE)) == 0)
				return;
			pck -> length = DATA_SIZE;
		}
		pck -> src_port = soc.sport;
		pck -> dest_port = soc.port;
		pck -> control[0] = ctrl[0];
		pck -> control[1] = ctrl[1];
		//puts_P(PSTR("pack before memcpy no problem"));
		memcpy(pck -> app_data, &str[i * DATA_SIZE], pck -> length);
		for(j = 0; j < pck -> length + 5; j++)
			checksum += *(((uint8_t *) pck) + j);
		*((uint16_t *)(((void *)pck) + j)) = checksum;
		length = pck -> length + STRUCT_SIZE;
		
		//printf_P(PSTR("CTRL:%u, NetpackLength:%u, App data:%u, package num: %u, check sum: %u, sizeofPck: %u\n "), ctrl[1], length, len, i, checksum, (len - (i * DATA_SIZE) + STRUCT_SIZE));
		net_tx(addr, length, pck);
		
		//printf_P(PSTR(" package num: %d transfer"), i);
		vPortFree(pck);
		pck = NULL;
	}
}

void tran_tx(struct socket_t soc, uint8_t len, const void *data, uint8_t addr)
{
	//	uint8_t length = STRUCT_SIZE + len;
	uint8_t *buf;

	if((buf = pvPortMalloc(len)) == 0)
		return;
	memcpy(buf, data, len);
	//puts_P(PSTR("cpy buf no problem"));
	pack(soc, len, buf, addr);
	vPortFree(buf);
	//buf = NULL;
}

void tran_rx_task(void *param)
{
	uint8_t src_addr;
	//uint8_t *buffer;
	//uint8_t *data;
	static struct net_packet_t tf;
	struct package *pck;
	struct app_packet *apck = 0;
	uint8_t len;
	uint8_t i;
#if TRAN_DEBUG > 0
	puts_P(PSTR("\e[96mTRAN RX task initialised.]"));
#endif
loop: 
	while(xQueueReceive(net_rx, &tf, portMAX_DELAY) != pdTRUE);
#ifdef SIMULATION
	printf("Received: %02x, %u, %p\n", tf.addr, tf.len, tf.payload);
#endif
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

							if ((apck = pvPortMalloc(sizeof(struct app_packet))) == 0)
							goto drop;

					if ((apck->data = pvPortMalloc(sizeof(pck->length))) == 0)
						goto drop;

					apck -> addr = tf.addr;
					apck -> len = pck -> length;
					memcpy(apck -> data, pck -> app_data, pck -> length);
					while(xQueueSendToBack(sockets[i].queue, apck, portMAX_DELAY) != pdTRUE);
#if TRAN_DEBUG > 3					
					fputs_P(PSTR("\e[90mTRAN-TRANSFER-DATA;]", stdout);
		#endif
							goto drop;
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
	{
		vPortFree(tf.ptr);
		tf.ptr = 0;
	}
	if(apck)
	{
		vPortFree(apck);
		apck = 0;
	}
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
	//printf("Tran RX init\n");
}

uint8_t soc_recfrom(uint8_t sid, void *buf, uint8_t *len, uint8_t *addr)
{
	struct app_packet app_tem;
	while(xQueueReceive(sockets[sid].queue, &app_tem, 0) != pdTRUE);
	uint8_t length = *len < app_tem.len ? *len : app_tem.len;
	*len = app_tem.len;
	*addr = app_tem.addr;
	memcpy(buf, app_tem.data, length);
	vPortFree(app_tem.data);
	return length;
}

uint8_t soc_sendto(uint8_t sid, void *buf, uint8_t len, uint8_t addr)
{
	//if(socket[i])
	sockets[sid].sport = 1;
	sockets[sid].port = 233;
	sockets[sid].type = SOCKET_DATAGRAM;
	tran_tx(sockets[sid], len, buf, addr);
	//puts_P(PSTR("tran no problem"));
	sockets[sid].status = SOCKET_FREE;
	return sid;
}	
