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

// Interface with upper LLC layer

enum LLC_Primitives {DL_UNITDATA = 0, DL_DATA_ACK, DL_DATA_ACK_STATUS};

// Structure of primitive to/from LLC layer
struct llc_pri {
	uint8_t pri;	// Primitives
	uint8_t len;	// Data length
	uint8_t *ptr;	// Pointer to data
};

// Pass primitives to LLC layer
QueueHandle_t to_llc;
// Primitives passed from LLC layer
QueueHandle_t from_llc;

#ifdef __cplusplus
}
#endif

#endif
