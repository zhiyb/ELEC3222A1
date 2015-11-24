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
		void *TRAN_Segement;
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

void net_tx(const uint8_t *packet, uint8_t length)
{
	struct net_buffer *tx;
	tx.TRAN_Segement = &packet; //load the package into tx TRAN_Segement buffer
	
	tx.contol 
	llc_tx(uint8_t pri, uint8_t addr, uint8_t len, void *ptr)
	
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


while (xTaskCreate(llc_rx_task, "LLC RX", 160, NULL, tskPROT_PRIORITY, NULL) != pdPASS); //create a task for rx
//this task will run the function continuously 








uint8_t net_rx(uint8_t *packet)
{
	//define a another buffer here and pass it to dll_read(*packet)
	if (dll_read(*packet1))
		{
			*packet = packet1.[TRAN_Segement]
			return 1;
		}
	else return 0;
}

void net_init()
{
	;
}
