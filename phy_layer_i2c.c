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
	//volatile uint8_t mode;
	volatile uint8_t status;
	volatile uint8_t transmit;
};

static struct phy_ctrl_t ctrl;

QueueHandle_t phy_tx, phy_rx;
static QueueHandle_t phy_m;

// Returns current hardware mode (enum PHYModes)
uint8_t phy_mode()
{
	return (TWSR & (0xff << 3)) >= 0x60 ? PHYRX : PHYTX;
}

// Whether channel is free
uint8_t phy_free()
{
	return /*phy_mode() == PHYRX &&*/ ctrl.status == 0;
}

// Start transmission
void phy_transmit()
{
	//cli();
	ctrl.transmit = 1;
	//printf("s%02x,", (TWSR & (0xff << 3)));
	if (/*phy_mode() == PHYRX &&*/ ctrl.status == 0 && (TWSR & (0xff << 3)) == 0xf8) {
		TWCR = TWCR_ACT | _BV(TWSTA);
		//ctrl.mode = PHYTX;
	} else {
		//while (ctrl.transmit);
	}
	//sei();
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
	while (xQueuePeek(phy_tx, &data, portMAX_DELAY) != pdTRUE);
	//puts_P(PSTR("\e[96mPHY TX received."));
	xQueueReset(phy_m);
	// Put rfm12b to TX mode
	phy_transmit();
	// Wait for transmit complete (return to RX mode)
	while (xQueueReceive(phy_m, &data, portMAX_DELAY) != pdTRUE || data != PHYRX);
	//puts_P(PSTR("\e[96mPHY TX transmitted."));
	goto loop;
}

ISR(TWI_vect)
{
	uint8_t status;
	BaseType_t xTaskWoken = pdFALSE;
	ctrl.status = status = TWSR & (0xff << 3);
	//printf("$%02x,", status);
	//if (ctrl.mode == PHYRX) {	// Slave receiver mode
		//TWCR &= ~(_BV(TWSTA) | _BV(TWSTO));
		switch (status) {
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
				xQueueSendToBackFromISR(phy_rx, &data, &xTaskWoken);
			}
			TWCR = TWCR_ACT;
			break;
		case 0xa0:	// STOP or repeated START received
			ctrl.status = 0;
			//putchar('\n');
			if (ctrl.transmit) {
				//TWCR |= _BV(TWSTA);
				TWCR = TWCR_ACT | _BV(TWSTA);
				//ctrl.mode = PHYTX;
				goto ret;
			}
			TWCR = TWCR_ACT;
			break;
#if 0
		default:
			printf("%%%01x%02x%%", ctrl.mode, status);
		}
		TWCR &= ~(_BV(TWSTA) | _BV(TWSTO));
	} else {			// Master transmitter mode
		switch (status) {
#endif
		case 0x08:	// START transmitted
		case 0x10:	// Repeated START transmitted
			TWDR = 0;	// General call, write
			//TWCR &= ~_BV(TWSTA);
			TWCR = TWCR_ACT;
			break;
		case 0x18:	// SLA+W transmitted; ACK received
		case 0x20:	// ...NACK received
		case 0x28:	// Data transmitted; ACK received
		case 0x30:	// ...NACK received
			{
				uint8_t data;
				if (xQueueReceiveFromISR(phy_tx, &data, &xTaskWoken) == pdTRUE) {
					TWDR = data;
					TWCR = TWCR_ACT;
				} else {
					TWCR = TWCR_ACT | _BV(TWSTO);
					//putchar('\n');
					//TWCR |= _BV(TWSTO);
					data = /*ctrl.mode =*/ PHYRX;
					ctrl.status = 0;
					ctrl.transmit = 0;
					xQueueSendToBackFromISR(phy_m, &data, &xTaskWoken);
				}
			}
			break;
		case 0x38:	// Arbitration lost in SLA+W or data
			//TWCR |= _BV(TWSTA);	// Transmit START again
			TWCR = TWCR_ACT | _BV(TWSTA);
			//printf("%%%01x%02x%%", ctrl.mode, status);
			printf("!%02x,", status);
			break;
		default:
			//printf("%%%01x%02x%%", ctrl.mode, status);
			printf("!%02x,", status);
		}
	//}
ret:
	//TWCR |= _BV(TWINT);

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

	phy_rx = xQueueCreate(16, 1);
	phy_tx = xQueueCreate(16, 1);
	phy_m = xQueueCreate(1, 1);

	//ctrl.mode = PHYRX;
	ctrl.status = 0;
	ctrl.transmit = 0;

	// Arbitrary delay for avoid exact same time I2C transmission
	uint8_t i = mac_address();
	while (i--)
		_delay_ms(10);

	xTaskCreate(phy_tx_task, "PHY TX", 128, NULL, tskINT_PRIORITY, NULL);
}
