#include "socket.h"

struct socket_t sockets[MAX_SOCKETS];

void socket_init()
{
	uint8_t i = 0;
	for (i = 0; i < MAX_SOCKETS; i++)
		sockets[i].status = SOCKET_FREE;
}
