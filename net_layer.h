#ifndef NET_LAYER_H
#define NET_LAYER_H

#include <stdint.h>

// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

// Net layer: routing & address handle

#define NET_PACKET_MAX_SIZE	128

// Initialisation
void net_init();

// Interface with upper TRAN layer

// Unacknowledged packet, acknowledged packet
//enum NET_Primitives {NET_UNITDATA = 0, NET_DATA_ACK};

// Data transfer between LLC and upper layer
struct net_packet_t {
	//uint8_t pri;	// NET_Primitives
	uint8_t addr;	// RX: The address of the sending host
	uint8_t len;	// Data length
	void *ptr;	// Pointer to frame data on heap (free after use)
	void *payload;	// Pointer to payload field
};

// RTOS queue for packet transmission & reception
// Queue item size: net_packet_t
extern QueueHandle_t net_rx;

// Transmit a packet
// The data pointed by ptr will not be freed
// Return:	DL_UNITDATA: always 1
// 		DL_DATA_ACK: acknowledged or not
uint8_t net_tx(uint8_t address, uint8_t length, const void *data);

uint8_t net_address(void);
uint8_t net_address_update(uint8_t addr);
void net_report();

#ifdef __cplusplus
}
#endif

#endif
