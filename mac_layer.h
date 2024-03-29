#ifndef MAC_LAYER_H
#define MAC_LAYER_H

#include <stdint.h>

// RTOS
#ifndef SIMULATION
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>
#else
#include <simulation.h>
#endif

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
extern QueueHandle_t mac_rx;

// Transmit through MAC layer
// Thread-safe
void mac_tx(uint8_t addr, void *data, uint8_t len);
uint8_t mac_written();
// MAC address
uint8_t mac_address();
// Set MAC address
uint8_t mac_address_update(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif
