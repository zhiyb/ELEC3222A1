/**** RFM 12 library for Atmel AVR Microcontrollers *******
 * 
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * @author Peter Fuhrmann, Hans-Gert Dahmen, Soeren Heisrath
 */


/** \file rfm12.c
 * \brief rfm12 library main file
 * \author Hans-Gert Dahmen
 * \author Peter Fuhrmann
 * \author Soeren Heisrath
 * \version 1.2
 * \date 2012/01/22
 *
 * All core functionality is implemented within this file.
 */


/************************
 * standard includes
*/
#ifdef __PLATFORM_AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#endif

#include <string.h>

#include <phy_layer.h>
#include <dll_layer.h>

#include "rfm12_config.h"

/************************
 * library internal includes
 * the order in which they are included is important
*/
#include "include/rfm12_hw.h"
#include "include/rfm12_core.h"
#include "rfm12.h"

/************************
 * library internal globals
*/

//! Buffer and status for packet transmission.
struct rfm12_ctrl_t ctrl;
#if 0
struct rfm12_tx_buffer_t tx;
struct rfm12_rx_buffer_t rx;
#endif

/************************
 * load other core and external components
 * (putting them directly into here allows GCC to optimize better)
*/

/* include spi functions into here */
#include "include/rfm12_spi.c"
#include "include/rfm12_spi_linux.c"

/*
 * include control / init functions into here
 * all of the stuff in there is optional, so there's no code-bloat.
*/
//#define RFM12_LIVECTRL_HOST 1//if we are buliding for the microcontroller, we are the host.
#include "include/rfm12_livectrl.c"

/*
 * include extra features here
 * all of the stuff in there is optional, so there's no code-bloat..
*/
#include "include/rfm12_extra.c"

/************************
 * Begin of library
*/

uint8_t phy_mode()
{
	return ctrl.mode;
}

void phy_transmit()
{
	if (ctrl.mode == PHYTX)
		return;
#if 0
	uint8_t data;
	if (!dll_data_request(&data))
		return;
#endif
	// Initialise transmission
	RFM12_INT_OFF();
	ctrl.mode = PHYTX;
	rfm12_data(RFM12_CMD_PWRMGT | PWRMGT_DEFAULT ); /* disable receiver */
#if 1
	rfm12_data(RFM12_CMD_TX | 0x2d);
	rfm12_data(RFM12_CMD_TX | 0xa4);	// The sync bytes
#else
	rfm12_data(RFM12_CMD_TX | data);
	if (dll_data_request(&data))
		rfm12_data(RFM12_CMD_TX | data);
#endif
	rfm12_data(RFM12_CMD_PWRMGT | PWRMGT_DEFAULT | RFM12_PWRMGT_ET);
	RFM12_INT_FLAG = (1<<RFM12_FLAG_BIT);
	RFM12_INT_ON();
}

void phy_receive()
{
	if (ctrl.mode == PHYTX) {
		//turn off the transmitter and enable receiver
		//the receiver is not enabled in transmit only mode (by PWRMGT_RECEIVE macro)
		rfm12_data( PWRMGT_RECEIVE );
		//load a dummy byte to clear int status
		rfm12_data( RFM12_CMD_TX | 0x00);
		ctrl.mode = PHYRX;
	}
	// Reset the receiver fifo
	rfm12_data( RFM12_CMD_FIFORESET | CLEAR_FIFO_INLINE);
	rfm12_data( RFM12_CMD_FIFORESET | ACCEPT_DATA_INLINE);

}

uint8_t phy_free()
{
	if (ctrl.mode != PHYRX)
		return 0;
	//disable the interrupt (as we're working directly with the transceiver now)
	//hint: we could be losing an interrupt here, because we read the status register.
	//this applys for the Wakeup timer, as it's flag is reset by reading.
	RFM12_INT_OFF();
	uint16_t status = rfm12_read(RFM12_CMD_STATUS);
	RFM12_INT_ON();
	return !(status & RFM12_STATUS_RSSI);
}

#if 0
static inline uint8_t tx_end()
{
	return tx.read == tx.write;
}

static inline uint8_t tx_pop()
{
	uint8_t read = tx.read;
	uint8_t data = *(tx.buffer + read);
	tx.read = (read + 1) & RFM12_TX_BUFFER_MASK;
#if 0
	char c = (char)data;
	if (isprint(c))
		putchar(c);
#endif
	return data;
}

static inline void rx_push(const uint8_t data)
{
	uint8_t write = rx.write, read = rx.read;
	*(rx.buffer + write) = data;
	rx.write = write = (write + 1) & RFM12_RX_BUFFER_MASK;
	if (write == read)
		rx.read = (read + 1) & RFM12_RX_BUFFER_MASK;
}

void rfm12_send(uint8_t length, char *buffer)
{
	if (length == 0)
		return;
	uint8_t write = tx.write, remain;
	do
		remain = (tx.read - write) & RFM12_TX_BUFFER_MASK;
	while (length >= remain && remain != 0);
	if (write > RFM12_TX_BUFFER_SIZE - length) {
		// Remain bytes to end of buffer
		uint8_t remain = RFM12_TX_BUFFER_SIZE - write;
		memcpy(tx.buffer + write, buffer, remain);
		memcpy(tx.buffer, buffer + remain, length - remain);
		//write = length - len;
	} else {
		memcpy(tx.buffer + write, buffer, length);
		//write += length;
	}
	tx.write = (write + length) & RFM12_TX_BUFFER_MASK;
	if (ctrl.mode == TX)
		return;
	// Initialise transmission
	RFM12_INT_OFF();
	ctrl.mode = TX;
	rfm12_data(RFM12_CMD_PWRMGT | PWRMGT_DEFAULT ); /* disable receiver */
	rfm12_data(RFM12_CMD_TX | tx_pop());
	rfm12_data(RFM12_CMD_PWRMGT | PWRMGT_DEFAULT | RFM12_PWRMGT_ET);
	RFM12_INT_FLAG = (1<<RFM12_FLAG_BIT);
	RFM12_INT_ON();
}

uint8_t rfm12_recv(uint8_t length, char *buffer)
{
	uint8_t len = rfm12_available();
	if (len == 0)
		return 0;
	uint8_t read = rx.read;
	length = length < len ? length : len;
	if (read > RFM12_RX_BUFFER_SIZE - length) {
		// Remain bytes to end of buffer
		uint8_t remain = RFM12_RX_BUFFER_SIZE - read;
		memcpy(buffer, rx.buffer + read, remain);
		memcpy(buffer + remain, rx.buffer, length - remain);
	} else {
		memcpy(buffer, rx.buffer + read, length);
	}
	rx.read = (read + length) & RFM12_RX_BUFFER_MASK;
	return len;
}
#endif

//! Interrupt handler to handle all transmit and receive data transfers to the rfm12.
/** The receiver will generate an interrupt request (IT) for the
* microcontroller - by pulling the nIRQ pin low - on the following events:
* - The TX register is ready to receive the next byte (RGIT)
* - The FIFO has received the preprogrammed amount of bits (FFIT)
* - Power-on reset (POR) 
* - FIFO overflow (FFOV) / TX register underrun (RGUR) 
* - Wake-up timer timeout (WKUP) 
* - Negative pulse on the interrupt input pin nINT (EXT) 
* - Supply voltage below the preprogrammed value is detected (LBD) 
*
* The rfm12 status register is read to determine which event has occured.
* Reading the status register will clear the event flags.
*
* The interrupt handles the RGIT and FFIT events by default.
* Upon specific configuration of the library the WKUP and LBD events
* are handled additionally.
*
* \see rfm12_control_t, rf_rx_buffer_t and rf_tx_buffer_t
*/

#include <stdio.h>
//if polling is used, do not define an interrupt handler, but a polling function
#if (RFM12_USE_POLLING)
void rfm12_poll(void)
#else
//ISR(RFM12_INT_VECT, ISR_NOBLOCK)
ISR(RFM12_INT_VECT)
#endif
{
	RFM12_INT_OFF();
	uint8_t status;
	uint8_t recheck_interrupt;
	//uint8_t debug = 0;

	do {
#ifdef __PLATFORM_AVR__
		//clear AVR int flag
		RFM12_INT_FLAG = (1<<RFM12_FLAG_BIT);
#endif

		//first we read the first byte of the status register
		//to get the interrupt flags
		status = rfm12_read_int_flags_inline();
#if 0
		if (debug == 0) {
			printf("<%02x>", status);
			debug++;
		}
#endif
#if 0
		printf("<%02x>", status);
#endif

		//if we use at least one of the status bits, we need to check the status again
		//for the case in which another interrupt condition occured while we were handeling
		//the first one.
		recheck_interrupt = 0;

#if 0
		//low battery detector feature
		#if RFM12_LOW_BATT_DETECTOR
			if (status & (RFM12_STATUS_LBD >> 8)) {
				//debug
				#if RFM12_UART_DEBUG >= 2
					uart_putc('L');
				#endif

				//set status variable to low battery
				ctrl.low_batt = RFM12_BATT_LOW;
				recheck_interrupt = 1;
			}
		#endif /* RFM12_LOW_BATT_DETECTOR */

		//wakeup timer feature
		#if RFM12_USE_WAKEUP_TIMER
			if (status & (RFM12_STATUS_WKUP >> 8)) {
				//debug
				#if RFM12_UART_DEBUG >= 2
					uart_putc('W');
				#endif

				ctrl.wkup_flag = 1;
				recheck_interrupt = 1;
			}
			if (status & ((RFM12_STATUS_WKUP | RFM12_STATUS_FFIT) >> 8) ) {
				//restart the wakeup timer by toggling the bit on and off
				rfm12_data(ctrl.pwrmgt_shadow & ~RFM12_PWRMGT_EW);
				rfm12_data(ctrl.pwrmgt_shadow);
			}
		#endif /* RFM12_USE_WAKEUP_TIMER */
#endif

		//check if the fifo interrupt occurred
		if (status & (RFM12_STATUS_FFIT >> 8)) {
			//yes
			recheck_interrupt = 1;
			//see what we have to do (start rx, rx or tx)
			switch (ctrl.mode) {
			case PHYRX:	// Receiving
#if 1
				dll_data_handler(rfm12_read(RFM12_CMD_READ));
#else
#if 1
				{
					char c = rfm12_read(RFM12_CMD_READ);
					if (isprint(c))
						putchar(c);
					rx_push(c);
				}
#else
				rx_push(rfm12_read(RFM12_CMD_READ));
#endif
#endif
				continue;
			default:	// Transmitting
#if 1
				{
					uint8_t data;
					if (dll_data_request(&data)) {
						rfm12_data(RFM12_CMD_TX | data);
#if 0
						if (status & (RFM12_STATUS_RGUR >> 8)) {
							if (!dll_data_request(&data))
								continue;
							rfm12_data(RFM12_CMD_TX | data);
						}
#endif
						continue;
					}
				}
#else
				if (!tx_end()) {
					rfm12_data(RFM12_CMD_TX | tx_pop());
					if (status & (RFM12_STATUS_RGUR >> 8) && !tx_end())
						rfm12_data(RFM12_CMD_TX | tx_pop());
					continue;
				}
#endif
			}
			phy_receive();
		}
	} while (recheck_interrupt);

	//RFM12_INT_FLAG = (1<<RFM12_FLAG_BIT);
	//turn the int back on
	RFM12_INT_ON();
}

//enable internal data register and fifo
//setup selected band
#define RFM12_CMD_CFG_DEFAULT   (RFM12_CMD_CFG | RFM12_CFG_EL | RFM12_CFG_EF | RFM12_BASEBAND | RFM12_XTAL_LOAD)

//set rx parameters: int-in/vdi-out pin is vdi-out,
//Bandwith, LNA, RSSI
#define RFM12_CMD_RXCTRL_DEFAULT (RFM12_CMD_RXCTRL | RFM12_RXCTRL_P16_VDI | RFM12_RXCTRL_VDI_FAST | RFM12_FILTER_BW | RFM12_LNA_GAIN | RFM12_RSSI_THRESHOLD )

//set AFC to automatic, (+4 or -3)*2.5kHz Limit, fine mode, active and enabled
#define RFM12_CMD_AFC_DEFAULT  (RFM12_CMD_AFC | RFM12_AFC_AUTO_KEEP | RFM12_AFC_LIMIT_4 | RFM12_AFC_FI | RFM12_AFC_OE | RFM12_AFC_EN)

//set TX Power, frequency shift
#define RFM12_CMD_TXCONF_DEFAULT  (RFM12_CMD_TXCONF | RFM12_POWER | RFM12_TXCONF_FS_CALC(FSK_SHIFT) )

#ifdef __PLATFORM_AVR__
static const uint16_t init_cmds[] PROGMEM = {
#else
static const uint16_t init_cmds[] = {
#endif
	//defined above (so shadow register is inited with same value)
	RFM12_CMD_CFG_DEFAULT,

	//set power default state (usually disable clock output)
	//do not write the power register two times in a short time
	//as it seems to need some recovery
	(RFM12_CMD_PWRMGT | PWRMGT_DEFAULT),

	//set frequency
	(RFM12_CMD_FREQUENCY | RFM12_FREQUENCY_CALC(RFM12_FREQUENCY) ),

	//set data rate
	(RFM12_CMD_DATARATE | DATARATE_VALUE ),

	//defined above (so shadow register is inited with same value)
	RFM12_CMD_RXCTRL_DEFAULT,

	//automatic clock lock control(AL), digital Filter(!S),
	//Data quality detector value 3, fast clock recovery lock
	(RFM12_CMD_DATAFILTER | RFM12_DATAFILTER_AL | RFM12_DATAFILTER_ML | 3),

	//2 Byte Sync Pattern, Pattern fill fifo,
	//disable sensitive reset, Fifo filled interrupt at 8 bytes
	(RFM12_CMD_FIFORESET | CLEAR_FIFO_INLINE),

	//defined above (so shadow register is inited with same value)
	RFM12_CMD_AFC_DEFAULT,

	//defined above (so shadow register is inited with same value)
	RFM12_CMD_TXCONF_DEFAULT,

	//disable low dutycycle mode
	(RFM12_CMD_DUTYCYCLE),

	//disable wakeup timer
	(RFM12_CMD_WAKEUP),

	//enable rf receiver chain, if receiving is not disabled (default)
	//the magic is done via defines
	(RFM12_CMD_PWRMGT | PWRMGT_RECEIVE),
};

//! This is the main library initialization function
/**This function takes care of all module initialization, including:
* - Setup of the used frequency band and external capacitor
* - Setting the exact frequency (channel)
* - Setting the transmission data rate
* - Configuring various module related rx parameters, including the amplification
* - Enabling the digital data filter
* - Enabling the use of the modules fifo, as well as enabling sync pattern detection
* - Configuring the automatic frequency correction
* - Setting the transmit power
*
* This initialization function also sets up various library internal configuration structs and
* puts the module into receive mode before returning.
*
* \note Please note that the transmit power and receive amplification values are currently hard coded.
* Have a look into rfm12_hw.h for possible settings.
*/
void rfm12_init(void) {
	//initialize spi
#ifdef __PLATFORM_AVR__
	SS_RELEASE();
	DDR_SS |= (1<<BIT_SS);
#endif
	spi_init();
#if 1
#ifdef __PLATFORM_AVR__
	_delay_ms(100);
	// Enable power sensitive reset
	rfm12_data(RFM12_CMD_FIFORESET | (8 << 4));
	// Issuing FE00h command will trigger software reset
	rfm12_data(0xfe00);
	_delay_ms(100);
#endif
#endif

	// Initialise control structure
	ctrl.mode = PHYRX;
#if 0
	rx.write = 0;
	rx.read = 0;
	tx.write = 0;
	tx.read = 0;
#endif

	//typically sets DDR registers for RFM12BP TX/RX pin
	#ifdef TX_INIT_HOOK
		TX_INIT_HOOK;
	#endif

	#ifdef RX_INIT_HOOK
		RX_INIT_HOOK;
	#endif

	//write all the initialisation values to rfm12
	uint8_t x;

	#ifdef __PLATFORM_AVR__
		for (x = 0; x < ( sizeof(init_cmds) / 2) ; x++)
			rfm12_data(pgm_read_word(&init_cmds[x]));
	#else
		for (x = 0; x < ( sizeof(init_cmds) / 2) ; x++)
			rfm12_data(init_cmds[x]);
	#endif

#if 1
	//Sync pattern
	rfm12_data(RFM12_CMD_SYNCPATTERN | 0xa4);
#endif

	#ifdef RX_ENTER_HOOK
		RX_ENTER_HOOK;
	#endif

	#if RFM12_USE_CLOCK_OUTPUT || RFM12_LOW_BATT_DETECTOR
		rfm12_data(RFM12_CMD_LBDMCD | RFM12_LBD_VOLTAGE | RFM12_CLOCK_OUT_FREQUENCY ); //set low battery detect, clock output
	#endif

	//ASK receive mode feature initialization
	#if RFM12_RECEIVE_ASK
		adc_init();
	#endif

	//setup interrupt for falling edge trigger
#ifdef __PLATFORM_AVR__
	RFM12_INT_SETUP();
#endif

	//clear int flag
	rfm12_read(RFM12_CMD_STATUS);

#ifdef __PLATFORM_AVR__
	RFM12_INT_FLAG = (1<<RFM12_FLAG_BIT);
#endif

	//init receiver fifo, we now begin receiving.
	rfm12_data(CLEAR_FIFO);
	rfm12_data(ACCEPT_DATA);

	//activate the interrupt
	RFM12_INT_ON();
}
