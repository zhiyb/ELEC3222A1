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

// Initialisation
void mac_init();

// Interface with upper LLC layer

// Data pass to MAC from upper layer
struct mac_frame {
	uint8_t len;	// Data length
	void *ptr;	// Pointer to data (free after use)
};

// RTOS queue for frame transmission & reception
// Queue item size: mac_frame
QueueHandle_t mac_tx, mac_rx;

uint8_t mac_written();
// Copy data to tx queue
void mac_write(void *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif
