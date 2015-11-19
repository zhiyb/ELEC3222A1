#ifndef PHY_LAYER_H
#define PHY_LAYER_H

// RTOS
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

// Actual physical layer responsible for hardware operation

// Initialisation
void phy_init();

// Interface with upper DLL layer

enum PHYModes {PHYRX = 0, PHYTX};

// RTOS queue for data transmission & reception
// Queue item size: 1 byte
// Write to phy_tx will start transmission immediately
extern QueueHandle_t phy_rx, phy_tx;

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
