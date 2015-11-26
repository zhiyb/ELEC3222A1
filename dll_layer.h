#ifndef DLL_LAYER_H
#define DLL_LAYER_H

#include <stdint.h>

// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

// Data link layer: communication of whole unit of packets

// Initialisation
void dll_init();

// Interface with upper NET layer

// Maximum packet length: NET_PACKET_MAX_SIZE (128 bytes)
// Buffer a packet and start transmission
void dll_write(const uint8_t *packet, uint8_t length);
// Check if all buffered data was transmitted
uint8_t dll_written();
// Take 1 packet from buffer, return the actual packet size read
// Return 0 means no packet available
uint8_t dll_read(uint8_t *packet);

#ifdef __cplusplus
}
#endif

#endif
