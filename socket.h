#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>

#ifndef SIMULATION
// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>
#endif

#ifdef SIMULATION
#include "simulation.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SOCKETS	16

enum SocketStatus {SOCKET_ALLOCATED = 0x7f, SOCKET_ACTIVE = 0x80, SOCKET_FREE = 0};
enum SocketTypes {SOCKET_LISTEN = 0, SOCKET_CONNECTION = 1, SOCKET_DATAGRAM = 2};

struct socket_t {
	//uint8_t address;
	uint8_t port;//, sport;

	// enum SocketStatus
	uint8_t status;
	//uint8_t type;
	QueueHandle_t queue;
};

extern struct socket_t sockets[MAX_SOCKETS];

void socket_init();
#ifdef SOCKET_TCP
// Listen on a port
void listen(uint8_t sid, uint16_t port);
// Accept a pending new connection
uint8_t accept(uint8_t sid);
// Read from socket buffer
uint8_t soc_read(uint8_t sid, uint8_t *buffer, uint8_t len);
#endif
// Bind a socket into listening mode
void soc_bind(uint8_t sid, uint8_t port);

// Implemented externally
// Alloc a socket
uint8_t  soc_socket();
// Read data from socket queue
uint8_t soc_recfrom(uint8_t sid, void *buf, uint8_t *len, uint8_t *addr, uint8_t *port);
// Send data
uint8_t soc_sendto(uint8_t sid, void *buf, uint8_t len, uint8_t addr, uint8_t port);

#ifdef __cplusplus
}
#endif

#endif
