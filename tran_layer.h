#ifndef TRAN_LAYER_H
#define TRAN_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

void tran_send(uint8_t length, char *buffer);
// For unblocking operation, check if data available for receive
uint8_t tran_available();
uint8_t tran_recv(uint8_t length, char *buffer);

#ifdef __cplusplus
}
#endif

#endif
