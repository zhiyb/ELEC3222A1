#ifndef TRAN_LAYER_H
#define TRAN_LAYER_H

#include <stdint.h>

// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

// TRAN layer: services routing, error control

#define TRAN_PACKET_MAX_SIZE	121

// Initialisation
void tran_init();

// Interface to be used by APP(s)
// Pack the data into the structure as the design
void pack(void);
// Transfer the data to the next layer
void trans(void);
// Get the socket in order to know the port and addr
void get_soc(struct socket *a)
// Data length limited only by (uint16_t)
// Buffer data stream and start transmission
void tran_write(const uint8_t *data, uint16_t length);
// Read <length> bytes of data, return the actual bytes read
//uint16_t tran_read(uint8_t *data, uint16_t length);
void unpack(uint8_t len_data, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
