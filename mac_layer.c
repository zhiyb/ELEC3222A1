#include <avr/pgmspace.h>
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
// Exclude header & footer
#define FRAME_MAX_SIZE		(32 - 2)

enum Status {WaitingHeader = 0, ReceivingData, Escaped};
static uint8_t status = WaitingHeader;

void mac_tx_task(void *param)
{
	static struct mac_frame data;
	puts_P(PSTR("\e[96mMAC TX task initialised."));

loop:
	// Receive 1 frame from queue
	while (xQueueReceive(mac_tx, &data, portMAX_DELAY) != pdTRUE);
	if (data.len == 0)
		goto loop;

	// CSMA
	if (phy_mode() == PHYRX)
		do {
			while (rand() % 4096 >= 4096 * CSMA)
				vTaskDelay(1);
		} while (status != WaitingHeader || !phy_free());

	// Start transmission
	uint8_t *ptr = data.ptr;

	// Transmit header
	uint8_t tmp = FRAME_HEADER;
	while (xQueueSendToBack(phy_tx, &tmp, portMAX_DELAY) != pdTRUE);

	// Transmit payload data
	uint16_t checksum = 0;
	while (data.len--) {
		if (*ptr == FRAME_HEADER || *ptr == FRAME_FOOTER) {
			// An escape byte required
			tmp = FRAME_ESCAPE;
			while (xQueueSendToBack(phy_tx, &tmp, portMAX_DELAY) != pdTRUE);
		}
		while (xQueueSendToBack(phy_tx, ptr, portMAX_DELAY) != pdTRUE);
		checksum += *ptr++;
	}
	vPortFree(data.ptr);

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
	static uint8_t *buffer = 0;

	//static struct mac_frame data;
	static uint8_t *ptr = 0;
	static uint8_t size = 0;
	static uint16_t checksum;
	uint8_t data;
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
		if (buffer == 0 && (buffer = pvPortMalloc(FRAME_MAX_SIZE)) == 0)
			break;

		status = ReceivingData;
		ptr = buffer;
		size = 0;
		checksum = 0;
		break;

	case ReceivingData:
		switch (data) {
		//case FRAME_HEADER:	// Same as FOOTER
		case FRAME_FOOTER:
			if (size >= 3) {
				// Check checksum for frame corruption
				// Higher byte in received checksum (little endian)
				uint8_t ckh = *(ptr - 1);
				checksum = checksum + 0x0100 * ckh - ckh;

				if (checksum == 0) {	// Checksum passed
					// Send frame to upper layer
					struct mac_frame data;
					data.len = size - 2;	// Remove checksum
					data.ptr = buffer;
					// Send to upper layer
					xQueueSendToBack(mac_rx, &data, portMAX_DELAY);
					buffer = pvPortMalloc(FRAME_MAX_SIZE);
					if (buffer == 0) {
						status = WaitingHeader;
						break;
					}
				}
			}
			// Reset
			ptr = buffer;
			size = 0;
			checksum = 0;
			goto loop;
		case FRAME_ESCAPE:
			status = Escaped;
			goto loop;
		}

		// Data reception
	case Escaped:
		// Buffer overwrite
		if (size++ == FRAME_MAX_SIZE) {
			status = WaitingHeader;
			break;
		}
		checksum += *ptr++ = data;
		break;
	};
	goto loop;
}

void mac_write(void *data, uint8_t length)
{
	uint8_t *ptr;
alloc:
	ptr = pvPortMalloc(length);
	if (ptr == 0) {
		puts_P(PSTR("malloc failed!"));
		goto alloc;
	}
	memcpy(ptr, data, length);
	struct mac_frame item;
	item.len = length;
	item.ptr = ptr;
	while (xQueueSendToBack(mac_tx, &item, portMAX_DELAY) != pdTRUE);
}

uint8_t mac_written()
{
	return phy_mode() != PHYTX && uxQueueMessagesWaiting(mac_tx) == 0;
}

void mac_init()
{
	puts_P(PSTR("\e[96mMAC task setting up..."));
	mac_tx = xQueueCreate(1, sizeof(struct mac_frame));
	mac_rx = xQueueCreate(3, sizeof(struct mac_frame));
	xTaskCreate(mac_tx_task, "MAC TX", 128, NULL, tskPROT_PRIORITY, NULL);
	xTaskCreate(mac_rx_task, "MAC RX", 128, NULL, tskPROT_PRIORITY, NULL);
}
