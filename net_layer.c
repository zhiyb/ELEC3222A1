#include "net_layer.h"
#include "dll_lyaer.h"
#include <studio.h>
#include <llc_layer.h>

//--------function call always from top to bottom.--------//
//														  //
//				 TRAN call NET call DLL 				  //
//--------------------------------------------------------//

static union net_buffer{
	struct{
			data[126];
	}d;
	struct {
		unit16_t control;
		unit8_t SRC_address;
		unit8_t DEST_Address;
		unit8_t Length;
		unit8_t TRAN_Segement[0];
	} s;
}Txbuffert;

static union net_buffer{
	struct{
			data[126];
	}d;
	struct {
		unit16_t control;
		unit8_t SRC_address;
		unit8_t DEST_Address;
		unit8_t Length;
		unit8_t TRAN_Segement[0];
	} s;
}Rxbuffert;

#define Checksum(st) ((unit16_t * )((st).s.data + (st).s.length))

uint8_t IP;

MAC_cache_init(){
	uint8_t MAC[255];
	int i 
	for(i=0; i<255; i++)
	{
		MAC[i] = 0xFF; //initialise the array with invalid MAC address 0xFF: broadcast address
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


MAC_cache_update(){
	
}

void net_write(const uint8_t *packet, uint8_t length)
{
	Txbuffert.s.control
	packet = control + SRC_address + DEST_Address + Length + packet + Checksum;

	dll_write(*packet, legth);
}













uint8_t net_read(uint8_t *packet)
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
