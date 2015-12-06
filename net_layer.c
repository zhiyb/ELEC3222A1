#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "net_layer.h"
#include "llc_layer.h"
#include "mac_layer.h"
//#include "trans_layer.h"//?? name correct ?

#include <task.h>
//#include <semphr.h>

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

#define NET_PKT_MIN_SIZE	(sizeof(struct net_buffer) + 2)

struct net_ack_t {
	uint8_t mac;	// SRC ACK address
	uint8_t addr;	// SRC address
};

struct net_arp_t {
	uint8_t addr;
	uint8_t mac;
	struct net_arp_t *next;
} *arp;

// Pointer to checksum field
#define CHECKSUM(s)	((uint16_t *)(&(s)->TRAN_Segment + (s)->Length))

static EEMEM uint8_t NVNETAddress = ADDRESS;
static uint8_t net_addr;

void net_rx_task(void *param);

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

void net_arp_init()
{
	arp = 0;
}

uint8_t net_arp_find(uint8_t address)
{
	struct net_arp_t **ptr = &arp;
	while (*ptr != 0) {
		if ((*ptr)->addr == address)
			return (*ptr)->mac;
		ptr = &(*ptr)->next;
	}
	return MAC_BROADCAST;
}

void net_arp_update(uint8_t address, uint8_t mac)
{
	if (mac == MAC_BROADCAST)
		return;
	struct net_arp_t **ptr = &arp;
	while (*ptr != 0) {
		if ((*ptr)->addr == address) {
			(*ptr)->mac = mac;
			return;
		}
		ptr = &(*ptr)->next;
	}
	*ptr = pvPortMalloc(sizeof(struct net_arp_t));
	(*ptr)->addr = address;
	(*ptr)->mac = mac;
	(*ptr)->next = 0;
}

void net_init()
{
	net_arp_init();
	net_addr = eeprom_read_byte(&NVNETAddress);
	while ((net_rx = xQueueCreate(2, sizeof(struct net_packet_t))) == NULL); //create a queue for rx
	while ((net_ack = xQueueCreate(1, sizeof(struct net_ack_t))) == NULL);
	//while (xTaskCreate(net_rx_task, "NET RX", configMINIMAL_STACK_SIZE, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
	while (xTaskCreate(net_rx_task, "NET RX", 120, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
}

uint8_t net_tx(uint8_t address, uint8_t len, const void *data)
{ //requrie whole package and destination address
	// Allocate buffer
	uint8_t length = NET_PKT_MIN_SIZE + len;
	struct net_buffer *buffer;
	for (;;) {
		// Checksum
		buffer = pvPortMalloc(length);
		if (buffer == NULL) {
			fputs("NET-BUFFER-FAILED;", stdout);
			continue;
		} else
			break;
	}
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

	uint8_t *ptr = (uint8_t *)buffer;
	uint16_t checksum = 0;
	{
		uint8_t len = length - 2;
		while (len--)
			checksum += *ptr++;
	}
	*CHECKSUM(buffer) = checksum;

	llc_tx(DL_UNITDATA, MAC_BROADCAST, NET_PKT_MIN_SIZE, buffer);
	while (!llc_written());

	struct net_ack_t ack;
	xQueueReset(net_ack);
	if (xQueueReceive(net_ack, &ack, RETRY_TIME) != pdTRUE && ack.addr != address && ack.mac != MAC_BROADCAST) {
		if (arp_count-- == 0) {
			vPortFree(buffer);
			return 0;
		}
		goto findMAC;
	}

	// ARP found
	mac = ack.mac;
	net_arp_update(address, mac);

transmit:
	// Transmit packet
	buffer->Control = NETData;
	buffer->Length = len;
	memcpy(buffer->TRAN_Segment, data, len);
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
	puts_P(PSTR("\e[96mNET RX task initialised."));

loop:
	while (xQueueReceive(llc_rx, &pkt, portMAX_DELAY) != pdTRUE);

	struct net_buffer *packet = pkt.payload;
	if (pkt.len != packet->Length + NET_PKT_MIN_SIZE) {
#if NET_DEBUG > 1
		fputs_P(PSTR("\e[90mNET-LEN-FAILED;"), stdout);
#endif
		goto drop;
	}
	if (packet->DEST_Address != net_address()) {
#if NET_DEBUG > 1
		fputs_P(PSTR("\e[90mNET-ADDR-DROP;"), stdout);
#endif
		goto drop;
	}

	switch (packet->Control) {
	case ARPReq:
#if NET_DEBUG > 2
		fputs_P(PSTR("\e[90mNET-ARP-REQ;"), stdout);
#endif
		net_arp_update(packet->SRC_Address, pkt.addr);
		// Send ARPAck
		{
			struct net_buffer *buffer;
			buffer = pvPortMalloc(NET_PKT_MIN_SIZE);
			if (buffer == NULL) {
#if NET_DEBUG >= 0
				fputs_P(PSTR("\e[90mNET-ARP-REQ-FAILED;"), stdout);
#endif
				goto drop;
			}
			buffer->Control = ARPAck;
			buffer->SRC_Address = net_address();
			buffer->DEST_Address = packet->SRC_Address;
			buffer->Length = 0;
			llc_tx(DL_DATA_ACK, pkt.addr, NET_PKT_MIN_SIZE, buffer);
			vPortFree(packet);
		}
		goto drop;
	case ARPAck:
#if NET_DEBUG > 2
		fputs_P(PSTR("\e[90mNET-ARP-ACK;"), stdout);
#endif
		{
			struct net_ack_t ack;
			ack.addr = packet->SRC_Address;
			ack.mac = pkt.addr;
			if (xQueueSendToBack(net_ack, &ack, 0) != pdTRUE) {
#if NET_DEBUG >= 0
				fputs_P(PSTR("\e[90mNET-ARP-ACK-FAILED;"), stdout);
#endif
			}
		}
		goto drop;
	case NETData:
#if NET_DEBUG > 2
		fputs_P(PSTR("\e[90mNET-DATA;"), stdout);
#endif
		{
			struct net_packet_t p;
			p.addr = packet->SRC_Address;
			p.len = packet->Length;
			p.payload = packet->TRAN_Segment;
			p.ptr = pkt.ptr;
			if (xQueueSendToBack(net_rx, &p, 0) != pdTRUE) {
#if NET_DEBUG >= 0
				fputs_P(PSTR("\e[90mNET-DATA-FAILED;"), stdout);
#endif
				goto drop;
			}
		}
		goto loop;
	default:
#if NET_DEBUG >= 0
		fputs_P(PSTR("\e[90mNET-CTRL-FAILED;"), stdout);
#endif
		goto drop;
	}

drop:
	if (pkt.ptr)
		vPortFree(pkt.ptr);
	goto loop;
}
