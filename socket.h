#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>

// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SOCKETS	16

enum SocketStatus {SOCKET_ALLOCATED = 0, SOCKET_ACTIVE = 1, SOCKET_FREE = 0xff};
enum SocketTypes {SOCKET_LISTEN = 0, SOCKET_CONNECTION = 1, SOCKET_DATAGRAM = 2};

struct socket_t {
	uint8_t address;
	uint8_t port, sport;

	// enum SocketStatus
	uint8_t status;
	uint8_t type;
	QueueHandler queue;
};

extern struct socket_t sockets[MAX_SOCKETS];

void socket_init();
// Alloc a socket
uint8_t  socket();
// Listen on a port
void listen(uint8_t sid, uint16_t port);
// Accept a pending new connection
uint8_t accept(uint8_t sid);
// Read from socket buffer
uint8_t read(uint8_t sid, uint8_t *buffer, uint8_t len);
// Bind a socket into listening mode
void bind(uint8_t sid, uint8_t port);
//
#ifdef __cplusplus
}
#endif

#endif
