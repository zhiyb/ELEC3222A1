#ifndef PHY_LAYER_H
#define PHY_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

// Actual physical layer responsible for hardware operation

// To be accessed by upper DLL layer

enum PHYModes {PHYRX = 0, PHYTX};

// Returns current hardware mode (enum PHYModes)
uint8_t phy_mode();
// Whether channel is free
void phy_free();
// Start transmission
void phy_transmit();

#ifdef __cplusplus
}
#endif

#endif
