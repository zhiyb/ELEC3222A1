#ifndef NET_LAYER_H
#define NET_LAYER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// NET layer: machine routing, error control

#define NET_PACKET_MAX_SIZE	128

// Initialisation
void net_init();

// Interface with upper TRAN layer

// Maximum packet length: TRAN_PACKET_MAX_SIZE (121 bytes)
// Buffer a packet and start transmission
void net_write(const uint8_t *packet, uint8_t length);
// Take 1 packet from buffer, return the actual packet size read
// Return 0 means no packet available
uint8_t net_read(uint8_t *packet);

#ifdef __cplusplus
}
#endif

#endif
