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

void net_tx(const uint8_t *packet, uint8_t address);

void net_rx_task(void *param);

#ifdef __cplusplus
}
#endif

#endif
