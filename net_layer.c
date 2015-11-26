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
#define REQ 1
#define ACK 0
#define DATA 2

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

uint8_t IP;

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
}

void net_tx(const uint8_t *packet, uint8_t address) //requrie whole package and destination address
{
	MAC_cache_init(); //for testing just generate a new cache every time before transmission
	struct net_buffer *tx;
	tx.TRAN_Segement = &packet; //load the package into tx TRAN_Segement buffer
	if (MAC[address] == 0xFF){ //checking availability in mac cache
		tx.control = REQ; //if not available, mark the package as a request package 
		tx.SRC_address = ; //need to know the address 
		tx.DEST_Address = ;
		tx.Length = ;
		llc_tx(pri = DL_UNITDATA, addr = MAC_BROADCAST , len = sizeof(package) , *ptr = tx);//broadcast request

	}
	else // if available send package normally
		{
		tx.control = DATA;
		tx.SRC_address = ; // need to know the address 
		tx.DEST_Address = ;
		llc_tx(pri = DL_UNITDATA, addr = address , len = sizeof(package) , *ptr = tx);
		}

	tx.contol 
	llc_tx(uint8_t pri, uint8_t addr, uint8_t len, void *ptr)
	
}

void net_rx_task(void *param){
	static struct llc_packet_t pkt;
	
loop:
	while (xQueueReceive(llc_rx, &pkt, 0) != pdTRUE);
	uint8_t *ptr = pkt.payload; //received NET package
	uint8_t len = pkt.len; //length of payload
	uint8_t addr = pkt.addr; //source mac address
	uint8_t pri = pkt.pri; // request type
	void *null; 
	uint8_t sender_IP = ptr.SRC_address;
	struct net_buffer *reply;
	reply.SRC_address = /*function to generate mac address in mac.c*/
	if(pri == REQ){ //if it is a request, send the ACK back to sender
		llc_tx(ACK, addr = sender_IP, len = 0, ptr = reply);
		MAC_uptate(ptr.SRC_address, addr);
	}
	else if(pri == ACK){
		;
	}
}

uint8_t net_rx(uint8_t *packet)
{
	static struct llc_packet_t package;
	while (xQueueReceive(llc_rx, &package, 0) != pdTRUE); //load receive sructure to pointer package
	uint8_t *ptr = package //copy package into ptr
	if (ptr.addr == **own IP**) {
		if(ptr.control == REQ){
			put mac address on the buffer 
		}
		else if (ptr.control == ACK){
			
		}
	}
		
		else if(ptr.control == ACK){
			//put ACK to the 
		

	}
}


 // when broadcasting ARP, need to add a timeout sevice, retry on timeout 
void Find_MAC(IP, package){ //used for finding MAC address from the give IP address
	if (MAC[IP] == 0xFF)
	{
		 //go and broadcast the request. pri = 1 for ack, = 0 for data 
		if(llc_tx(pri = DL_UNITDATA, addr = MAC_BROADCAST , len = sizeof(package) , *ptr = package)) // if tx success, begin loading   
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

void MAC_uptate(uint8_t IP, uint8_t mac_address){
	MAC[IP] = mac_address;
}

//this task will run the function continuously 
