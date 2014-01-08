#include <util/delay.h>
#include <avr/io.h>

#define ATTINY4313

// Port and pin names lifted from the AVR310 routines

#ifdef ATTINY4313
#define DDR_USI			DDRB
#define PORT_USI		PORTB
#define PIN_USI			PINB
#define PORT_USI_SDA	PORTB5
#define PORT_USI_SCL	PORTB7
#define PIN_USI_SDA		PINB5
#define PIN_USI_SCL		PINB7
#endif

#ifdef ATTINY85
#define DDR_USI			DDRB
#define PORT_USI		PORTB
#define PIN_USI			PINB
#define PORT_USI_SDA	PORTB0
#define PORT_USI_SCL	PORTB2
#define PIN_USI_SDA		PINB0
#define PIN_USI_SCL		PINB2
#endif

/*
	TWI clock (SCL) timing delays in microseconds.
	T_LOW is the time to hold SCL low between pulses.
	T_HIGH is the minimum time needed for reading SDA.
	T_LOW is also the time required between STOP and START bits.
	T_LOW is the time needed to set up a START bit.
	T_HIGH is the time needed to set up a STOP bit.
	TWI_FAST is for up to 400Kbits/sec. Otherwise, it's 100Kbits/sec max.
*/
#ifdef TWI_FAST
#define T_LOW	1.3
#define T_HIGH	0.6
#else
#define T_LOW	4.7
#define T_HIGH	4.0
#endif

/*
	Function prototypes
*/
void usi_twi_master_initialize(void);
uint8_t usi_twi_master_transmit(uint8_t *data, uint8_t nbytes);
void usi_twi_master_receive(uint8_t *data, uint8_t nbytes);
void usi_twi_master_start(void);
void usi_twi_master_stop(void);
uint8_t usi_twi_master_transfer(uint8_t nbits);
void usi_twi_master_receive(uint8_t*, uint8_t);

/*
	USICR_SETUP sets up the USICR (USI Control Register). It's
	main use is in bit 0, which toggles the SCL line each time
	this value is loaded into USICR.

const uint8_t USICR_SETUP =
		(0<<USISIE) | (0<<USIOIE) |		// No interrupts
		(1<<USIWM1) | (0<<USIWM0) |		// TWI mode
		(1<<USICS1) | (0<<USICS0) |		// Software clock strobe
		(1<<USICLK)	|					// Shift data register & increment counter
		(1<<USITC);						// Toggle clock port pin
*/
#define USICR_SETUP 0b00101011			// see above
#define USISR_8_BIT	0b11110000			// Resets counter to last 4 bits (0 here)
#define USISR_1_BIT	0b11111110			// Resets counter to last 4 bits (14 here)

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif
