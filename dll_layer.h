#ifndef DLL_LAYER
#define DLL_LAYER

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Data link layer: unreliable, but in unit of packet

// To be accessed by upper NET layer

// Send out a packet
void dll_send(uint8_t *data, uint8_t length);
// Whether packets received
void dll_available();
// Take 1 packet, return the actual size
uint8_t dll_read(uint8_t *data);

// To be accessed by lower PHY layer

static inline uint8_t dll_sync_byte() {return 0xaa;}

void dll_data_handler(const uint8_t data);
uint8_t dll_data_request(uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif
