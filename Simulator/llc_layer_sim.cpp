#include "../mac_layer.h"
#include "../llc_layer.h"
#include "../net_layer.h"
#include "simulator.h"
#include "simulation.h"

#define PRI_ACK	0xa5

#define RETRY_COUNT	4
#define RETRY_TIME	100

extern "C" {

struct llc_ack_t {
	uint8_t addr;
};

QueueHandle_t llc_rx;
static QueueHandle_t llc_ack;

void sim_start()
{
	llc_init();
	net_init();
	taskStart();
}

void sim_end()
{
	taskStop();
}

void sim_rx_handle(uint8_t pri, uint8_t addr, QByteArray &data)
{
	if (pri == PRI_ACK) {
		llc_ack_t ack;
		ack.addr = addr;
		xQueueSendToBack(llc_ack, &ack, -1);
		return;
	} else if (pri == DL_DATA_ACK) {
		QByteArray data;
		sim->transmit(PRI_ACK, addr, data);
	}
	if (data.size() == 0)
		return;
	struct llc_packet_t packet;
	packet.pri = pri;
	packet.addr = addr;
	packet.len = data.size();
	packet.ptr = pvPortMalloc(packet.len);
	packet.payload = packet.ptr;
	memcpy(packet.payload, data.data(), packet.len);
	xQueueSendToBack(llc_rx, &packet, -1);
}

void llc_init()
{
	llc_rx = xQueueCreate(4, sizeof(struct llc_packet_t));
	llc_ack = xQueueCreate(4, sizeof(struct llc_ack_t));
}

uint8_t llc_tx(uint8_t pri, uint8_t addr, uint8_t len, void *ptr)
{
	QByteArray data;
	data.resize(len);
	memcpy(data.data(), ptr, len);
	xQueueReset(llc_ack);
	sim->transmit(pri, addr, data);
	if (pri == DL_DATA_ACK && addr != MAC_BROADCAST) {
		struct llc_ack_t ack;
		for (int i = 0; i < RETRY_COUNT; i++) {
			xQueueReceive(llc_ack, &ack, RETRY_TIME);
			if (ack.addr == addr)
				return 1;
		}
	}
	return 0;
}

uint8_t llc_written()
{
	return 1;
}

uint8_t net_address()
{
	return addr.net;
}

uint8_t net_address_update(uint8_t addr)
{
	::addr.net = addr;
	return addr;
}

}
