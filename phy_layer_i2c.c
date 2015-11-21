// Author: Yubo Zhi (normanzyb@gmail.com)

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "phy_layer.h"
#include "mac_layer.h"

#include <task.h>

// I2C multi-master broadcast
// Master transmitter & slave receiver mode

#define I2C_ADDR	0x39
#define I2C_FREQ	(115200UL)
#define I2C_SCL		_BV(0)
#define I2C_SDA		_BV(1)
#define TWCR_ACT	(_BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE))

struct phy_ctrl_t
{
	volatile uint8_t status;
	volatile uint8_t transmit;
};

static struct phy_ctrl_t ctrl;

QueueHandle_t phy_tx_queue, phy_rx_queue;
static QueueHandle_t phy_m;

// Returns current hardware mode (enum PHYModes)
uint8_t phy_mode()
{
	return (TWSR & (0xff << 3)) >= 0x60 ? PHYRX : PHYTX;
}

// Whether channel is free
uint8_t phy_free()
{
	return ctrl.status == 0;
}

// Start transmission
void phy_transmit()
{
	ctrl.transmit = 1;
	if (ctrl.status == 0 && (TWSR & (0xff << 3)) == 0xf8)
		TWCR = TWCR_ACT | _BV(TWSTA);
}

// Reset receive mode (waiting for sync byte)
void phy_receive()
{
}

void phy_tx_task(void *param)
{
	uint8_t data;
	puts_P(PSTR("\e[96mPHY TX task initialised."));

loop:
	// Wait for item available to transmit
	while (xQueuePeek(phy_tx_queue, &data, portMAX_DELAY) != pdTRUE);
	// Put rfm12b to TX mode
	phy_transmit();
	xQueueReset(phy_m);
	// Wait for transmit complete (return to RX mode)
	while (xQueueReceive(phy_m, &data, portMAX_DELAY) != pdTRUE || data != PHYRX);
	goto loop;
}

void phy_tx(uint8_t data)
{
	while (xQueueSendToBack(phy_tx_queue, &data, portMAX_DELAY) != pdTRUE);
	phy_transmit();	// Put rfm12b into TX mode
}

uint8_t phy_rx()
{
	uint8_t data;
	while (xQueueReceive(phy_rx_queue, &data, portMAX_DELAY) != pdTRUE);
	return data;
}

ISR(TWI_vect)
{
	uint8_t status;
	BaseType_t xTaskWoken = pdFALSE;
	ctrl.status = status = TWSR & (0xff << 3);
	//printf("$%02x,", status);

	switch (status) {
	// Slave receiver mode
	case 0x60:	// SLA+W received; ACK returned
	case 0x68:	// Arbitration lost in SLA+R/W (master); SLA+W received; ACK returned
	case 0x70:	// General call received; ACK returned
	case 0x78:	// Arbitration lost in SLA+R/W (master); General call received; ACK returned
		TWCR = TWCR_ACT;
		break;
	case 0x80:	// Addressed with SLA+W; Data received; ACK returned
	case 0x88:	// ...NACK returned
	case 0x90:	// Addressed with general call; Data received; ACK returned
	case 0x98:	// ...NACK returned
		{
			uint8_t data = TWDR;
			xQueueSendToBackFromISR(phy_rx_queue, &data, &xTaskWoken);
		}
		TWCR = TWCR_ACT;
		break;
	case 0xa0:	// STOP or repeated START received
		ctrl.status = 0;
		if (ctrl.transmit) {
			TWCR = TWCR_ACT | _BV(TWSTA);
			break;
		}
		TWCR = TWCR_ACT;
		break;

	// Master transmitter mode
	case 0x08:	// START transmitted
	case 0x10:	// Repeated START transmitted
		TWDR = 0;	// General call, write
		TWCR = TWCR_ACT;
		break;
	case 0x18:	// SLA+W transmitted; ACK received
	case 0x20:	// ...NACK received
	case 0x28:	// Data transmitted; ACK received
	case 0x30:	// ...NACK received
		{
			uint8_t data;
			if (xQueueReceiveFromISR(phy_tx_queue, &data, &xTaskWoken) == pdTRUE) {
				TWDR = data;
				TWCR = TWCR_ACT;
			} else {
				TWCR = TWCR_ACT | _BV(TWSTO);
				data = PHYRX;
				ctrl.status = 0;
				ctrl.transmit = 0;
				xQueueSendToBackFromISR(phy_m, &data, &xTaskWoken);
			}
		}
		break;
	case 0x38:	// Arbitration lost in SLA+W or data
		//printf("!%02x,", status);
		TWCR = TWCR_ACT;
		break;
	default:
		printf("!%02x,", status);
	}

	if (xTaskWoken)
		taskYIELD();
}

void phy_init()
{
	// I2C port initialisation
	DDRC &= ~(I2C_SCL | I2C_SDA);
	PORTC |= I2C_SCL | I2C_SDA;

	// I2C module initialisation
	// Prescaler of 1
	TWBR = (F_CPU / I2C_FREQ - 16) / 2 / 1;
	TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
	TWSR = 0;
	TWAR = (I2C_ADDR << 1) | _BV(TWGCE);
	TWAMR = 0;

	phy_rx_queue = xQueueCreate(16, 1);
	phy_tx_queue = xQueueCreate(8, 1);
	phy_m = xQueueCreate(1, 1);

	ctrl.status = 0;
	ctrl.transmit = 0;

	// Arbitrary delay for avoid exact same time I2C transmission
	uint8_t i = mac_address();
	while (i--)
		_delay_ms(10);
}
