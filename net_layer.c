#include "net_layer.h"
#include "dll_lyaer.h"
#include <studio.h>

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
void Find_MAC(IP){
	uint8_t MAC[255];
	int i 
	for(i=0; i<255; i++)
	{
		MAC[i] = 0xFF; //initialise the array with invalid MAC address 0xFF: broadcast address
	}
		return MAC[IP];
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
