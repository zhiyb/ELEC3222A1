#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <colours.h>
#include "llc_layer.h"
#include "mac_layer.h"
#include "phy_layer.h"

#include <task.h>
#include <semphr.h>

#define CSMA		(0.1)
#define CSMA_TIME	5

// Field definition
#define FRAME_HEADER		0xaa
#define FRAME_FOOTER		0xaa
#define FRAME_ESCAPE		0xcc
// Appended bytes (Address & checksum)
#define FRAME_MIN_SIZE		(4)
// Exclude header & footer
#define FRAME_MAX_SIZE		(32 - 2)

static EEMEM uint8_t NVAddress = ADDRESS;
static SemaphoreHandle_t mac_semaphore;
QueueHandle_t mac_rx;

enum Status {WaitingHeader = 0, ReceivingData, ReceivingEscaped, FooterReceived};
static uint8_t status = WaitingHeader, mac_addr = MAC_BROADCAST;

struct mac_buffer_t {
	uint8_t dest;
	uint8_t src;
	uint8_t payload[FRAME_MAX_SIZE - 2];
};

// Transmit 1 byte though PHY, escaping if necessary
static void mac_tx_data(uint8_t data)
{
	// If escaping required
	if (data == FRAME_HEADER || data == FRAME_FOOTER || data == FRAME_ESCAPE)
		phy_tx(FRAME_ESCAPE);
	phy_tx(data);
}

void mac_tx(uint8_t addr, void *data, uint8_t len)
{
#if MAC_DEBUG > 1
	printf_P(PSTR(ESC_BLUE "MAC-TX,%02x-%u;"), addr, len);
#endif

	// Thread safe: lock mutex
	while (xSemaphoreTake(mac_semaphore, portMAX_DELAY) != pdTRUE);

	// CSMA
	while (phy_mode() == PHYTX)
		vTaskDelay(CSMA_TIME);

	do {
		while (rand() % 4096 >= 4096 * CSMA)
			vTaskDelay(CSMA_TIME);
	} while (!phy_free());

	// Start transmission
	uint8_t *ptr = data;

	// Transmit header
	phy_tx(FRAME_HEADER);
	uint16_t checksum = 0;

	// Transmit address
	// Destination address
	mac_tx_data(addr);
	checksum += addr;
	// Source (self) address
	uint8_t tmp = mac_address();
	mac_tx_data(tmp);
	checksum += tmp;

	// Transmit payload data
	tmp = len;
	while (tmp--) {
		mac_tx_data(*ptr);
		checksum += *ptr++;
	}

	// Transmit checksum
	checksum = -checksum;
	ptr = (uint8_t *)&checksum;
	mac_tx_data(*ptr++);
	mac_tx_data(*ptr);

	// Transmit footer
	phy_tx(FRAME_FOOTER);

	// Thread-safe: unlock mutex
	xSemaphoreGive(mac_semaphore);
}

void mac_rx_task(void *param)
{
	static struct mac_frame frame;
	struct mac_buffer_t *buffer = 0;
	uint8_t *ptr = 0, size = 0, data;
	uint16_t checksum = 0;
#if MAC_DEBUG > 0
	puts_P(PSTR(ESC_CYAN "MAC RX task initialised."));
#endif

loop:
	// Receive 1 byte data from PHY
	data = phy_rx();
#if MAC_DEBUG > 2
	if (data == FRAME_HEADER) {
		fputs_P(PSTR(ESC_GREY), stdout);
		putchar('^');
	} else if (data == FRAME_ESCAPE)
		putchar('`');
	else if (isprint(data))
		putchar(data);
	else {
		putchar('\\');
		putchar('0' + ((data >> 6) & 0x07));
		putchar('0' + ((data >> 3) & 0x07));
		putchar('0' + ((data >> 0) & 0x07));
	}
	//putchar(';');
#endif

	switch (status) {
	case WaitingHeader:
		if (data != FRAME_HEADER) {
			phy_receive();	// Reset receiver
#if MAC_DEBUG > 2
			fputs_P(PSTR(ESC_MAGENTA "MAC-RST;"), stdout);
#endif
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

				// Broadcast source address is invalid
				if (buffer->src == MAC_BROADCAST)
					goto loop;
#if 0
				// Not for this station
				if (buffer->dest != MAC_BROADCAST && buffer->dest != mac_address())
					goto loop;
#endif
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
					if (buffer->src == mac_address())
						mac_address_update(mac_address() + 1);
					// Send to upper layer
					if (xQueueSendToBack(mac_rx, &frame, 0) == pdTRUE) {
#if MAC_DEBUG > 1
						fputs_P(PSTR(ESC_YELLOW "MAC-RECV;"), stdout);
#endif
						buffer = pvPortMalloc(sizeof(struct mac_buffer_t));
						if (buffer == 0) {
							status = WaitingHeader;
#if MAC_DEBUG >= 0
							fputs_P(PSTR(ESC_GREY "MAC-MEM-FAIL;"), stdout);
#endif
							goto loop;
						}
					} else {
#if MAC_DEBUG > 0
						fputs_P(PSTR(ESC_GREY "MAC-Q-FAIL;"), stdout);
#endif
					}
				} else {
#if MAC_DEBUG > 1
					fputs_P(PSTR(ESC_MAGENTA "MAC-DROP;"), stdout);
#endif
				}
			} else {
				status = ReceivingData;
#if MAC_DEBUG > 2
				fputs_P(PSTR(ESC_MAGENTA "MAC-RST;"), stdout);
#endif
			}

			// Reset
			ptr = (uint8_t *)buffer;
			size = 0;
			checksum = 0;
			goto loop;
		case FRAME_ESCAPE:
			status = ReceivingEscaped;
#if MAC_DEBUG > 2
			fputs_P(PSTR(ESC_GREY "MAC-ESC;"), stdout);
#endif
			goto loop;
		}

		// Data reception
	case ReceivingEscaped:
		status = ReceivingData;
		// Buffer overwrite
		if (size++ == FRAME_MAX_SIZE) {
			status = WaitingHeader;
			break;
		}
		checksum += *ptr++ = data;
#if 1
		if (size == 1)	// Destination address received
			if (buffer->dest != mac_address() && buffer->dest != MAC_BROADCAST) {
				status = WaitingHeader;	// Not for this station
#if MAC_DEBUG > 2
				fputs_P(PSTR(ESC_GREY "MAC-SKIP;"), stdout);
#endif
			}
#endif
		break;
	};
	goto loop;
}

uint8_t mac_written()
{
	return phy_mode() == PHYRX;
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
	mac_rx = xQueueCreate(6, sizeof(struct mac_frame));
	while ((mac_semaphore = xSemaphoreCreateMutex()) == NULL);
	while (xTaskCreate(mac_rx_task, "MAC RX", configMINIMAL_STACK_SIZE, NULL, tskHIGH_PRIORITY, NULL) != pdPASS);
}
