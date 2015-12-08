#ifndef LLC_LAYER_H
#define LLC_LAYER_H

#include <stdint.h>

// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

// Logical link control layer: transmission in unit of packets

#define LLC_FRAME_MAX_SIZE	26

// Initialisation
void llc_init();

// Interface with upper NET layer

// Unacknowledged packet, acknowledged packet
enum LLC_Primitives {DL_UNITDATA = 0, DL_DATA_ACK};

// Data transfer between LLC and upper layer
struct llc_packet_t {
	uint8_t pri;	// LLC_Primitives
	uint8_t addr;	// RX: The MAC address of the sending host
	uint8_t len;	// Data length
	void *ptr;	// Pointer to frame data on heap (free after use)
	void *payload;	// Pointer to payload field
};

// Transmit a packet
// Return:	DL_UNITDATA: always 1
// 		DL_DATA_ACK: acknowledged or not
uint8_t llc_tx(uint8_t pri, uint8_t addr, uint8_t len, void *ptr);
// Receive a packet
void llc_rx(struct llc_packet_t *pkt);
uint8_t llc_written();
// Copy data to tx queue
void llc_write(void *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif
