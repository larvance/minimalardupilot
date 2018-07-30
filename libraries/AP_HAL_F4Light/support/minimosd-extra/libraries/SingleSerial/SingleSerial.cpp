// -*-  tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: t -*-
//
// Simple and fast Interrupt-driven serial transmit/receive library for chips with single hardware serial port
//
//	Copyright (c) 2015 NightGhost night_ghost@ykoctpa.ru  All rights reserved..
//
// based on FastSerial by Michael Smith and Arduino's HardwareSerial
//
//      Copyright (c) 2010 Michael Smith. All rights reserved.
//
// Receive and baudrate calculations derived from the Arduino
// HardwareSerial driver:
//
//      Copyright (c) 2006 Nicholas Zambetti.  All right reserved.
//
// Transmit algorithm inspired by work:
//
//      Code Jose Julio and Jordi Munoz. DIYDrones.com
//
//      This library is free software; you can redistribute it and/or
//      modify it under the terms of the GNU Lesser General Public
//      License as published by the Free Software Foundation; either
//      version 2.1 of the License, or (at your option) any later version.
//
//      This library is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//      Lesser General Public License for more details.
//
//      You should have received a copy of the GNU Lesser General Public
//      License along with this library; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//


#include "SingleSerial.h"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif


SingleSerial::Buffer SingleSerial::_rxBuffer;
SingleSerial::Buffer SingleSerial::_txBuffer;

uint8_t SingleSerial::_rxBytes[SERIAL_RX_BUFFER_SIZE];
uint8_t SingleSerial::_txBytes[SERIAL_TX_BUFFER_SIZE];

#ifdef DEBUG
extern volatile uint16_t lost_bytes;
#endif

#define IS_OPEN (UCSR0B & _BV(RXEN0) )

// Constructor /////////////////////////////////////////////////////////////////

SingleSerial::SingleSerial() 
{
//	begin(57600); no autostart!
}

// Public Methods //////////////////////////////////////////////////////////////

void SingleSerial::begin(unsigned long baud)
{
	uint16_t ubrr;
	bool use_u2x = true;

	// if we are currently open...
	if (IS_OPEN) {
		end();
	}

	// reset buffer pointers
	_txBuffer.head = _txBuffer.tail = 0;
	_rxBuffer.head = _rxBuffer.tail = 0;

	// If the user has supplied a new baud rate, compute the new UBRR value.
	if (baud > 0) {
//*
#if F_CPU == 16000000UL
		// hardcoded exception for compatibility with the bootloader shipped
		// with the Duemilanove and previous boards and the firmware on the 8U2
		// on the Uno and Mega 2560.
		if (baud == 57600)
			use_u2x = false;
#endif
//*/
		if (use_u2x) {
			UCSR0A = 1 << U2X0;
			ubrr = (F_CPU / 4 / baud - 1) / 2;
		} else {
			UCSR0A = 0;
			ubrr = (F_CPU / 8 / baud - 1) / 2;
		}

		UBRR0H = ubrr >> 8;
		UBRR0L = ubrr;
	}

	//UCSR0B |= (_BV(RXEN0) |  _BV(TXEN0) | _BV(RXCIE0));
	UCSR0B = _BV(RXEN0) |  _BV(TXEN0) | _BV(RXCIE0);
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8 bit
}

void SingleSerial::begin(unsigned long baud, uint8_t mode)
{
    begin(baud);
    UCSR0C = mode;
}


void SingleSerial::end(){
	//UCSR0B &= ~((_BV(RXEN0) |  _BV(TXEN0) | _BV(RXCIE0)) | _BV(UDRIE0));
	UCSR0B = 0;

}

uint8_t SingleSerial::available_S(void){
	if (!IS_OPEN)
		return 0;
	return ((_rxBuffer.head - _rxBuffer.tail) & (SERIAL_RX_BUFFER_SIZE -1) );
}

uint8_t SingleSerial::available(void)
{
	return available_S();
}

/*
uint8_t SingleSerial::txspace(void)
{
	if (!IS_OPEN)
		return (0);
	return ((SERIAL_TX_BUFFER_SIZE) - ((_txBuffer.head - _txBuffer.tail) & SERIAL_TX_BUFFER_SIZE));
}
*/
uint8_t SingleSerial::read_S(void)
{
	uint8_t c;

	// if the head and tail are equal, the buffer is empty
	if (!IS_OPEN || (_rxBuffer.head == _rxBuffer.tail))
		return 0;

	// pull character from tail
	c = _rxBytes[_rxBuffer.tail];
	_rxBuffer.tail = (_rxBuffer.tail + 1) & (SERIAL_RX_BUFFER_SIZE-1);

	return c;
}

uint8_t SingleSerial::read(void){
    return read_S();
}

uint8_t SingleSerial::peek(void){

	// if the head and tail are equal, the buffer is empty
	if (!IS_OPEN || (_rxBuffer.head == _rxBuffer.tail))
		return 0;

	// pull character from tail
	return _rxBytes[_rxBuffer.tail];
}

void SingleSerial::flush(void){
	// don't reverse this or there may be problems if the RX interrupt
	// occurs after reading the value of _rxBuffer->head but before writing
	// the value to _rxBuffer->tail; the previous value of head
	// may be written to tail, making it appear as if the buffer
	// don't reverse this or there may be problems if the RX interrupt
	// occurs after reading the value of head but before writing
	// the value to tail; the previous value of rx_buffer_head
	// may be written to tail, making it appear as if the buffer
	// were full, not empty.
	_rxBuffer.head = _rxBuffer.tail;

	// don't reverse this or there may be problems if the TX interrupt
	// occurs after reading the value of _txBuffer->tail but before writing
	// the value to _txBuffer->head.
	_txBuffer.tail = _txBuffer.head;

#ifdef DEBUG
    lost_bytes=0;
#endif

}

void SingleSerial::wait(void){
    while (SingleSerial::_txBuffer.tail != SingleSerial::_txBuffer.head);
}

#if defined(ARDUINO) && ARDUINO >= 100

void SingleSerial::write_S(uint8_t c)
{
	uint8_t i;

	if (!IS_OPEN) // drop bytes if not open
		return;

	
	i = (_txBuffer.head + 1) & (SERIAL_TX_BUFFER_SIZE-1);

	while (i == _txBuffer.tail)// wait for room in the tx buffer
		;

	// add byte to the buffer
	_txBytes[_txBuffer.head] = c;
	_txBuffer.head = i;

	// enable the data-ready interrupt, as it may be off if the buffer is empty
	UCSR0B |= _BV(UDRIE0);
}

size_t SingleSerial::write(uint8_t c) {
    write_S(c);
    // return number of bytes written (always 1)
    return 1;

}


#else
void SingleSerial::write(uint8_t c)
{
	uint8_t i;

	if (!IS_OPEN) // drop bytes if not open
		return;

	i = (_txBuffer.head + 1) & (SERIAL_TX_BUFFER_SIZE-1);
	while (i == _txBuffer.tail) 	// wait for room in the tx buffer
		;

	// add byte to the buffer
	_txBytes[_txBuffer.head] = c;
	_txBuffer.head = i;

	// enable the data-ready interrupt, as it may be off if the buffer is empty
	UCSR0B |= _BV(UDRIE0));
}
#endif


void serialEventRun(void){}
