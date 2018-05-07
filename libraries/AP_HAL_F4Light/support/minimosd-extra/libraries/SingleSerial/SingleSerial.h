// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: t -*-
//
//
//
// Simple and fast Interrupt-driven serial transmit/receive library for chips with single hardware serial port
//
//	Copyright (c) 2015 NightGhost night_ghost@ykoctpa.ru  All rights reserved. 
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
//      version 2.1 of the License, or (at your option) any later
//      version.
//
//      This library is distributed in the hope that it will be
//      useful, but WITHOUT ANY WARRANTY; without even the implied
//      warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//      PURPOSE.  See the GNU Lesser General Public License for more
//      details.
//
//      You should have received a copy of the GNU Lesser General
//      Public License along with this library; if not, write to the
//      Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//      Boston, MA 02110-1301 USA
//

//
// Note that this library does not pre-declare drivers for serial
// ports; the user must explicitly create drivers for the ports they
// wish to use.  This is less friendly than the stock Arduino driver,
// but it saves a few bytes of RAM for every unused port and frees up
// the vector for another driver (e.g. MSPIM on USARTs).
//

#ifndef SingleSerial_h
#define SingleSerial_h

// disable the stock Arduino serial driver
#ifdef HardwareSerial_h
# error Must include SingleSerial.h before the Arduino serial driver is defined.
#endif
#define HardwareSerial_h

#include <inttypes.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "BetterStream.h"

#if !defined(SERIAL_TX_BUFFER_SIZE)
#define SERIAL_TX_BUFFER_SIZE 16
#endif
#if !defined(SERIAL_RX_BUFFER_SIZE)
#define SERIAL_RX_BUFFER_SIZE 128
#endif

/// @file	SingleSerial.h
/// @brief	An enhanced version of the Arduino HardwareSerial class
///			implementing interrupt-driven transmission 
///
/// Because Arduino libraries aren't really libraries, but we want to
/// only define interrupt handlers for serial ports that are actually
/// used, we have to force our users to define them using a macro.
///
/// SingleSerialPort(<port name>)
///
/// <port name> is the name of the object that will be created by the
/// macro.

///	@name	Compatibility
///

class SingleSerial: public BetterStream {
public:

	/// Constructor
	SingleSerial();

	/// @name 	Serial API
    //@{
	void begin(long baud);
	void end(void);
	virtual uint8_t available(void);
//	virtual uint8_t txspace(void);
	virtual uint8_t read(void);
	virtual uint8_t peek(void);
	virtual void flush(void);
	void wait();
#if defined(ARDUINO) && ARDUINO >= 100
	virtual size_t write(uint8_t c);
#else
	virtual void write(uint8_t c);
#endif
	using BetterStream::write;
    //@}


	/// Transmit/receive buffer descriptor.
	///
	/// Public so the interrupt handlers can see it
	struct Buffer {
		volatile uint8_t head, tail;	///< head and tail pointers
	};

	// ring buffers
	static Buffer	_rxBuffer;
	static Buffer	_txBuffer;
	static uint8_t	_rxBytes[SERIAL_RX_BUFFER_SIZE];
	static uint8_t	_txBytes[SERIAL_TX_BUFFER_SIZE];

private:

};



/// Generic Rx/Tx vectors for a serial port 
///
#define SingleSerialHandler( _RXVECTOR, _TXVECTOR, _UDR, _UCSRB, _TXBITS) \
ISR(_RXVECTOR, ISR_BLOCK)                                               \
{                                                                       \
        uint8_t c;                                                      \
        uint8_t i;                                                      \
                                                                        \
        /* read the byte as quickly as possible */                      \
        c = _UDR;                                                       \
        /* work out where the head will go next */                      \
        i = (SingleSerial::_rxBuffer.head + 1) & (SERIAL_RX_BUFFER_SIZE -1); \
        /* decide whether we have space for another byte */             \
        if (i != SingleSerial::_rxBuffer.tail) {                  \
            /* we do, move the head */                              \
            SingleSerial::_rxBytes[SingleSerial::_rxBuffer.head] = c; \
            SingleSerial::_rxBuffer.head = i;                     \
        } /* buffer overflow */                                   \
/*        else    digitalWrite(LEDPIN, !digitalRead(LEDPIN));  */ \
}                                                                       \
ISR(_TXVECTOR, ISR_BLOCK)                                               \
{                                                                       \
        /* if there is another character to send */                     \
        if (SingleSerial::_txBuffer.tail != SingleSerial::_txBuffer.head) { \
                _UDR = SingleSerial::_txBytes[SingleSerial::_txBuffer.tail]; \
                /* increment the tail */                                \
                SingleSerial::_txBuffer.tail =                    \
                        (SingleSerial::_txBuffer.tail + 1) & (SERIAL_TX_BUFFER_SIZE-1); \
        } else {                                                        \
                /* there are no more bytes to send, disable the interrupt */ \
            /*    if (SingleSerial::_txBuffer.head == SingleSerial::_txBuffer.tail) */ \
                        _UCSRB &= ~_TXBITS;                             \
        }                                                               \
}                                                                       \
struct hack


///
/// Macro defining a SingleSerial port instance.
///
#define SingleSerialPort(_name)   \
      SingleSerialHandler(USART0_RX_vect, USART0_UDRE_vect, UDR0, UCSR0B, _BV(UDRIE0)); \
      SingleSerial _name


//
// Portability; convert various older sets of defines for U(S)ART0 up
// to match the definitions for the 1280 and later devices.
//
#if !defined(USART0_RX_vect)
# if defined(USART_RX_vect)
#  define USART0_RX_vect        USART_RX_vect
#  define USART0_UDRE_vect      USART_UDRE_vect
# elif defined(UART0_RX_vect)
#  define USART0_RX_vect        UART0_RX_vect
#  define USART0_UDRE_vect      UART0_UDRE_vect
# endif
#endif


#if !defined(UDR0)
# if defined(UDR)
#  define UDR0                  UDR
#  define UBRR0H                UBRRH
#  define UBRR0L                UBRRL
#  define UCSR0A                UCSRA
#  define UCSR0B                UCSRB
#  define U2X0                  U2X
#  define RXEN0                 RXEN
#  define TXEN0                 TXEN
#  define RXCIE0                RXCIE
#  define UDRIE0                UDRIE
# endif
#endif


/// Forward declarations for clients that want to assume that the
/// default Serial* objects exist.
///
/// Note that the application is responsible for ensuring that these
/// actually get defined, otherwise Arduino will suck in the
/// HardwareSerial library and linking will fail.
//@{
extern class SingleSerial Serial;
//@}

/// The SingleSerial class definition
///


#endif // SingleSerial_h
