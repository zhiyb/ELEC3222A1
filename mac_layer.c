#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "llc_layer.h"
#include "mac_layer.h"
#include "phy_layer.h"

#include <task.h>

#define CSMA	(0.1)

// Field definition
#define FRAME_HEADER		0xaa
#define FRAME_FOOTER		0xaa
#define FRAME_ESCAPE		0xcc
// Appended bytes (Address & checksum)
#define FRAME_MIN_SIZE		(4)
// Exclude header & footer
#define FRAME_MAX_SIZE		(32 - 2)

static EEMEM uint8_t NVAddress = ADDRESS;
QueueHandle_t mac_tx, mac_rx;

enum Status {WaitingHeader = 0, ReceivingData, ReceivingEscaped, FooterReceived};
static uint8_t status = WaitingHeader, mac_addr = MAC_BROADCAST;

struct mac_buffer_t {
	uint8_t dest;
	uint8_t src;
	uint8_t payload[FRAME_MAX_SIZE - 2];
};

void mac_tx_task(void *param)
{
	static struct mac_frame frame;
	puts_P(PSTR("\e[96mMAC TX task initialised."));

loop:
	// Receive 1 frame from queue
	while (xQueueReceive(mac_tx, &frame, portMAX_DELAY) != pdTRUE);

	// CSMA
	if (phy_mode() == PHYRX)
		do {
			while (rand() % 4096 >= 4096 * CSMA)
				vTaskDelay(1);
		} while (!phy_free());

	// Start transmission
	uint8_t *ptr = frame.payload;

	// Transmit header
	uint8_t tmp = FRAME_HEADER;
	while (xQueueSendToBack(phy_tx, &tmp, portMAX_DELAY) != pdTRUE);
	uint16_t checksum = 0;

	// Transmit address
	// Destination address
	while (xQueueSendToBack(phy_tx, &frame.addr, portMAX_DELAY) != pdTRUE);
	checksum += frame.addr;
	// Source (self) address
	tmp = mac_address();
	while (xQueueSendToBack(phy_tx, &tmp, portMAX_DELAY) != pdTRUE);
	checksum += tmp;

	// Transmit payload data
	uint8_t len = frame.len;
	while (len--) {
		if (*ptr == FRAME_HEADER || *ptr == FRAME_FOOTER) {
			// An escape byte required
			tmp = FRAME_ESCAPE;
			while (xQueueSendToBack(phy_tx, &tmp, portMAX_DELAY) != pdTRUE);
		}
		while (xQueueSendToBack(phy_tx, ptr, portMAX_DELAY) != pdTRUE);
		checksum += *ptr++;
	}
	vPortFree(frame.ptr);

	// Transmit checksum
	checksum = -checksum;
	ptr = (uint8_t *)&checksum;
	while (xQueueSendToBack(phy_tx, ptr, portMAX_DELAY) != pdTRUE);
	ptr++;
	while (xQueueSendToBack(phy_tx, ptr, portMAX_DELAY) != pdTRUE);

	// Transmit footer
	tmp = FRAME_FOOTER;
	while (xQueueSendToBack(phy_tx, &tmp, portMAX_DELAY) != pdTRUE);
	goto loop;
}

void mac_rx_task(void *param)
{
	static struct mac_frame frame;
	struct mac_buffer_t *buffer = 0;
	uint8_t *ptr = 0, size = 0, data;
	uint16_t checksum = 0;
	puts_P(PSTR("\e[96mMAC RX task initialised."));

loop:
	// Receive 1 byte data from PHY queue
	while (xQueueReceive(phy_rx, &data, portMAX_DELAY) != pdTRUE);
	switch (status) {
	case WaitingHeader:
		if (data != FRAME_HEADER) {
			phy_receive();	// Reset receiver
			break;
		}

		// Alloc buffer
		if (buffer == 0 && (buffer = pvPortMalloc(sizeof(struct mac_buffer_t))) == 0)
			break;

		status = ReceivingData;
		ptr = (uint8_t *)buffer;
		size = 0;
		checksum = 0;
		break;

	case FooterReceived:
	case ReceivingData:
		switch (data) {
		//case FRAME_HEADER:	// Same as FOOTER
		case FRAME_FOOTER:
			if (size > FRAME_MIN_SIZE) {
				status = FooterReceived;
				// Check checksum for frame corruption
				// Higher byte in received checksum (little endian)
				uint8_t ckh = *(ptr - 1);
				checksum = checksum + 0x0100 * ckh - ckh;

				if (checksum == 0) {	// Checksum passed
					// Send frame to upper layer
					frame.addr = buffer->src;
					frame.len = size - FRAME_MIN_SIZE;	// Only payload length
					frame.ptr = buffer;
					frame.payload = buffer->payload;
					// Send to upper layer
					if (xQueueSendToBack(mac_rx, &frame, portMAX_DELAY) == pdTRUE) {
						buffer = pvPortMalloc(sizeof(struct mac_buffer_t));
						if (buffer == 0) {
							status = WaitingHeader;
							break;
						}
					}
				}
			} else
				status = ReceivingData;

			// Reset
			ptr = (uint8_t *)buffer;
			size = 0;
			checksum = 0;
			goto loop;
		case FRAME_ESCAPE:
			status = ReceivingEscaped;
			goto loop;
		}

		// Data reception
	case ReceivingEscaped:
		// Buffer overwrite
		if (size++ == FRAME_MAX_SIZE) {
			status = WaitingHeader;
			break;
		}
		checksum += *ptr++ = data;
		if (size == 1)	// Destination address received
			if (buffer->dest != mac_address() && buffer->dest != MAC_BROADCAST)
				status = WaitingHeader;	// Not for this station
		break;
	};
	goto loop;
}

void mac_write(uint8_t addr, void *data, uint8_t length)
{
	uint8_t *ptr;
	while ((ptr = pvPortMalloc(length)) == 0)
		puts_P(PSTR("malloc failed!"));
	memcpy(ptr, data, length);//(destination, source, number)
	struct mac_frame item;
	item.addr = addr;
	item.len = length;
	item.ptr = ptr;
	item.payload = ptr;
	while (xQueueSendToBack(mac_tx, &item, portMAX_DELAY) != pdTRUE);
	/////////////////(xQueue, pvItemtoQueue, xTicketsToWait)
}

uint8_t mac_written()
{
	return phy_mode() == PHYRX && uxQueueMessagesWaiting(mac_tx) == 0;
}

uint8_t mac_address_update(uint8_t addr)
{
	while (addr == MAC_BROADCAST)
		addr = rand() & 0xff;//generate a random new mac address
	eeprom_update_byte(&NVAddress, addr);
	mac_addr = addr;
	return mac_addr;
}

uint8_t mac_address()
{
	if (mac_addr != MAC_BROADCAST)//if mac_address is broadcast address, it should be updated. t
		// if they are the same then you won't know if the package is for you or broadcast.
		return mac_addr;
	return mac_address_update(eeprom_read_byte(&NVAddress));
}

void mac_init()
{
	puts_P(PSTR("\e[96mMAC task setting up..."));
	mac_tx = xQueueCreate(1, sizeof(struct mac_frame));
	mac_rx = xQueueCreate(2, sizeof(struct mac_frame));
	xTaskCreate(mac_tx_task, "MAC TX", configMINIMAL_STACK_SIZE, NULL, tskPROT_PRIORITY, NULL);
	xTaskCreate(mac_rx_task, "MAC RX", configMINIMAL_STACK_SIZE, NULL, tskPROT_PRIORITY, NULL);
}
