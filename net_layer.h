#ifndef NET_LAYER_H
#define NET_LAYER_H

void net_send(uint8_t length, char *buffer);
// For unblocking operation, check if data available for receive
uint8_t net_available();
uint8_t net_recv(uint8_t length, char *buffer);

#endif
