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

// Returns current hardware mode (enum PHYModes)
uint8_t phy_mode();
// Whether channel is free
uint8_t phy_free();
// Start transmission
void phy_transmit();
// Reset receive mode (waiting for sync byte)
void phy_receive();

#ifdef __cplusplus
}
#endif

#endif
