#ifndef PHY_LAYER_H
#define PHY_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

// Push data into TX buffer
void phy_send(uint8_t length, char *buffer);
// For unblocking operation, check if data available for receive
uint8_t phy_available();
// Pull data from RX buffer
uint8_t phy_recv(uint8_t length, char *buffer);

#ifdef __cplusplus
}
#endif

#endif
