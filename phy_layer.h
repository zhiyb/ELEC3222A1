#ifndef PHY_LAYER_H
#define PHY_LAYER_H

// If transceiver is ready to transmit (channel free)
uint8_t phy_ready();
// Push data into TX buffer
void phy_send(uint8_t length, char *buffer);
// For unblocking operation, check if data available for receive
uint8_t phy_available();
// Pull data from RX buffer
uint8_t phy_recv(uint8_t length, char *buffer);

#endif
