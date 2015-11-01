#ifndef DLL_LAYER
#define DLL_LAYER

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Data link layer: unreliable, in order, in unit of NET packets

// Initialisation
void dll_init();

// Interface with upper NET layer

// Maximum packet length is defined by NET_PACKET_MAX_SIZE
// Send out a packet
void dll_send(uint8_t *data, uint8_t length);
// Whether packets received
void dll_available();
// Take 1 packet, return the actual packet size
uint8_t dll_read(uint8_t *data);

// Interface with lower PHY layer

// Receive data stream from PHY, called from ISR
void dll_data_handler(const uint8_t data);
// Transmit data stream request from PHY, called from ISR
uint8_t dll_data_request(uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif
