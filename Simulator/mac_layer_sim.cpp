#include "../mac_layer.h"
#include "../llc_layer.h"
#include "simulator.h"
#include "simulation.h"

extern "C" {

QueueHandle_t mac_rx;

void sim_start()
{
	mac_init();
	llc_init();
	taskStart();
}

void sim_end()
{
	taskStop();
}

void sim_rx_handle(uint8_t addr, char *ptr, int len)
{
	struct mac_frame frame;
	frame.addr = addr;
	frame.len = len;
	frame.ptr = malloc(len);
	frame.payload = frame.ptr;
	memcpy(frame.ptr, ptr, len);
	xQueueSendToBack(mac_rx, &frame, 0);
}

void mac_init()
{
	mac_rx = xQueueCreate(4, sizeof(struct mac_frame));
}

void mac_tx(uint8_t addr, void *data, uint8_t len)
{
	sim->transmit(addr, data, len);
}

uint8_t mac_written()
{
	return 1;
}

uint8_t mac_address()
{
	return addr.mac;
}

uint8_t mac_address_update(uint8_t addr)
{
	while (addr == MAC_BROADCAST)
		addr = rand() & 0xff;//generate a random new mac address
	::addr.mac = addr;
	return addr;
}

}
