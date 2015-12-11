#ifndef TRAN_LAYER_H
#define TRAN_LAYER_H

#include <stdint.h>
#ifndef SIMULATION
// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
#ifdef SIMULATION
#include "simulation.h"
#endif

#define TRAN_PACKET_MAX_SIZE	121

struct package{
	uint8_t control[2];
	uint8_t src_port;
	uint8_t dest_port;
	uint8_t length;
	uint8_t app_data[0];
};

struct app_packet
{
	uint8_t len;
	uint8_t addr;
    uint8_t *data;
};
// Initialisation
void tran_init();

// Read data from socket queue
uint8_t soc_recfrom(uint8_t sid, void *buf, uint8_t *len, uint8_t *addr);
// Send data
uint8_t soc_sendto(uint8_t sid, void *buf, uint8_t len, uint8_t addr);


extern QueueHandle_t tran_rx;
#ifdef __cplusplus
}
#endif

#endif
