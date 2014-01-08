#include "usi_twi_master.h"

/*
	Initialize USI for TWI. This guarantees the SCL and SDA pins
	are set up as outputs and will have pullups when used as inputs.
	2013-11-23/au. [we probably don't need to do any of this]
*/
void usi_twi_master_initialize(void)
{

	PORT_USI |= _BV(PIN_USI_SCL);		// SCL high
	PORT_USI |= _BV(PIN_USI_SDA);		// SDA high
	DDR_USI |= _BV(PIN_USI_SCL);		// Set SCL pin as output
	DDR_USI |= _BV(PIN_USI_SDA);		// Set SDA pin as output

}

/*
	Read one byte from the TWI bus. Called by usi_twi_master_receive()
*/
uint8_t usi_twi_master_recvByte(void)
{

	uint8_t data;

	DDR_USI &= ~_BV(PIN_USI_SDA);				// Set SDA as input
	data  = usi_twi_master_transfer(8);			// Read a byte
	return(data);

}

/*
	A start condition must precede this call. The first byte must
	be a properly formatted I2C address with R/W (bit 0) set to 1.
	Returns the data in the supplied array; the first byte of the
	array is not changed. 2013-11-23/au.
*/
void usi_twi_master_receive(uint8_t *data, uint8_t nbytes)
{

	usi_twi_master_transmit(data++, 1);				// Send the address
	nbytes--;
	do {
		*(data++) = usi_twi_master_recvByte();
		USIDR = (nbytes == 1 ? 0xFF : 0x00);		// Load NACK (0xFF) if last byte
		usi_twi_master_transfer(1);					// Send ACK or NACK
	} while (--nbytes);

}

/*
	Puts a start condition on the bus. Taken from the AVR310 routines
*/
void usi_twi_master_start(void)
{

	PORT_USI |= _BV(PIN_USI_SCL);					// SCL high
	while( !(PORT_USI & _BV(PIN_USI_SCL)) ) {		// Wait for clock stretching to end
	}
	_delay_us(T_LOW);								// Setup time for START
	PORT_USI &= ~_BV(PIN_USI_SDA);					// Pull SDA LOW
	_delay_us(T_HIGH);								// Wait
	PORT_USI &= ~_BV(PIN_USI_SCL);					// Pull SCL LOW
	PORT_USI |= _BV(PIN_USI_SDA);					// Release SDA

}

/*
	Puts a stop condition on the bus. Taken from the AVR310 routines
	and cleaned up a bit. 2013-11-21/au.
*/
void usi_twi_master_stop(void)
{

	PORT_USI &= ~_BV(PIN_USI_SDA);				// SDA low
	PORT_USI |= _BV(PIN_USI_SCL);				// SCL high
	while(!(PIN_USI & _BV(PIN_USI_SCL))) {		// Wait for clock stretching to end
	}
	_delay_us(T_HIGH);							// Setup time for STOP
	PORT_USI |= _BV(PIN_USI_SDA);				// SDA high
	_delay_us(T_LOW);                			// Time to hold STOP

}

/*
	Called by usi_twi_master_transmit, usi_twi_master_receive, and
	usi_twi_master_recvByte. Load USIDR with data for write operations.
	Transfers 8 bits for data or one bit for an ACK. Set SDA direction
	(input or output) before entering this routine. Always leaves SDA
	as output. 2013-12-28/au
*/
uint8_t usi_twi_master_transfer(uint8_t nbits)
{

	uint8_t i;

	// Set the 4-bit counter for a 1-bit or 8-bit data transfer
	USISR = (nbits == 1 ? USISR_1_BIT : USISR_8_BIT);

	do {
		_delay_us(T_LOW);				// Set up time for transfer
		USICR = USICR_SETUP;			// Clock strobe, rising SCL edge
		i = 0;
		while(!(PIN_USI & _BV(PIN_USI_SCL))) {
			_delay_us(T_LOW);			// Wait for SCL to go high
		}
		_delay_us(T_HIGH);				// USI reads the bit here
		USICR = USICR_SETUP;			// Clock strobe, falling SCL edge
	} while(!(USISR & _BV(USIOIF)));	// Until 4-bit counter overflow. Both rising
										// and falling edges increment the counter
	_delay_us(T_LOW);
	i = USIDR;							// Read the incoming data byte
	DDR_USI |= _BV(PIN_USI_SDA);		// Set SDA as output
	USIDR = 0xFF;						// Set SDA high. Bit 7 is the SDA pin output latch.
	return(i);							// Return received data

}

/*
	A start condition must precede this call. The first byte must
	be a properly formatted I2C address with R/W (bit 0) set to 0.
	Returns 1 (TRUE) on success, 0 (FALSE) otherwise. If FALSE, you
	should abort the transfer (send a stop) before proceeding.
	Taken from USI_TWI_Start_Transceiver_With_Data (AVR310).
	2013-11-23/au.
*/
uint8_t usi_twi_master_transmit(uint8_t *data, uint8_t nbytes)
{

	do {

		PORT_USI &= ~_BV(PIN_USI_SCL);					// Set SCL low
		USIDR = *(data++);								// Load data byte
		usi_twi_master_transfer(8);						// Send 8 bits on bus
		DDR_USI &= ~_BV(PIN_USI_SDA);					// Enable SDA as input

		if ((usi_twi_master_transfer(1) & (0x01))) {	// If slave doesn't ACK
			return(FALSE);
		}

	} while (--nbytes);

	return(TRUE);

}
