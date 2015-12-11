#include "socket.h"
#include <stdio.h>
#include "tran_layer.h"
#ifdef SIMULATION
#include "simulation.h"
#endif
struct socket_t sockets[MAX_SOCKETS];

void socket_init()
{
	uint8_t i = 0;
	for (i = 0; i < MAX_SOCKETS; i++)
		sockets[i].status = SOCKET_FREE;
	//printf("socket init");
}

#ifdef SOCKET_TCP
// Listen on a port
void soc_listen(uint8_t sid, uint16_t port)
{
	struct socket_t *sptr = sockets + sid;
	sptr->port = port;
	sptr->status = SOCKET_ACTIVE | SOCKET_DATAGRAM;
 //	sptr->queue = xQueue	
}

// Accept a pending new connection
uint8_t soc_accept(uint8_t sid)
{
	uint8_t id;
	struct socket_t *sptr = sockets + sid;
	xQueueReceive(sptr->queue, &id, portMAX_DELAY != pdTRUE);
	return id;
	//while(xQdueueReceive(net_rx, data, portMAX_DELAY) != pdTRUE);
}

// Read from socket buffer
uint8_t soc_read(uint8_t sid, uint8_t *buffer, uint8_t len)
{
	uint8_t i;
	struct socket_t *sptr = sockets + sid;
	for(i = 0; i != len; i++)
	{
		while(xQueueReceive(sptr->queue, buffer, portMAX_DELAY) != pdTRUE);
		buffer ++;
	}	//len = *buffer;
	//buffer++;
	return len;
}
#endif

void soc_bind(uint8_t sid, uint8_t port)
{
	// TODO: if binding on 0, then find a random unused number
	sockets[sid].status = SOCKET_ACTIVE | SOCKET_DATAGRAM;
	sockets[sid].port = port;
}
