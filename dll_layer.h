#ifndef DLL_LAYER_H
#define DLL_LAYER_H

#include <stdint.h>

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
// Check if packets buffered ready for read
uint8_t dll_available();
// Take 1 packet from buffer, return the actual packet size read
uint8_t dll_read(uint8_t *packet);

// Interface with lower PHY layer

// Receive data stream from PHY, called from ISR
void dll_data_handler(const uint8_t data);
// Transmit data stream request from PHY, called from ISR
uint8_t dll_data_request(uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif
