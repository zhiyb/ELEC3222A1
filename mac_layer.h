#ifndef MAC_LAYER_H
#define MAC_LAYER_H

#include <stdint.h>

// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

// Media access control layer: transmission in unit of frames

// Broadcast destination MAC address
#define MAC_BROADCAST	0xff

// Initialisation
void mac_init();

// Interface with upper LLC layer

// Data transfer between MAC and upper layer
struct mac_frame {
	uint8_t addr;	// Destination MAC address to transmit
			// ... or source MAC address received
	uint8_t len;	// Data length
	void *ptr;	// Pointer to frame data on heap (free after use)
	void *payload;	// Pointer to payload field
};

// RTOS queue for frame transmission & reception
// Queue item size: mac_frame
extern QueueHandle_t mac_tx, mac_rx;

uint8_t mac_written();
// Copy data to tx queue
void mac_write(uint8_t addr, void *data, uint8_t length);
// MAC address
uint8_t mac_address();
// Set MAC address
uint8_t mac_address_update(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif
