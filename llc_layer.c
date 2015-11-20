#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "net_layer.h"
#include "llc_layer.h"
#include "mac_layer.h"

#include <task.h>

#define PACKET_MAX_SIZE		NET_PACKET_MAX_SIZE
#define FRAME_DATA_MAX_SIZE	23
#define APPEND_BYTES		5

QueueHandle_t llc_tx, llc_rx;
// ACK queue, item size 1 byte
static QueueHandle_t llc_ack;

union ctrl_t {
	uint8_t u;
	struct {
		uint8_t last:1;		// Is last frame?
		uint8_t ack:1;		// Is an ACK frame?
		uint8_t ackreq:1;	// Is ACK required?
		uint8_t pktid:5;	// Packet ID (for ACK)
	} s;
};

struct llc_frame
{
	union ctrl_t ctrl;
	uint8_t seq;
	uint8_t len;
	uint8_t payload[LLC_FRAME_MAX_SIZE - APPEND_BYTES];
};

void llc_tx_task(void *param)
{
	static struct llc_packet pkt;
	puts_P(PSTR("\e[96mLLC TX task initialised."));

loop:
	// Receive 1 packet to transmit from LLC queue
	while (xQueueReceive(llc_tx, &pkt, portMAX_DELAY) != pdTRUE);

	union ctrl_t ctrl;

	switch (pkt.pri) {
	case DL_DATA_ACK_STATUS:	// ACK packet
		ctrl.s.ack = 1;
		break;
	case DL_DATA_ACK:	// Packet requiring ACK
		ctrl.s.ackreq = 1;
	case DL_UNITDATA:	// Packet not requiring ACK
		break;
	}

	pktid = NEXT_PKTID(pktid);

	goto loop;
}

void llc_rx_task(void *param)
{
	static struct mac_frame data;
	uint8_t seq = 0, *buffer = 0, *ptr = 0, size = 0;
	puts_P(PSTR("\e[96mLLC RX task initialised."));

loop:
	// Receive 1 frame from MAC queue
	while (xQueueReceive(mac_rx, &data, portMAX_DELAY) != pdTRUE);
	struct llc_frame *frame = data.ptr;

	// Data length check
	if (frame->len > FRAME_DATA_MAX_SIZE || frame->len + APPEND_BYTES != data.len)
		goto discard;

	// Address routing check
	if (frame->dest == MAC_BROADCAST) {
		// Broadcasting packet
		// Cannot ack broadcast frame
		if (frame->ctrl.s.ackreq)
			goto discard;
	} else {
		// Point-to-point packet
		if (frame->dest != mac_address())
			goto discard;
	}

	// Frame sequence check
	if (frame->seq == 0 || frame->seq != seq) {
		// Reset sequence index
		seq = 0;
		if (frame->seq != 0)
			goto discard;
		// New packet
		if (buffer == 0 && (buffer = pvPortMalloc(PACKET_MAX_SIZE)) == 0)
			goto discard;
		ptr = buffer;
		size = 0;
	}

	// Packet buffer space check
	if (size + frame->len > PACKET_MAX_SIZE) {
		seq = 0;
		goto discard;
	}

	// Frame accepted, copy content
	memcpy(ptr, frame->payload, frame->len);
	size += frame->len;

	// Check for last frame
	if (frame->ctrl.s.last) {
		seq = 0;
		if (frame->ctrl.s.ack) {
			// Received frame is an ACK frame
			uint8_t pktseq = *buffer;
			xQueueSendToBack(llc_ack, &pktseq, 0);
		} else {
			// Push received data to rx queue
			struct llc_packet pkt;
			pkt.pri = frame->ctrl.s.ackreq ? DL_DATA_ACK : DL_UNITDATA;
			pkt.addr = frame->src;
			pkt.len = size;
			pkt.ptr = buffer;
			// If no space available in rx queue
			if (xQueueSendToBack(llc_rx, &pkt, 0) != pdTRUE)
				goto discard;

			buffer = 0;
			if (frame->ctrl.s.ackreq) {
				// Push an ACK packet to tx queue
				pkt.pri = DL_DATA_ACK_STATUS;
				pkt.addr = frame->src;
				pkt.len = frame->ctrl.s.pktid;
				pkt.ptr = 0;
				xQueueSendToBack(llc_tx, &pkt, 0);
			}
		}
	}

discard:
	vPortFree(data.ptr);

	goto loop;
}

uint8_t llc_written()
{
	return mac_written() && uxQueueMessagesWaiting(llc_tx) == 0;
}

void llc_init()
{
	puts_P(PSTR("\e[96mLLC task setting up..."));
	llc_tx = xQueueCreate(1, sizeof(struct llc_packet));
	llc_rx = xQueueCreate(3, sizeof(struct llc_packet));
	llc_ack = xQueueCreate(4, 1);
	xTaskCreate(llc_tx_task, "LLC TX", 128, NULL, tskPROT_PRIORITY, NULL);
	xTaskCreate(llc_rx_task, "LLC RX", 128, NULL, tskPROT_PRIORITY, NULL);
}
