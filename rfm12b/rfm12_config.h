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

/******************************************************
 *                                                    *
 *           C O N F I G U R A T I O N                *
 *                                                    *
 ******************************************************/

/************************
 * RFM12 PIN DEFINITIONS
 */

//Pin that the RFM12's slave select is connected to
#define DDR_SS DDRB
#define PORT_SS PORTB
#define BIT_SS 4

//SPI port
#define DDR_SPI DDRB
#define PORT_SPI PORTB
#define PIN_SPI PINB
#define BIT_MOSI 5
#define BIT_MISO 6
#define BIT_SCK  7
#define BIT_SPI_SS 4
//this is the hardware SS pin of the AVR - it 
//needs to be set to output for the spi-interface to work 
//correctly, independently of the CS pin used for the RFM12

/************************
 * RFM12 CONFIGURATION OPTIONS
 */

//baseband of the module (either RFM12_BAND_433, RFM12_BAND_868 or RFM12_BAND_912)
#define RFM12_BASEBAND RFM12_BAND_433

//center frequency to use (+- FSK frequency shift)
#define RFM12_FREQUENCY       (433170000UL + FSK_SHIFT * 4)
//#define RFM12_FREQUENCY       (433170000UL)

//Transmit FSK frequency shift in kHz
#define FSK_SHIFT             125000UL

//Receive RSSI Threshold
#define RFM12_RSSI_THRESHOLD  RFM12_RXCTRL_RSSI_79

//Receive Filter Bandwidth
#define RFM12_FILTER_BW       RFM12_RXCTRL_BW_400

//Output power relative to maximum (0dB is maximum)
#define RFM12_POWER           RFM12_TXCONF_POWER_0

//Receive LNA gain
#define RFM12_LNA_GAIN        RFM12_RXCTRL_LNA_0

//crystal load capacitance - the frequency can be verified by measuring the
//clock output of RFM12 and comparing to 1MHz.
//11.5pF seems to be o.k. for RFM12, and 10.5pF for RFM12BP, but this may vary.
#define RFM12_XTAL_LOAD       RFM12_XTAL_11_5PF

//use this for datarates >= 2700 Baud
//#define DATARATE_VALUE        RFM12_DATARATE_CALC_HIGH(114900.0)
#define DATARATE_VALUE        RFM12_DATARATE_CALC_HIGH(58000.0)
// Data quality detector, minimum 4, maximum 7
#define DATAFILTER_DQD		6

//use this for 340 Baud < datarate < 2700 Baud
//#define DATARATE_VALUE      RFM12_DATARATE_CALC_LOW(1200.0)

//TX BUFFER SIZE
// For performance reason, use power-of-2
#define RFM12_TX_BUFFER_SIZE  256

//RX BUFFER SIZE (there are going to be 2 Buffers of this size for double_buffering)
// For performance reason, use power-of-2
#define RFM12_RX_BUFFER_SIZE  256


/************************
 * RFM12 INTERRUPT VECTOR
 * set the name for the interrupt vector here
 */

//the interrupt vector
#define RFM12_INT_VECT (INT2_vect)

//the interrupt mask register
#define RFM12_INT_MSK EIMSK

//the interrupt bit in the mask register
#define RFM12_INT_BIT (INT2)

//the interrupt flag register
#define RFM12_INT_FLAG EIFR

//the interrupt bit in the flag register
#define RFM12_FLAG_BIT (INTF2)

#if 0
//setup the interrupt to trigger on negative edge
#define RFM12_INT_SETUP()   EICRA |= (1 << ISC21)
#else
//setup the interrupt to trigger on low level
#define RFM12_INT_SETUP()   EICRA &= ~(_BV(ISC21) | _BV(ISC20))
#endif


/************************
 * FEATURE CONFIGURATION
 */

#define RFM12_LIVECTRL 1
#define RFM12_LIVECTRL_CLIENT 0
#define RFM12_LIVECTRL_HOST 1
#define RFM12_LIVECTRL_LOAD_SAVE_SETTINGS 0
#define RFM12_NORETURNS 0
#define RFM12_NOCOLLISIONDETECTION 0
#define RFM12_TRANSMIT_ONLY 0
#define RFM12_SPI_SOFTWARE 0
#define RFM12_USE_POLLING 0
#define RFM12_RECEIVE_ASK 0
#define RFM12_TRANSMIT_ASK 0
#define RFM12_USE_WAKEUP_TIMER 0
#define RFM12_USE_POWER_CONTROL 0
#define RFM12_LOW_POWER 0
#define RFM12_USE_CLOCK_OUTPUT 0
#define RFM12_LOW_BATT_DETECTOR 0


#define RFM12_LBD_VOLTAGE             RFM12_LBD_VOLTAGE_3V0


#define RFM12_CLOCK_OUT_FREQUENCY     RFM12_CLOCK_OUT_FREQUENCY_1_00_MHz

/* use a callback function that is called directly from the
 * interrupt routine whenever there is a data packet available. When
 * this value is set to 1, you must use the function
 * "rfm12_set_callback(your_function)" to point to your
 * callback function in order to receive packets.
 */
#define RFM12_USE_RX_CALLBACK 0

#define RFM12_UART_DEBUG 0
