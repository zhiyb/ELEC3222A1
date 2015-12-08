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


#define TRAN_PACKET_MAX_SIZE	121

// Initialisation
void tran_init();

// Interface to be used by APP(s)
// Pack the data into the structure as the design
void pack(void);
// Transfer function for the transport layer
void tran_tx(struct socket soc, uint8_t len, const void *data);
// Receiving task in transport layer
void tran_rx_task(void *param);
// Read data from socket queue
uint8_t rec_from(uint8_t sid. void *buf, uint8_t len, uint8_t addr);
externn QueueHandle_t tran_tx, tran_rx;
#ifdef __cplusplus
}
#endif

#endif
