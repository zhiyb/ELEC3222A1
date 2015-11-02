#ifndef TRAN_LAYER_H
#define TRAN_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

// For use by APP

void tran_send(char *buffer, uint16_t length);
// For unblocking operation, check if data available for receive
uint8_t tran_available();
// Return actual bytes read
uint16_t tran_recv(char *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
