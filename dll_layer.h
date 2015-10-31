#ifndef DLL_LAYER_H
#define DLL_LAYER_H

void dll_send(uint8_t length, char *buffer);
// For unblocking operation, check if data available for receive
uint8_t dll_available();
uint8_t dll_recv(uint8_t length, char *buffer);

#endif
