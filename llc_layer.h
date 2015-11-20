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

#define LLC_FRAME_MAX_SIZE	28

// Initialisation
void llc_init();

// Interface with upper NET layer

enum LLC_Primitives {DL_UNITDATA = 0, DL_DATA_ACK, DL_DATA_ACK_STATUS};

// Data transfer between LLC and upper layer
struct llc_packet {
	uint8_t pri;	// LLC_Primitives
	uint8_t len;	// Data length
	void *ptr;	// Pointer to data (free after use)
};

// RTOS queue for packet transmission & reception
// Queue item size: llc_packet
extern QueueHandle_t llc_tx, llc_rx;

uint8_t llc_written();
// Copy data to tx queue
void llc_write(void *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif
