#ifndef SIMULATION
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <colours.h>
#endif
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "net_layer.h"
#include "llc_layer.h"
#include "mac_layer.h"

#ifndef SIMULATION
#include <task.h>
#include <semphr.h>
#else
#include <simulation.h>
#endif

#ifdef SIMULATION
#define LLC_DEBUG	-1
#endif

#define PACKET_MAX_SIZE		NET_PACKET_MAX_SIZE
#define FRAME_MIN_SIZE		FRAME_APPEND_SIZE
#define FRAME_MAX_SIZE		LLC_FRAME_MAX_SIZE
#define FRAME_APPEND_SIZE	3
#define FRAME_DATA_MAX_SIZE	(FRAME_MAX_SIZE - FRAME_APPEND_SIZE)
// Power of 2
#define ACN_CACHE_SIZE		8

#define RETRY_TIME	300
#define RETRY_COUNT	8

#define LLC_MUTEX

QueueHandle_t llc_rx;
static QueueHandle_t llc_ack;
#ifdef LLC_MUTEX
static SemaphoreHandle_t llc_semaphore;
#endif

union ctrl_t {
	uint8_t u8;
	struct {
		uint8_t ack:1;		// Is an ACK frame?
		uint8_t ackreq:1;	// Is ACK required?
		uint8_t acn:1;		// AC0 / AC1
		uint8_t last:1;		// Is last frame?
	} s;
};

struct llc_frame_t {
	union ctrl_t ctrl;
	uint8_t seq;			// Frame sequence number
	uint8_t len;			// Frame payload data length
	uint8_t payload[0];
};

struct llc_ack_t {
	union ctrl_t ctrl;
	uint8_t seq;
	uint8_t addr;
};

static struct llc_acn_t {
	uint8_t addr;	// MAC address
	uint8_t s, r;	// Sender, receiver
	uint8_t seq;	// Received frame sequence
	uint8_t *buffer;	// Data buffer
	uint8_t size;	// Current data buffer size
} acnCache[ACN_CACHE_SIZE];
static uint8_t lastACN;

void llc_report()
{
	uint8_t acnAct = 0, acnRec = 0;
	struct llc_acn_t *aptr = acnCache;
	uint8_t i = ACN_CACHE_SIZE;
	while (i--) {
		if (aptr->addr != MAC_BROADCAST)
			acnRec++;
		if (aptr->buffer != 0)
			acnAct++;
		aptr++;
	}
	printf_P(PSTR("LLC: recorded: %u, active: %u\n"), acnRec, acnAct);
}

static void llc_acn_reset(struct llc_acn_t *acn)
{
	if (acn->addr != MAC_BROADCAST && acn->buffer)
		vPortFree(acn->buffer);
	acn->addr = MAC_BROADCAST;
	acn->s = 0;
	acn->r = 0xff;
	acn->seq = 0xff;
	acn->buffer = 0;
	acn->size = 0;
}

static struct llc_acn_t *llc_acn(uint8_t addr)
{
	struct llc_acn_t *acn;
	uint8_t i, free = 0xff;
	for (i = ACN_CACHE_SIZE; i != 0;) {
		acn = acnCache + --i;
		// Check free entry
		if (free == 0xff && acn->addr == MAC_BROADCAST)
			free = i;
		if (acn->addr == addr)
			return acn;
	}

	// No record
	// If free entry recorded
	if (free != 0xff)
		acn = acnCache + free;
	else {
		acn = acnCache + lastACN;
		lastACN = (lastACN - 1) & (ACN_CACHE_SIZE - 1);
	}
	llc_acn_reset(acn);
	acn->addr = addr;
	return acn;
}

static uint8_t llc_tx_frame(union ctrl_t ctrl, uint8_t seq, uint8_t addr, uint8_t len, uint8_t *data)
{
	// Generate LLC frame
	static struct llc_frame_t *frame;
	for (;;) {
		frame = pvPortMalloc(sizeof(struct llc_frame_t) + len);
		if (frame != 0)
			break;
		vTaskDelay(RETRY_TIME);
	}
	frame->ctrl = ctrl;
	frame->seq = seq;
	frame->len = len;
	memcpy(frame->payload, data, len);

	// Send to MAC layer
	xQueueReset(llc_ack);
	len += FRAME_APPEND_SIZE;
	mac_tx(addr, frame, len);

	if (ctrl.s.ackreq) {
		uint8_t count = 0;
		do {
			// Waiting for acknowledge
			struct llc_ack_t ack;
			while (!mac_written())
				vTaskDelay(1);
			if (xQueueReceive(llc_ack, &ack, RETRY_TIME) != pdTRUE) {
				// No ACK received, retry
				mac_tx(addr, frame, len);
				count++;
#if LLC_DEBUG > 0
				printf_P(PSTR(ESC_MAGENTA "LLC-RETRY,%u;"), count);
#endif
				continue;
			}
			if (ack.addr != addr || ack.seq != seq || ack.ctrl.s.acn != ctrl.s.acn)
				continue;
			vPortFree(frame);
			return 1;
		} while (count != RETRY_COUNT);
		// No ACK received
		vPortFree(frame);
		return 0;
	}
	vPortFree(frame);
	return 1;
}

uint8_t llc_tx(uint8_t pri, uint8_t addr, uint8_t len, void *ptr)
{
	uint8_t seq = 0;
	union ctrl_t ctrl;
	ctrl.u8 = 0;

#ifdef LLC_MUTEX
	// Thread safe
	while (xSemaphoreTake(llc_semaphore, portMAX_DELAY) != pdTRUE);
#endif

	struct llc_acn_t *acn = 0;
	if (addr != MAC_BROADCAST) {
		if (pri == DL_DATA_ACK) {
#if LLC_DEBUG > 1
			printf_P(PSTR(ESC_BLUE "LLC-PKT,%02x-%u,%u;"), addr, len, acn->s);
#endif
			ctrl.s.ackreq = 1;
			acn = llc_acn(addr);
			ctrl.s.acn = acn->s;
			acn->s = !acn->s;
		}
	}

#if LLC_DEBUG > 1
	if (!ctrl.s.ackreq)
		printf_P(PSTR(ESC_BLUE "LLC-UN-PKT,%02x-%u;"), addr, len);
#endif

	// Split packet into multiple frames
	while (len > FRAME_DATA_MAX_SIZE) {
		// Write to buffer
		if (!llc_tx_frame(ctrl, seq++, addr, FRAME_DATA_MAX_SIZE, ptr))
			goto failed;
		ptr += FRAME_DATA_MAX_SIZE;
		len -= FRAME_DATA_MAX_SIZE;
	}
	// Write the last frame to buffer
	ctrl.s.last = 1;
	if (llc_tx_frame(ctrl, seq, addr, len, ptr)) {
#ifdef LLC_MUTEX
		xSemaphoreGive(llc_semaphore);
#endif
		return 1;
	}

failed:	// ackreq must be 1 to be here
	// Reset ACn entry
	//llc_acn_reset(acn);
#ifdef LLC_MUTEX
	xSemaphoreGive(llc_semaphore);
#endif
	return 0;
}

static void llc_rx_task(void *param)
{
	static struct mac_frame data;

loop:
	// Receive 1 frame from MAC queue
	while (xQueueReceive(mac_rx, &data, portMAX_DELAY) != pdTRUE);

	// Frame size check
	if (data.len < FRAME_MIN_SIZE || data.len > FRAME_MAX_SIZE)
		goto drop;

	// Payload size check
	struct llc_frame_t *frame = data.payload;
	if (frame->len > FRAME_DATA_MAX_SIZE)
		goto drop;

	// This is an ACK frame
	if (frame->ctrl.s.ack) {
		struct llc_ack_t ack;
		if (!uxQueueSpacesAvailable(llc_ack)) {
			xQueueReceive(llc_ack, &ack, 0);
#if LLC_DEBUG > 0
			fputs_P(PSTR(ESC_GREY "LLC-RX-ACK-FAIL;"), stdout);
#endif
		}
		ack.ctrl = frame->ctrl;
		ack.seq = frame->seq;
		ack.addr = data.addr;
		xQueueSendToBack(llc_ack, &ack, 0);
#if LLC_DEBUG > 0
		fputs_P(PSTR(ESC_YELLOW "LLC-RX-ACK;"), stdout);
#endif
		goto drop;
	}

	// If this is a broadcast, or doesn't need ACK
	if (data.addr == MAC_BROADCAST || frame->ctrl.s.ackreq == 0) {
		// If the packet only have a single frame
		if (frame->seq == 0 && frame->ctrl.s.last) {
			struct llc_packet_t pkt;
			pkt.pri = DL_UNITDATA;
			pkt.addr = data.addr;
			pkt.len = frame->len;
			pkt.ptr = data.ptr;
			pkt.payload = frame->payload;

			// Send to upper layer
			if (xQueueSendToBack(llc_rx, &pkt, 0) == pdTRUE)
				goto loop;
		}
		goto drop;
	}

	// The packet need ACK
	struct llc_acn_t *acn = llc_acn(data.addr);

#if LLC_DEBUG > 1
	printf_P(PSTR(ESC_YELLOW "LLC-FRAME,%u/%u,%u/%u;"), frame->ctrl.s.acn, acn->r, frame->seq, acn->seq);
#endif

	// If this is a new packet
	if (frame->ctrl.s.acn != acn->r) {
		// If this frame is out of sequence
		if (frame->seq != 0) {
#if LLC_DEBUG > 0
			printf_P(PSTR(ESC_MAGENTA "LLC-NEW-OUT,%u,%u;"), frame->seq, acn->seq);
#endif
			goto drop;
		}

		// If this packet is carried by only a single frame
		if (frame->ctrl.s.last) {
			acn->seq = 0xff;	// Mark as done
			// If this frame is not empty
			if (frame->len != 0) {
				// Send ACK
				struct llc_frame_t f;
				f.ctrl.u8 = frame->ctrl.u8;
				f.ctrl.s.ack = 1;
				f.seq = 0;
				f.len = 0;
				mac_tx(data.addr, &f, FRAME_APPEND_SIZE);

				struct llc_packet_t pkt;
				pkt.pri = DL_DATA_ACK;
				pkt.addr = data.addr;
				pkt.len = frame->len;
				pkt.ptr = data.ptr;
				pkt.payload = frame->payload;

				// Unable to handle it at the moment
				if (xQueueSendToBack(llc_rx, &pkt, 0) != pdTRUE) {
#if LLC_DEBUG > 0
					fputs_P(PSTR(ESC_GREY "LLC-Q-SNG-FAIL;"), stdout);
#endif
					goto drop;
				}

				goto loop;
			}
		} else {
			if (acn->buffer == NULL && (acn->buffer = pvPortMalloc(PACKET_MAX_SIZE)) == NULL) {
#if LLC_DEBUG >= 0
				fputs_P(PSTR(ESC_GREY "LLC-MEM-FAIL;"), stdout);
#endif
				goto drop;
			}
#if LLC_DEBUG > 1
			fputs_P(PSTR(ESC_YELLOW "LLC-SEQ-STA;"), stdout);
#endif
			memcpy(acn->buffer, frame->payload, frame->len);
			acn->size = frame->len;
			acn->seq = 0;
		}

		acn->r = frame->ctrl.s.acn;

	// If this is a continuous frame
	} else {
		// If this frame is not duplicated
		if (frame->seq != 0 && frame->seq != acn->seq) {
			// If this frame is out of sequence
			if (frame->seq != acn->seq + 1) {
#if LLC_DEBUG > 0
				//fputs_P(PSTR("\e[93mLLC-SEQ-OUT;"), stdout);
				printf_P(PSTR(ESC_MAGENTA "LLC-SEQ-OUT,%u,%u;"), frame->seq, acn->seq);
#endif
				// Ignore this packet
				acn->r = 0xff;
				goto drop;
			}

			// If data buffer is insufficient
			if (acn->size + frame->len > PACKET_MAX_SIZE) {
#if LLC_DEBUG > 0
				fputs_P(PSTR(ESC_GREY "LLC-SEQ-MEM-FAIL;"), stdout);
#endif
				goto drop;
			}

#if LLC_DEBUG > 1
			fputs_P(PSTR(ESC_YELLOW "LLC-SEQ-ACC;"), stdout);
#endif
			if (acn->buffer != NULL) {
				memcpy(acn->buffer + acn->size, frame->payload, frame->len);
				acn->size += frame->len;
			}

			// If this is the last frame
			if (frame->ctrl.s.last) {
				// If this packet is not empty
				if (acn->size != 0) {
					// Send to upper layer
					struct llc_packet_t pkt;
					pkt.pri = DL_DATA_ACK;
					pkt.addr = data.addr;
					pkt.len = acn->size;
					pkt.ptr = acn->buffer;
					pkt.payload = acn->buffer;

					acn->buffer = 0;
					acn->size = 0;

					// Unable to handle it at the moment
					if (xQueueSendToBack(llc_rx, &pkt, 0) != pdTRUE) {
						vPortFree(pkt.ptr);
#if LLC_DEBUG > 0
						fputs_P(PSTR(ESC_GREY "LLC-Q-MLT-FAIL;"), stdout);
#endif
						goto drop;
					}
				}
			}
			acn->seq++;
		}
	}

	// Send ACK
	// Change to ACK frame
	frame->ctrl.s.ack = 1;
	frame->len = 0;

	// Send to MAC layer
	mac_tx(data.addr, frame, FRAME_APPEND_SIZE);
	vPortFree(data.ptr);

#if LLC_DEBUG > 1
	fputs_P(PSTR(ESC_YELLOW "LLC-TX-ACK;"), stdout);
#endif
	goto loop;

drop:
	if (data.ptr)
		vPortFree(data.ptr);
	goto loop;
}

uint8_t llc_written()
{
	return mac_written();
}

void llc_init()
{
	uint8_t i;
	for (i = ACN_CACHE_SIZE; i != 0;)
		acnCache[--i].addr = MAC_BROADCAST;
	lastACN = ACN_CACHE_SIZE - 1;

	while ((llc_rx = xQueueCreate(1, sizeof(struct llc_packet_t))) == 0);
	while ((llc_ack = xQueueCreate(1, sizeof(struct llc_ack_t))) == 0); //create queue for ack
#ifdef LLC_MUTEX
	while ((llc_semaphore = xSemaphoreCreateMutex()) == 0);
#endif
#if LLC_DEBUG > 0
	while (xTaskCreate(llc_rx_task, "LLC RX", 160, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#else
	while (xTaskCreate(llc_rx_task, "LLC RX", 100, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
#endif
}
