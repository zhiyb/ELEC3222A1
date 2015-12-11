#include <stdio.h>

#include <string.h>
#include "tran_layer.h"
#include "net_layer.h"
#include "socket.h"

#ifndef SIMULATION
#include <colours.h>
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

struct package {
	uint8_t control[2];
	uint8_t src_port;
	uint8_t dest_port;
	uint8_t length;
	uint8_t app_data[0];
};

struct app_packet {
	uint8_t len;
	uint8_t addr;
	uint8_t port;
	uint8_t *data;
};

static inline void tran_tx(uint8_t sid, uint8_t len, const void *data, uint8_t addr, uint8_t port)
{
	const uint8_t *str = data;
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
		struct socket_t *sp = sockets + sid;
		pck -> src_port = sp->port;
		pck -> dest_port = port;
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

void tran_rx_task(void *param)
{
	static struct net_packet_t tf;
	struct package *pck;
	uint8_t len;
	uint8_t i;
#if TRAN_DEBUG > 1
	puts_P(PSTR("\e[96mTRAN RX task initialised.]"));
#endif
loop: 
	while(xQueueReceive(net_rx, &tf, portMAX_DELAY) != pdTRUE);
#ifdef SIMULATION
	printf("Received: %02x, %u, %p\n", tf.addr, tf.len, tf.payload);
#endif
	pck = tf.payload;
	len = tf.len;
	if (len != pck -> length + STRUCT_SIZE)
	{
#if TRAN_DEBUG > 1
		fputs_P(PSTR("\e[90mTRAN-LEN-FAILED;"), stdout);
#endif
		goto drop;
	}

	struct socket_t *sp = sockets;
	i = MAX_SOCKETS;
	while (i--) {
		if((sp->status == (SOCKET_ACTIVE | SOCKET_DATAGRAM)))
		{
			if(pck -> dest_port == sp->port)
			{
#if TRAN_DEBUG > 2
				fputs_P(PSTR(ESC_MAGENTA "TRAN-SOCKET-FOUND;"), stdout);
#endif

				struct app_packet apck;

				if ((apck.data = pvPortMalloc(sizeof(pck->length))) == 0)
					goto drop;

				apck.addr = tf.addr;
				apck.len = pck -> length;
				apck.port = pck->src_port;
				memcpy(apck.data, pck -> app_data, pck -> length);
				while(xQueueSendToBack(sp->queue, &apck, portMAX_DELAY) != pdTRUE);
#if TRAN_DEBUG > 2
				fputs_P(PSTR(ESC_YELLOW "TRAN-DATA;"), stdout);
#endif
				goto drop;
			}
		}
		sp += sizeof(struct socket_t);
	}
	
	
drop:
	if(tf.ptr)
	{
		vPortFree(tf.ptr);
		tf.ptr = 0;
	}
	goto loop;
}

void tran_init()
{
#if TRAN_DEBUG > 0
	while (xTaskCreate(tran_rx_task, "TRAN RX", 160, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#else
	while (xTaskCreate(tran_rx_task, "TRAN RX", 120, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#endif
}

/*
 * Socket functions
 */

// Alloc a socket
uint8_t soc_socket()
{
	struct socket_t *sp = sockets;
	uint8_t i;
	for (i = 0; i != MAX_SOCKETS; i++) {
		if(sp->status == SOCKET_FREE) {
			sp->status = SOCKET_ALLOCATED;
			while ((sp->queue = xQueueCreate(2, sizeof(struct app_packet))) == NULL);
			return i;
		}
		sp += sizeof(struct socket_t);
	}
	return 0xff;

}

uint8_t soc_recfrom(uint8_t sid, void *buf, uint8_t *len, uint8_t *addr, uint8_t *port)
{
	if (sid == 0xff)
		return 0;
	struct app_packet app_tem;
	while(xQueueReceive(sockets[sid].queue, &app_tem, 0) != pdTRUE);
	uint8_t length = *len < app_tem.len ? *len : app_tem.len;
	*len = app_tem.len;
	*addr = app_tem.addr;
	*port = app_tem.port;
	if (app_tem.data) {
		memcpy(buf, app_tem.data, length);
		vPortFree(app_tem.data);
	}
	return length;
}

uint8_t soc_sendto(uint8_t sid, void *buf, uint8_t len, uint8_t addr, uint8_t port)
{
	if (sid == 0xff)
		return 0;
	tran_tx(sid, len, buf, addr, port);
	return len;
}	
