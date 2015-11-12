#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "dll_layer.h"
#include "phy_layer.h"

#define CSMA	(0.01)

#define FRAME_DATA_MAX_SIZE	23
#define FRAME_APPEND_SIZE	9
#define FRAME_MAX_SIZE		(FRAME_DATA_MAX_SIZE + FRAME_APPEND_SIZE)
// Omitted: header, footer
#define DLL_BUFFER_TX_APPEND	(FRAME_APPEND_SIZE - 2)
//#define DLL_BUFFER_TX_APPEND	FRAME_APPEND_SIZE
#define DLL_BUFFER_TX_SIZE	(FRAME_DATA_MAX_SIZE + DLL_BUFFER_TX_APPEND)
// Omitted: header, footer
#define DLL_BUFFER_RX_APPEND	(FRAME_APPEND_SIZE - 2)
#define DLL_BUFFER_RX_SIZE	(FRAME_DATA_MAX_SIZE + DLL_BUFFER_RX_APPEND)
// For performance, power-of-2 only
#define DLL_BUFFER_TX_NUM	8	// ~256 bytes, for 1 packet
#define DLL_BUFFER_RX_NUM	32	// ~1k bytes

// Field definition
#define FRAME_HEADER		0xaa
#define FRAME_FOOTER		0xaa
#define FRAME_ESCAPE		0xcc
// "Control" field
// Intermediate frames in series
#define FRAME_CTRL_CONT		0x0000
// Last frame in series
#define FRAME_CTRL_LAST		0x0001

/*
 * Transmit & receive buffer, required for reconstruct packet
 */

static union dll_tx_buffer_t {
	struct {
		volatile uint8_t length;		// Total length (validity)
		uint8_t data[DLL_BUFFER_TX_SIZE];	// Buffer space
	} d;
	struct {
		volatile uint8_t valid;			// Total length (validity)
		uint16_t control;
		uint16_t address;			// Index
		uint8_t length;				// Length of data section
		uint8_t data[0];			// Start of data section
		// Followed by uint16_t checksum
	} s;
} tx[DLL_BUFFER_TX_NUM];

static union dll_rx_buffer_t {
	struct {
		volatile uint8_t length;		// Total length (validity)
		uint8_t data[DLL_BUFFER_RX_SIZE];	// Buffer space
	} d;
	struct {
		volatile uint8_t valid;			// Total length (validity)
		uint16_t control;
		uint16_t address;
		uint8_t length;				// Length of data section
		uint8_t data[0];			// Start of data section
	} s;
} rx[DLL_BUFFER_RX_NUM];

#define CHECKSUM(st)	((uint16_t *)((st).s.data + (st).s.length))

/*
 * Control and status, state machine
 */

static struct dll_ctrl_t
{
	struct {
		volatile uint8_t id;		// Current reading tx buffer id
		uint8_t offset;	// Current transmitting offset in a frame
		uint8_t escaped;
	} tx;
	struct {
		volatile uint8_t id;		// Current writing rx buffer id
		uint8_t recursion;
		uint8_t offset;			// Current receiving offset in a frame
		uint8_t escaped;
		uint16_t checksum;		// Checksum computation
	} rx;
} ctrl;

/*
 * Functions to control buffers
 */

#define PREV(id, size)	(((id) - 1) & ((size) - 1))
#define NEXT(id, size)	(((id) + 1) & ((size) - 1))

#undef DEBUG
//#define DEBUG
static void dll_tx_push(uint16_t address, uint16_t control, const uint8_t *data, uint8_t length)
{
	// Find next available buffer slot
	uint8_t id = ctrl.tx.id;
	union dll_tx_buffer_t *ptr;
#ifdef DEBUG
	printf("\e[95mPUSH");
#endif
	while ((ptr = tx + id)->s.valid)
		id = NEXT(id, DLL_BUFFER_TX_NUM);
	// Fill a buffer
	ptr->s.control = control;
	ptr->s.address = address;
	ptr->s.length = length;
	memcpy(ptr->s.data, data, length);
	uint16_t checksum = 0;
	uint8_t i = length + DLL_BUFFER_TX_APPEND - 2;	// Control, Address, Length, Data
	uint8_t *p = ptr->d.data;
	while (i--)
		checksum += *p++;
	checksum = -checksum;
	memcpy(CHECKSUM(*ptr), &checksum, 2);	// Strict aliasing rule?
	// Mark as valid
	ptr->d.length = length + DLL_BUFFER_TX_APPEND;
#ifdef DEBUG
	printf("%02x/%02x#%04x\n", id, ptr->d.length, checksum);
#endif
	// Start transmission
	if (phy_mode() == PHYTX)
		return;
	while (!phy_free() || rand() % 50000 < CSMA * 50000) {
		_delay_ms(1);
	}
	phy_transmit();
}

#if 1
// Invalidate frames starting from <id>
// Return the first frame after invalid frames
#undef DEBUG
static uint8_t dll_rx_invalidate(uint8_t id)
{
	union dll_rx_buffer_t *ptr = rx + id;
	if (!ptr->s.valid)
		return id;
	for (;;) {
#ifdef DEBUG
		printf("$%02x", id);
#endif
		ptr->s.valid = 0;
		//if (ptr->s.control == FRAME_CTRL_LAST)
		//	return;
		id = NEXT(id, DLL_BUFFER_RX_NUM);
		ptr = rx + id;
		if (!ptr->s.valid || ptr->s.address == 0)
			break;
	}
	return id;
}
#endif

/*
 * Interface with upper layer
 */

// Send out a packet
void dll_write(const uint8_t *packet, uint8_t length)
{
	// Split large packet into multiple frames
	uint8_t address = 0;
	while (length > FRAME_DATA_MAX_SIZE) {
		// Write to buffer
		dll_tx_push(address++, 0, packet, FRAME_DATA_MAX_SIZE);
		packet += FRAME_DATA_MAX_SIZE;
		length -= FRAME_DATA_MAX_SIZE;
	}
	// Write the last frame to buffer
	dll_tx_push(address, 1, packet, length);
}

uint8_t dll_written()
{
	return phy_mode() == PHYRX;
}

#undef DEBUG
// Take 1 packet, return the actual size
uint8_t dll_read(uint8_t *packet)
{
	uint8_t id = ctrl.rx.id;
	union dll_rx_buffer_t *ptr = rx + id;
#ifdef DEBUG
	printf_P(PSTR("\e[94mR{%02x}"), ctrl.rx.id);
#endif
	while (!ptr->s.valid) {
		if ((id = NEXT(id, DLL_BUFFER_RX_NUM)) == ctrl.rx.id) {
#ifdef DEBUG
			putchar('X');
#endif
			return 0;
		}
#ifdef DEBUG
		printf("I<%02x>", id);
#endif
		ptr = rx + id;
	}
	if (ptr->s.address != 0) {	// Invalid starting address
		id = dll_rx_invalidate(id);
#ifdef DEBUG
		putchar('S');
#endif
	}
	uint8_t length = 0;
	uint16_t address = 0;
loop:
	ptr = rx + id;
	uint8_t first = id;
#ifdef DEBUG
	printf("\nL<%02x>[%04x/%04x#%04x](%02x)", id, address, ptr->s.address, ptr->s.control, ptr->s.valid);
#endif
	while (ptr->s.valid) {
#ifdef DEBUG
		printf("\nM<%02x>[%04x/%04x#%04x](%02x)", id, address, ptr->s.address, ptr->s.control, ptr->s.valid);
#endif
		if (ptr->s.address != address++) {	// Discontinued
			id = dll_rx_invalidate(first);
			length = 0;
			address = 0;
#ifdef BDEBUG
			putchar('D');
#endif
			goto loop;
		}
		memcpy(packet, ptr->s.data, ptr->s.length);
		length += ptr->s.length;
		packet += ptr->s.length;
		if (ptr->s.control == FRAME_CTRL_LAST) {	// Complete frame
			dll_rx_invalidate(first);
#ifdef DEBUG
			putchar('C');
#endif
			return length;
		}
		id = NEXT(id, DLL_BUFFER_RX_NUM);
		ptr = rx + id;
	}
#ifdef DEBUG
	putchar('E');
#endif
	return 0;
}

/*
 * Interface with lower layer
 */

#undef DEBUG
//#define DEBUG
#if 1
// Receive data stream from PHY, called from ISR
void dll_data_handler(const uint8_t data)
{
	uint8_t offset = ctrl.rx.offset;
	uint8_t id = ctrl.rx.id;
	union dll_rx_buffer_t *ptr = rx + id;

	if (offset == 0) {
		// Receiving header
		if (data == FRAME_HEADER)
			goto header;
		else
			phy_receive();	// Reset receiver
		return;
	}

	if (offset == FRAME_MAX_SIZE) {
		// Data overwrite, invalid frame
		goto abort;
	}

	// Escape sequence
	if (!ctrl.rx.escaped && data == FRAME_ESCAPE) {
#ifdef DEBUG
		putchar('E');
#endif
		ctrl.rx.escaped = 1;
		return;
	}

	// Check un-escaped header & footer
	if (!ctrl.rx.escaped)
		if (data == FRAME_HEADER || data == FRAME_FOOTER)
			goto check;
	ctrl.rx.escaped = 0;

	// General packet data
#ifdef DEBUG
	if (isprint(data))
		putchar(data);
	else
		printf("\\x%02x", data);
#endif
	*(ptr->d.data + offset - 1) = data;
	ctrl.rx.checksum += data;
	ctrl.rx.offset = offset + 1;
	return;

	/* Header / footer received, check validation of received frame */
check:
#ifdef DEBUG
	printf("K(%02x,%02x)", ptr->s.length, offset);
#endif
	// Data fields received:
	//      1     2  3     4  5       6   6+L       8+L     9+L
	// Header, Control, Address, Length, Data, Checksum, Footer
	if (offset < 6)
		goto ignore;	// Length field not yet received
	else if (offset != ptr->s.length + FRAME_APPEND_SIZE - 1)
		goto ignore;	// Incorrect data length
	union {
		uint16_t u16;
		struct {
			uint8_t l;
			uint8_t h;
		} s;	// Little endian
	} ckrecv;
	memcpy(&ckrecv.u16, ptr->s.data + ptr->s.length, 2);
	ctrl.rx.checksum = ctrl.rx.checksum + ckrecv.u16 - ckrecv.s.h - ckrecv.s.l;
#ifdef DEBUG
	printf("<%04x/%04x>", ckrecv.u16, ctrl.rx.checksum);
#endif
	if (ctrl.rx.checksum != 0)
		goto ignore;	// Checksum failure
	goto complete;
	return;

	/* Ignore this frame */
ignore:
#ifdef DEBUG
	putchar('I');
	putchar('\n');
#endif

	/* Header received */
header:
	ctrl.rx.offset = 1;
	ctrl.rx.checksum = 0;
#ifdef DEBUG
	fputs("\e[93m", stdout);
	printf("H{%02x}", ctrl.rx.id);
#endif
	return;

	/* Frame successfully received */
complete:
#ifdef DEBUG
	putchar('C');
#endif
	ctrl.rx.offset = 0;
	// Check control field
	switch (ptr->s.control) {
	case FRAME_CTRL_CONT:
	case FRAME_CTRL_LAST:
		break;
	default:
		goto ignore;
	}
	// Check address field
	union dll_rx_buffer_t *prev = rx + PREV(id, DLL_BUFFER_RX_NUM);
	if (!prev->s.valid || prev->s.control == FRAME_CTRL_LAST) {
		if (ptr->s.address != 0)
			goto ignore;
	} else if (ptr->s.address != 0 && prev->s.address + 1 != ptr->s.address)
		goto drop;
	// Frame accepted
	ptr->s.valid = DLL_BUFFER_RX_APPEND + ptr->s.length;
	ctrl.rx.id = NEXT(id, DLL_BUFFER_RX_NUM);
#ifdef DEBUG
	printf("A<%02x>(%04d)[%02x]\n", id, (rx + id)->s.address, ptr->s.valid);
#endif
	return;

	/* Frame corrupted, drop entire packet */
drop:
	id = PREV(id, DLL_BUFFER_RX_NUM);
	// prev is pre-loaded in 'complete' routine
	while (prev->s.valid && prev->s.control != FRAME_CTRL_LAST) {
		prev->s.valid = 0;
		id = PREV(id, DLL_BUFFER_RX_NUM);
		prev = rx + id;
	}
	ctrl.rx.id = NEXT(id, DLL_BUFFER_RX_NUM);
#ifdef DEBUG
	putchar('D');
	putchar('\n');
#endif
	return;

	/* Invalid data recorded */
abort:
	ctrl.rx.offset = 0;
	// Reset receiver
	phy_receive();
#ifdef DEBUG
	putchar('X');
	putchar('\n');
#endif
}
#endif

#undef DEBUG
//#define DEBUG
// Transmit data stream request from PHY, called from ISR
uint8_t dll_data_request(uint8_t *buffer)
{
	uint8_t id = ctrl.tx.id;
	union dll_tx_buffer_t *ptr;
start:
	ptr = tx + id;
	//printf("(%02x/%02x@%02x)", ctrl.tx.offset, (tx + id)->d.length, id);
	if (!ptr->s.valid) {
		// No more packet available
#ifdef DEBUG
		printf(";\n");
#endif
		ctrl.tx.offset = 0;
		return 0;
	}
	if (ctrl.tx.offset == 0) {
		// Header
#ifdef DEBUG
		fputs("\e[94m", stdout);
		//putchar('H');
		printf("H{%02x}", id);
#endif
		*buffer = FRAME_HEADER;
		ctrl.tx.offset++;
		return 1;
	} else if (ctrl.tx.offset == ptr->d.length + 1) {
		// Footer
#ifdef DEBUG
		putchar('F');
#endif
		*buffer = FRAME_FOOTER;
		ctrl.tx.offset++;
		return 1;
	} else if (ctrl.tx.offset == ptr->d.length + 2) {
		// Packet transmitted
#ifdef DEBUG
		printf("P\n");
#endif
		ptr->s.valid = 0;
		ctrl.tx.id = id = NEXT(id, DLL_BUFFER_TX_NUM);
		ctrl.tx.offset = 0;
		goto start;
	}
	uint8_t data = *(ptr->d.data + ctrl.tx.offset - 1);
	if (!ctrl.tx.escaped && (data == FRAME_HEADER || data == FRAME_FOOTER || data == FRAME_ESCAPE)) {
		ctrl.tx.escaped = 1;
		*buffer = FRAME_ESCAPE;
#ifdef DEBUG
		putchar('E');
#endif
	} else {
		ctrl.tx.escaped = 0;
		ctrl.tx.offset++;
		*buffer = data;
#ifdef DEBUG
		if (isprint(data))
			putchar(data);
		else
			printf("\\x%02x", data);
#endif
	}
	return 1;
}

/*
 * Miscellaneous
 */

// Initialise data structure
void dll_init()
{
	//ctrl.mode = RX;
	ctrl.tx.id = 0;
	ctrl.tx.offset = 0;
	ctrl.tx.escaped = 0;
	ctrl.rx.id = 0;
	ctrl.rx.offset = 0;
	ctrl.rx.escaped = 0;
	ctrl.rx.checksum = 0;
	ctrl.rx.recursion = 0;

	uint8_t i;
	for (i = 0; i < DLL_BUFFER_TX_NUM; i++)
		tx[i].s.valid = 0;
	for (i = 0; i < DLL_BUFFER_RX_NUM; i++)
		rx[i].s.valid = 0;
}
