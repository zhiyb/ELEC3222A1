#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SOCKETS	16

enum SocketStatus {SOCKET_ALLOCATED = 0, SOCKET_ACTIVE = 1, SOCKET_FREE = 0xff};

struct socket_t {
	uint16_t address;
	uint16_t port;

	// enum SocketStatus
	uint8_t status;

	// Destination MAC address, used by NET & MAC
	uint8_t dest_mac;

	// Logical Link Control Layer
	struct {
		uint8_t status;	// State machine
		uint16_t frame;	// Frame sequence number
	} llc;
};

extern struct socket_t sockets[MAX_SOCKETS];

void socket_init();

#ifdef __cplusplus
}
#endif

#endif
