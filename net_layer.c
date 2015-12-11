#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifndef SIMULATION
#include <colours.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#endif
#include "net_layer.h"
#include "llc_layer.h"
#include "mac_layer.h"

#ifndef SIMULATION
#include <task.h>
//#include <semphr.h>
#else
#include "simulation.h"
#endif

#ifdef SIMULATION
#define NET_DEBUG	3
#endif

enum NetControl {ARPReq = 0, ARPAck, NETData};

#define RETRY_TIME	300
#define RETRY_COUNT	3

QueueHandle_t net_rx, net_ack;

struct net_buffer {
	uint16_t Control;
	uint8_t SRC_Address;
	uint8_t DEST_Address;
	uint8_t Length;
	uint8_t TRAN_Segment[0];
};

#define NET_PKT_MIN_SIZE	(5 + 2)

struct net_ack_t {
	uint8_t mac;	// SRC ACK address
	uint8_t net;	// SRC address
};

struct net_arp_t {
	uint8_t net;
	uint8_t mac;
	struct net_arp_t *next;
} *arp;

// Pointer to checksum field
#define CHECKSUM(s)	((uint16_t *)((s)->TRAN_Segment + (s)->Length))

#ifndef SIMULATION
static EEMEM uint8_t NVNETAddress = ADDRESS;
static uint8_t net_addr;
#endif

void net_rx_task(void *param);

void net_report()
{
	uint8_t rec = 0;
	struct net_arp_t **ptr = &arp;
	while (*ptr != NULL) {
		rec++;
		ptr = &(*ptr)->next;
	}

	printf_P(PSTR("NET: ARP entries: %u\n"), rec);
}

#ifndef SIMULATION
uint8_t net_address(void)
{
	return net_addr;
}

uint8_t net_address_update(uint8_t addr)
{
	net_addr = addr;
	eeprom_update_byte(&NVNETAddress, addr);
	return net_addr;
}
#endif

void net_arp_init()
{
	arp = 0;
}

uint8_t net_arp_find(uint8_t address)
{
	struct net_arp_t **ptr = &arp;
	while (*ptr != 0) {
		if ((*ptr)->net == address)
			return (*ptr)->mac;
		ptr = &(*ptr)->next;
	}
	return MAC_BROADCAST;
}

void net_arp_update(uint8_t net, uint8_t mac)
{
	if (mac == MAC_BROADCAST)
		return;
	struct net_arp_t **ptr = &arp;
	while (*ptr != 0) {
		if ((*ptr)->net == net) {
			(*ptr)->mac = mac;
			return;
		}
		ptr = &(*ptr)->next;
	}
	if ((*ptr = pvPortMalloc(sizeof(struct net_arp_t))) == 0)
		return; //exit function when fail to allocate new memory
	(*ptr)->net = net;
	(*ptr)->mac = mac;
	(*ptr)->next = 0;
}

void net_init()
{
	net_arp_init();
#ifndef SIMULATION
	net_addr = eeprom_read_byte(&NVNETAddress);
#endif
	while ((net_rx = xQueueCreate(2, sizeof(struct net_packet_t))) == 0); //create a queue for rx
	while ((net_ack = xQueueCreate(1, sizeof(struct net_ack_t))) == 0);
#if NET_DEBUG > 0
	while (xTaskCreate(net_rx_task, "NET RX", 160, NULL, tskPROT_PRIORITY, 0) != pdPASS);
#else //NET_BEBUG is 0 when not defined
	while (xTaskCreate(net_rx_task, "NET RX", 120, NULL, tskPROT_PRIORITY, 0) != pdPASS);
	//while (xTaskCreate(net_rx_task, "NET RX", configMINIMAL_STACK_SIZE, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#endif
}

uint8_t net_tx(uint8_t address, uint8_t len, const void *data)
{ //requrie whole package and destination address
	// Allocate buffer
	uint8_t length = NET_PKT_MIN_SIZE + len;
	struct net_buffer *buffer;
	for (;;) {
		if ((buffer = pvPortMalloc(length)) != NULL)
			break;
#if NET_DEBUG >= 0
		fputs(ESC_GREY "NET-BUFFER-FAILED;", stdout);
#endif
		vTaskDelay(RETRY_TIME);
	}
#if NET_DEBUG > 1
	printf_P(PSTR(ESC_BLUE "NET-TX,%02x;"), address);
#endif
	buffer->SRC_Address = net_address();
	buffer->DEST_Address = address;

	// Find destination MAC address
	uint8_t mac = net_arp_find(address);

	uint8_t tx_count = RETRY_COUNT;
	// If found destination MAC address
	if (mac != MAC_BROADCAST)
		goto transmit;

	// Find destination MAC address by ARP
	uint8_t arp_count;
retryARP:
	arp_count = RETRY_COUNT;
findMAC:
	buffer->Control = ARPReq;
	buffer->Length = 0;

	{	// Compute checksum
		uint8_t *ptr = (uint8_t *)buffer;
		uint16_t checksum = 0;
		uint8_t check_len = NET_PKT_MIN_SIZE - 2;
		while (check_len--)
			checksum += *ptr++;
		// Store checksum to its position in buffer
		memcpy(CHECKSUM(buffer), &checksum, 2);
	}

	llc_tx(DL_UNITDATA, MAC_BROADCAST, NET_PKT_MIN_SIZE, buffer);
	while (!llc_written());

	struct net_ack_t ack;
	
	// Reset a queue to its original empty state
	xQueueReset(net_ack);
	if (xQueueReceive(net_ack, &ack, RETRY_TIME) != pdTRUE || ack.net != address || ack.mac == MAC_BROADCAST) {
		if (arp_count-- == 0) {
			vPortFree(buffer);
			return 0;
		}
		goto findMAC;
	}

	// ARP found
	mac = ack.mac;
	net_arp_update(address, mac);

	// An empty frame to reset target status
	if (!llc_tx(DL_DATA_ACK, mac, 0, 0))
		goto retryARP;

transmit:
#if NET_DEBUG > 1
	printf_P(PSTR(ESC_BLUE "NET-TX-MAC,%u-%u;"), address, mac);
#endif
	// Transmit packet
	buffer->Control = NETData;
	buffer->Length = len;
	memcpy(buffer->TRAN_Segment, data, len);
	
	{	// Compute checksum
		uint8_t *ptr = (uint8_t *)buffer;
		uint16_t checksum = 0;
		uint8_t check_len = length - 2;
		while (check_len--)
			checksum += *ptr++;
		// Store checksum to its position in buffer
		memcpy(CHECKSUM(buffer), &checksum, 2);
	}

	if (!llc_tx(DL_DATA_ACK, mac, length, buffer)) {
		if (tx_count-- == 0) {
			vPortFree(buffer);
			return 0;
		}
		goto retryARP;
	}
	vPortFree(buffer);
	return 1;
}

void net_rx_task(void *param)
{
	static struct llc_packet_t pkt;
#if NET_DEBUG > 0
	puts_P(PSTR("\e[96mNET RX task initialised."));
#endif

loop:
	while (xQueueReceive(llc_rx, &pkt, portMAX_DELAY) != pdTRUE);

	//packet point to the payload
	struct net_buffer *packet = pkt.payload;

	if (pkt.len != packet->Length + NET_PKT_MIN_SIZE) {
#if NET_DEBUG > 1
		fputs_P(PSTR(ESC_MAGENTA "NET-LEN-FAILED;"), stdout);
#endif
		goto drop;
	}

#ifndef SIMULATION
	// Address conflict
	if (packet->SRC_Address == net_address())
		net_address_update(net_address() + 1);
#endif

	if (packet->DEST_Address != net_address()) {
#if NET_DEBUG > 1
		fputs_P(PSTR(ESC_MAGENTA "NET-ADDR-DROP;"), stdout);
#endif
		goto drop;
	}

	{	// Check checksum
		uint16_t checksum = 0;
		uint8_t len = pkt.len - 2;
		uint8_t *ptr = (uint8_t *)packet;
		while (len--)
			checksum += *ptr++;
		uint16_t cksum;
		memcpy(&cksum, CHECKSUM(packet), 2);
		if (cksum != checksum) {
#if NET_DEBUG > 0
			printf_P(PSTR(ESC_GREY "NET-CKSUM-FAILED,%04x,%04x;"), checksum, cksum);
#endif 
			goto drop;
		}
	}

	
	switch (packet->Control) {
	case ARPReq:

#if NET_DEBUG > 2
		fputs_P(PSTR(ESC_YELLOW "NET-ARP-REQ;"), stdout);
#endif
		net_arp_update(packet->SRC_Address, pkt.addr);
		// Send ARPAck
		{
			struct net_buffer *buffer;
			buffer = pvPortMalloc(NET_PKT_MIN_SIZE);
			if (buffer == NULL) {
#if NET_DEBUG >= 0
				fputs_P(PSTR(ESC_GREY "NET-ARP-REQ-FAILED;"), stdout);
#endif
				goto drop;
			}
			buffer->Control = ARPAck;
			buffer->SRC_Address = net_address();
			buffer->DEST_Address = packet->SRC_Address;
			buffer->Length = 0;

			{	// Compute checksum
				uint8_t *ptr = (uint8_t *)buffer;
				uint16_t checksum = 0;
				uint8_t check_len = NET_PKT_MIN_SIZE - 2;
				while (check_len--)
					checksum += *ptr++;
				// Store checksum to its position in buffer
				memcpy(CHECKSUM(buffer), &checksum, 2);
			}

			llc_tx(DL_DATA_ACK, pkt.addr, NET_PKT_MIN_SIZE, buffer);
			vPortFree(buffer);
		}
		goto drop;
		
	case ARPAck:
#if NET_DEBUG > 2
		fputs_P(PSTR(ESC_YELLOW "NET-ARP-ACK;"), stdout);
#endif
		{
			struct net_ack_t ack;
			ack.net = packet->SRC_Address;
			ack.mac = pkt.addr;
			if (xQueueSendToBack(net_ack, &ack, 0) != pdTRUE) {
#if NET_DEBUG > 0
				fputs_P(PSTR(ESC_GREY "NET-ARP-ACK-FAILED;"), stdout);
#endif
			}
		}
		goto drop;
	
	case NETData:
#if NET_DEBUG > 2
		fputs_P(PSTR(ESC_YELLOW "NET-DATA;"), stdout);
#endif
		{
			struct net_packet_t p;
			p.addr = packet->SRC_Address;
			p.len = packet->Length;
			p.payload = packet->TRAN_Segment;
			p.ptr = pkt.ptr;
			if (xQueueSendToBack(net_rx, &p, 0) != pdTRUE) {
#if NET_DEBUG > 0
				fputs_P(PSTR(ESC_GREY "NET-DATA-FAILED;"), stdout);
#endif
				goto drop;
			}
		}
		goto loop;
	default:
#if NET_DEBUG > 0
		fputs_P(PSTR(ESC_GREY "NET-CTRL-FAILED;"), stdout);
#endif
		goto drop;
	}

drop:
	if (pkt.ptr)
		vPortFree(pkt.ptr);
	goto loop;
}
