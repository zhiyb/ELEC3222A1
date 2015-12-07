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

struct app_packet
{
	uint8_t len;
	uint8_t addr;
	uint8_t *data;
}
void pack(struct package pck, uint8_t len, char *str)
{
	uint8_t length;
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
			length = pck.length + STRUCT_SIZE;
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
			length = pck.length + STRUCT_SIZE;
			net_tx(soc.address, length, &pck);
		}
	}
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
	//uint8_t *buffer;
	//uint8_t *data;
	static struct net_packet_t tf;
	struct package *pck;
	struct app_packet apck;
	uint8_t len;
	uint8_t i;
#if TRAN_DEBUG > 0
	puts_P(PSTR("\e[96mTRAN RX task initialised.]"));
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
				src_addr = tf.ip;
				if(src_addr == sockets[i].address)
				{
#if TRAN_DEBUG > 2					
					fputs_P(PSTR("\e[90mTRAN-SOCKET-FOUND;]", stdout);
#endif

					//if(pck -> control[2] == 1)
					//{
						memcpy(apck.data[len], pck -> data, pck -> length);
						apck.addr = soc.address;
						apck.len = pck -> length;
							while(xQueueSendToBack(sockets[i].queue, apck, portMAX_DELAY) != pdTRUE);
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
	if{tf.ptr}
		vPortFree(tf.ptr);
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

uint8_t rec_from(uint8_t sid, void *buf, uint8_t len, uint8_t addr)
{
	struct app_packet *app_tem;
	while(xQueueReceive(socket[sid].queue, app_tem, 0) != pdTURE);
	len = app_tem -> len;
	addr = app_tem -> addr;
	buf = app_tem -> data;
	buf[len] = '\0';
	return len;
}
