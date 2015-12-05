#include "net_layer.h"
#include "llc_layer.h"
#include "trans_layer.h"//?? name correct ?
#include <studio.h>
#include <llc_layer.h>


#include <task.h>
#include <semphr.h>

//--------function call always from top to bottom.--------//
//														  //
//				 TRAN call NET call DLL 				  //
//--------------------------------------------------------//
#define ACK 0
#define REQ 1
#define DATA 2

QueueHandle_t net_ack;
/* union buffer format
static union net_buffer{
	struct{
			data[126];
	} d;
	struct {
		unit16_t control;
		unit8_t SRC_address;
		unit8_t DEST_Address;
		unit8_t Length;
		unit8_t TRAN_Segement[0];
		unit16_t Checksum;
	} s;
}Txbuffer;

static union net_buffer{
	struct{
			data[126];
	} d;
	struct {
		unit16_t control;
		unit8_t SRC_address;
		unit8_t DEST_Address;
		unit8_t Length;
		unit8_t TRAN_Segement[0];
		unit16_t Checksum;
	} s;
}Rxbuffer;
*/

struct net_buffer{
		unit16_t control;
		unit8_t SRC_address;
		unit8_t DEST_Address;
		unit8_t Length;
		void *TRAN_Segement[121];
		unit16_t Checksum;
};

//struct net_buffer *tx;
struct net_buffer *rx; //net_buffer type object tx rx

//#define Checksum(st) ((unit16_t * )((st).s.data + (st).s.length))

uint8_t net_addr;

uint8_t net_address_set(uint8_t addr){

	net_addr = addr;
	return net_addr;
}

void MAC_cache_uptate(uint8_t IP, uint8_t mac_address){
	MAC[IP] = mac_address;
}


void MAC_cache_init(){
	uint8_t MAC[255];
	int i 
	for(i=0; i<255; i++)
	{
		MAC[i] = 0xFF; //initialise the array with invalid MAC address 0xFF: broadcast address
	}
}

void net_init(){
	tx.control control = 0;
	tx.SRC_address = 0;
	tx.DEST_Address = 0;
	tx.Length = 0;
	rx.control control = 0;
	rx.SRC_address = 0;
	rx.DEST_Address = 0;
	rx.Length = 0;
	while (xTaskCreate(net_rx_task, "LLC RX", configMINIMAL_STACK_SIZE, NULL, tskPROT_PRIORITY, NULL) != pdPASS);
	while ((net_rx = xQueueCreate(2, sizeof(struct net_packet_t))) == NULL); //create a task for rx
	while ((net_ack = xQueueCreate(2, sizeof(struct net_packet_t))) == NULL);
}

void net_tx(const uint8_t *packet, uint8_t address){ //requrie whole package and destination address
	MAC_cache_init(); //for testing just generate a new cache every time before transmission
	struct net_buffer *tx;
	tx.TRAN_Segement = &packet; //load the package into tx TRAN_Segement buffer
	if (MAC[address] == 0xFF){ //checking availability in mac cache
		tx.control = REQ; //if not available, mark the package as a request package 
		tx.SRC_address = net_addr ; //need to know the address 
		tx.DEST_Address = address ;
		tx.Length = ; //length of whole package = 7 + sizeof(TRAM_Segment)
		llc_tx(DL_UNITDATA, MAC_BROADCAST, sizeof(tx), *ptr = tx);//broadcast request
		
		while (xQueueReceive(net_ack, &data, portMAX_DELAY) != pdTRUE);
		if(data.SRC_address == address){
			MAC_uptate(address, data.SRC_address)
		}

	}
	else // if available send package normally
		{
		tx.control = DATA;
		tx.SRC_address = ; // need to know the address 
		tx.DEST_Address = ;
		llc_tx(DL_UNITDATA, address , len = sizeof(tx), tx);
		}

	tx.contol 
	llc_tx(pri, addr, len, *ptr);
	
}



void net_rx_task(void *param){
	static struct llc_packet_t pkt;
	
loop:
	while (xQueueReceive(llc_rx, &pkt, 0) != pdTRUE);
	uint8_t *ptr = pkt.payload; //received NET package
	uint8_t len = pkt.len; //length of payload
	uint8_t mac_addr = pkt.addr; //source mac address
	//uint8_t pri = pkt.pri; // request type
	uint8_t sender_IP = ptr.SRC_address;
	struct net_buffer *reply;
	
	reply.SRC_address = sender_IP; // the one who send the request
	reply.DEST_Address = sender_IP; // whom to send the package to
	
	if(net_addr == ptr.DEST_Address) // decide to receive or drop
	{ // next we need to know if the ack is the one we are looking for 
		if(ptr.control == REQ){ //if it is a request, send the ACK back to sender
			//the reply package's SRC and DEST are all assigned as the senders IP addrs
			llc_tx(DL_DATA_ACK, mac_addr, sizeof(reply), reply);
			MAC_uptate(ptr.SRC_address, mac_addr); //update ARP cache 
		}
		else if(ptr.control == ACK){ //store the source mac to cache, judge whether the ack is for you
			//update anyway, this could be the mac address of a different machine
			MAC_uptate(ptr.SRC_address, mac_addr); 
			struct net_buffer pkt;
					pkt.control control = 0;
					pkt.SRC_address = mac_addr;
					pkt.DEST_Address = sender_IP;
					pkt.Length = 0;
					if (xQueueSendToBack(net_ack, &pkt, 0) != pdTRUE); 
					}

				else if(ptr.control == DATA){
						if (xQueueSendToBack(net_ack, &pkt, 0) != pdTRUE); 
						//continue transsmit 
					}
				
	else goto drop;		
		;
	}
}


 // when broadcasting ARP, need to add a timeout sevice, retry on timeout 
void Find_MAC(IP, package){ //used for finding MAC address from the give IP address
	if (MAC[IP] == 0xFF)
	{
		 //go and broadcast the request. pri = 1 for ack, = 0 for data 
		if(llc_tx(DL_UNITDATA, MAC_BROADCAST, sizeof(package), package)) // if tx success, begin loading   
		{
		 		static struct llc_packet_t package;
		 		while (xQueueReceive(llc_rx, &package, 0) != pdTRUE); //load receive sructure to pointer package
		 		uint8_t *ptr = package.addr; //ptr point to the start of the address bit.
		}
		return 0; //tx failed, need to retry at this point.
	}
	else	
		return MAC[IP];
}



//this task will run the function continuously 
