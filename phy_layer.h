#ifndef PHY_LAYER_H
#define PHY_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

// Actual physical layer responsible for hardware operation

// Initialisation
void phy_init();

// Interface with upper DLL layer

enum PHYModes {PHYRX = 0, PHYTX};

// Transmit 1 byte data (blocking)
void phy_tx(uint8_t data);
// Receive 1 byte data (blocking)
uint8_t phy_rx();

// Returns current hardware mode (enum PHYModes)
uint8_t phy_mode();
// Whether channel is free
uint8_t phy_free();
// Reset receive mode (waiting for sync byte)
void phy_receive();
// Disable PHY
void phy_disable();
// Enable PHY
void phy_enable();

#ifdef __cplusplus
}
#endif

#endif
