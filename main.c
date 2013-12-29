/*

	Check fuses:

	avrdude -c usbtiny -p ATtiny4313 -P usb -U lfuse:r:-:h
	Burned new fuses: 0xFF 0xDF 0xFF
	>avrdude -p ATtiny4313 -c usbtiny -U lfuse:w:0xff:m
	The 0xFF in lfuse gives the longest startup time with a crystal
	Using a 14.745600 MHz crystal.

*/

//#define DEBUG
//#define PN532DEBUG

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdlib.h>						// for atoi()
#ifdef MCPDEBUG
#include "mcp23008.h"					// for debugging
#endif
#include "usi_twi_master.h"
#include "PN532_twi.h"

#define LEDPIN PB2						// PB2 is the pin for OC0A
#define brightness(X) (OCR0A = X)		// PWM on PB2

#define COMMANDSENT (UCSRA & _BV(RXC))	// Is there something in the RX buffer?
#define TXREADY (UCSRA & _BV(UDRE))		// Is the transmit buffer empty?

#define TRUE	1
#define FALSE	0

void errMsg(uint8_t);
void init_ATtiny4313(void);
uint16_t recvNum(void);
void sendBlock(uint8_t*, uint8_t);
void sendPrompt(void);
void sendByte(uint8_t);
void sendCRLF(void);
void sendNum(int, uint8_t);
void sendString(char *);
uint8_t writeMiFare(uint8_t*);

int main (void)
{

	uint8_t i, j, cmd, frameBuf[36], cardID[5];

	init_ATtiny4313();

	usi_twi_master_initialize();

#ifdef MCPDEBUG
	if (!mcp23008_Init(MCP23008_BASEADDR)) {
		errMsg(1);
	}
#endif

#ifdef PN532DEBUG
	if (pn532_twi_sendCommand(GETFIRMWAREVERSION, frameBuf, 0)) {
		for (i = 0; i < 15; i++) {
			sendNum((int) frameBuf[i], 16);
			sendByte(' ');
		}
	} else {
		errMsg(2);
	}
#endif
	if (!pn532_twi_sendCommand(SAMCONFIG, frameBuf, 0)) {
		errMsg(15);
	}
	if (!	pn532_twi_sendCommand(RFCONFIGURATION, frameBuf, 0)) {
		errMsg(16);
	}

	sendPrompt();

	for (;;) {
		if (COMMANDSENT) {
			cmd = UDR;
			sendByte(cmd);
			switch(cmd) {
				case ('R'):
					wdt_enable(WDTO_60MS);
					while (1) {};
					break;

				case ('+'):
					brightness(255);
					break;

				case ('-'):
					brightness(0);
					break;

				case ('v'):
					if (pn532_twi_sendCommand(GETFIRMWAREVERSION, frameBuf, 0)) {
						sendNum((int) frameBuf[10], 10);
						sendByte('.');
						sendNum((int) frameBuf[11], 10);
//						for (i = 0; i < 15; i++) {
//							sendNum((int) frameBuf[i], 16);
//							sendByte(' ');
//						}
					} else {
						errMsg(2);
					}
					break;
#ifdef MCPDEBUG
				case ('m'):
					mcp23008_Write(MCP23008_BASEADDR, GPIO, 128);
				/*
					for (i = 0; i < 128; i++) {
						if (!mcp23008_Write(MCP23008_BASEADDR, GPIO, i)) {
							errMsg(14);
							break;
						}
						_delay_ms(5);
					}
				*/
					break;

				case ('n'):
					mcp23008_Write(MCP23008_BASEADDR, GPIO, 0);
					break;
#endif


				case ('r'):
					brightness(200);
					pn532_twi_sendCommand(INLISTPASSIVETARGET, frameBuf, 0);
					if (frameBuf[9]) {
						for (i = 0; i < 4; i++) {
							cardID[i] = frameBuf[i+15];
						}
						sendCRLF();
//						for (i = 0; i < 4; i++) {
//							sendNum(cardID[i], 16);
//						}
//						sendCRLF();
						if (pn532_readBlock(cardID, 1, frameBuf)) {;
							//pn532_writeBlock(cardID, 1, dataBlock, frameBuf);
							sendBlock(frameBuf, 10);
						} else {
							errMsg(17);
						}
						if (pn532_readBlock(cardID, 2, frameBuf)) {;
							//pn532_writeBlock(cardID, 1, dataBlock, frameBuf);
							sendBlock(frameBuf, 10);
						} else {
							errMsg(17);
						}
					} else {
						errMsg(18);
					}
					brightness(0);
					break;

#ifdef DEBUG
				case ('s'):
					for (i = 0; i < 255; i++) {
						frameBuf[0] = i;
						usi_twi_master_start();
						if (usi_twi_master_transmit(frameBuf, 1)) {
							sendByte(' ');
							sendNum(i, 16);
							j = 0;
						}
						usi_twi_master_stop();
					}
					break;
#endif
				case ('t'):					// a test function
					sendCRLF();
					sendString(":");
					int itest = recvNum();
					sendNum(itest, 10);
					break;

				case ('w'):
					writeMiFare(frameBuf);
					break;

				default:
					sendByte('?');
					break;

			}
			sendPrompt();
		}
	}
}

void sendBlock(uint8_t frameBuf[], uint8_t base)
{

	uint8_t i;

	if (base == 16) {
		for (i = 0; i < 16; i++) {
			sendNum(frameBuf[i+10], 16);
		}
	} else {
		for (i = 0; i < 16; i++) {
			sendByte(frameBuf[i+10]);
		}
	}

}

uint8_t writeMiFare(uint8_t frameBuf[])
{

	uint8_t cardID[5], buf[32];
	uint8_t i;

	if (! pn532_twi_sendCommand(INLISTPASSIVETARGET, frameBuf, 0)) {
		errMsg(21);
		return(FALSE);
	}
	if (frameBuf[9]) {
		for (i = 0; i < 4; i++) {
			cardID[i] = frameBuf[i+15];
		}
	} else {
		errMsg(23);
		return(FALSE);
	}

	for (i=0; i<32; i++) {
		while (!COMMANDSENT) {
		}
		buf[i] = UDR;
		sendByte(buf[i]);
		if (buf[i] == '\r') {
			sendByte('\n');
			break;
		}
	}
	for (; i<32; i++) {
		buf[i] = ' ';
	}
	if (! pn532_writeBlock(cardID, 1, buf, frameBuf)) {
		errMsg(19);
		return(FALSE);
	}
	if (! pn532_writeBlock(cardID, 2, &buf[15], frameBuf)) {
		errMsg(20);
		return(FALSE);
	}
	return(TRUE);
}

/*
	recvNum() accepts characters from the serial port and returns the twos complement
	16-bit integer. It echos the characters as they're typed and concludes when a
	carriage return is entered.

	Errors: If more than five (5) characters without sign are entered, or if more
	than six (6) characters with a leading + or - sign are entered, this routine
	sends back a question mark (?), CRLF, and returns zero (0).

*/

uint16_t recvNum(void)
{

	char strBuf[7];
	uint8_t i = 0;

	for (;;) {
		while (!COMMANDSENT) {
			asm("nop");
		}
		strBuf[i] = UDR;
		sendByte(strBuf[i]);
		if (strBuf[i] == '\r') {
			strBuf[i] = 0x00;
			sendCRLF();
			break;
		}
		if (i == 5) {
			if ((strBuf[0] != '+') && (strBuf[0] != '-')) {
				sendCRLF();
				sendByte('?');
				return(0);
			}
		}
		if (i == 6) {
			sendCRLF();
			sendByte('?');
			return(0);
		}
		i++;
	}

	return(atoi(strBuf));

}

void sendByte(uint8_t x)
{

	while (!TXREADY) {
		asm("nop");
	}

	UDR = x;

}

void sendCRLF(void)
{

	sendByte('\n');
	sendByte('\r');

}

void sendNum(int number, uint8_t base)
{

	char strBuf[7];

	if (base == 16) {
		if (number <= 0x0f) {					// send leading '0' for hex
			sendByte('0');
		}
	}
	sendString(itoa(number, strBuf, base));

}

void sendPrompt(void)
{

	sendCRLF();
	sendByte('>');

}

void sendString(char str[])
{

	uint8_t i = 0;

	while (str[i]) {
		sendByte(str[i++]);
	}

}

/*
	errMsg()
	1 - mcp23008_Init() fail
	2 - GETFIRMWAREVERSION fail
	3 - PN532 send cmd with ack fail
	4 - Receive ack fail in send cmd with ack command
	5 - Receive ack timeout
	6 - Firmware version mismatch
	7 - Send cmd ack fail in authenticate block
	8 - authenticate block reply error
	9 - Send cmd ack fail in read block
	10- readblock reply error
	11- send cmd with ack fail in write block
	12- bad reply in write block
	13- Firmware version mismatch
	14- mcp23008 write error
	15- SAMCONFIG error
	16- RFCONFIG error
	17- card read error
	18- No card in field
	19- Error writing block 1
	20- Error writing block 2
	22- PN532 IRQ sent (not error)

*/

void errMsg(uint8_t msgNum)
{

	sendCRLF();
	sendByte('E');
	sendNum(msgNum, 10);

}


/*
===========================================================================

	UBRRL and UBRRH USART Baud Rate Registers. 12 bits, computed from:
	
	UBRR = (F_CPU / (16 * Baudrate)) - 1 where "Baudrate" is the common
	understanding.

===========================================================================

	UCSRB Control STatus Register B
	---------------------------------------------------------------------
	Bit	|	7	|	6	|	5	|	4	|	3	|	2	|	1	|	0	|
	Item| RXCIE	| TXCIE	| UDRIE	|  RXEN	|  TXEN	| UCSZ2	|  RXBB	|  TXBB	|
	Set	|	0	|	0	|	0	|	1	|	1	|	0	|	0	|	0	|
	---------------------------------------------------------------------

	RXCIE is RX Complete Interrupt Enable (we won't use this)
	TXCIE is the TX Complete Interrupt ENable
	UDRIE is USART Data Register Empty Interrupt Enable
	RXEN is Receiver Enable
	TXEN is Transmitter Enable
	UCSZ2, combined with UCSZ[1:0] in UCSRC sets the number of data bits
	RXB8 Receive Data Bit 8 (ninth bit of data, if present)
	TXB8 Transmit bit 8 (ninth bit of data)

	UCSRB = 0b00011000

===========================================================================

	UCSRC Control STatus Register C
	---------------------------------------------------------------------
	Bit	|	7	|	6	|	5	|	4	|	3	|	2	|	1	|	0	|
	Item|UMSEL1	|UMSEL0	| UPM1	| UPM0	| USBS	| UCSZ1	| UCSZ0	| UCPOL	|
	Set	|	0	|	0	|	0	|	0	|	0	|	1	|	1	|	0	|
	---------------------------------------------------------------------

	UMSEL[1:0] is the USART mode select. We want 0b00, Asynchronous UART
	UPM[1:0] is Parity mode. 0b00 is disabled, 0b01 reserved, 0b10 Enabled,
	even parity, 0b11 Enabled, odd parity.
	USBS is Stop Bit Select (how many stop bits). 0->1-bit, 1->2-bits.
	UCSZ[1:0] is the rest of the character size description. An 8-bit
	situation says both of these bits should be high.
	UCPOL is clock polarity. Changes fall/rising edges.

	UCSRC = 0b00000110

===========================================================================

	TCCR0A (Timer/Counter Control Register A for Timer0) settings
	---------------------------------------------------------------------
	Bit	|	7	|	6	|	5	|	4	|	3	|	2	|	1	|	0	|
	Item| COM0A1| COM0A0| COM0B1| COM0B0|	-	|	-	| WGM01	| WGM00	|
	Set	|	1	|	0	|	0	|	0	|	0	|	0	|	0	|	1	|
	---------------------------------------------------------------------

	For phase-correct PWM with TOP=OCR0A, WGM0[2:0] should be 0b001 (WGM0[2]
	is in TCCR0B). In phase correct PWM mode the timer counts up, then down.

	We're only using one compare register here so COM0B[1:0] is 0b00 and
	COM0A[1:0] is 0b10, which sets OC0A on the way up and clears it on the
	way down.

	TCCR0A = 0b10000001

===========================================================================

	TCCR0B (Timer/Counter Control Register B for Timer0) settings
	---------------------------------------------------------------------
	Bit	|	7	|	6	|	5	|	4	|	3	|	2	|	1	|	0	|
	Item| FOC0A	| FOC0B	|	-	|	-	| WGM02	|  CS02	|  CS01	|  CS00	|
	Set	|	0	|	0	|	0	|	0	|	0	|	0	|	1	|	0	|
	---------------------------------------------------------------------

	CS0[2:0] are the clock select bits:

		0b000	No clock source (timer stopped)
		0b001	clk/1 (no prescaling)
		0b010	clk/8
		0b011	clk/64
		0b100	clk/256
		0b101	clk/1024
		0b110	External clock on T0 pin, falling edge
		0b111	External clock on T0 pin, rising edge

	The PWM frequency is clk/(N*510) where N is the prescale factor. If
	our nominal CPU clock frequency is 14.745600 MHz, the frequency and
	period for the prescaled timer are are:

		======================================
		CS0[2:0]	Frequency		Period
		--------------------------------------
		0b001		28.913 kHz		 34.6 us
		0b010		 3.614 kHz		276.7 us
		0b011	   452.765 Hz		  2.214 ms
		0b100	   112.941 Hz		  8.854 ms
		0b101	    28.235 Hz		 35.417 ms
		======================================

	The Faulhaber 2622 B-SC motor takes a PWM frequency input in the range
	of 500 Hz to 18 kHz. Let's choose 3.614 kHz. The motor stops turning at
	duty cycle < 2.0% (5/255) and starts turning at duty cycle > 3.0% (8/255).

	For phase-correct PWM, WGM0[2:0] (Wavform Generation Mode)
	should be 0b001 if we're using OC0A (setting WGM0[2] to "1" means that
	OC0A becomes TOP and OC0B is the compared register). WGM0[1:0] are in
	TCCR0A.

	The Force Output Compare (FOC0A and FOC0B bits) must be set to zero (0)
	when using the timer in PWM mode.

	TCCR0B = 0b00000010

===========================================================================

	MCUSR MCU Status Register (what caused the MCU Reset?)
	---------------------------------------------------------------------
	Bit	|	7	|	6	|	5	|	4	|	3	|	2	|	1	|	0	|
	Item|   -	|   -	|	-	|	-	| WDRF	|  BORF	| EXTRF	|  PORF	|
	Set	|	0	|	0	|	0	|	0	|	0	|	0	|	1	|	0	|
	---------------------------------------------------------------------

	WDRF: Watchdog Reset Flag is set if a WD Reset occurs. Reset by power
	on or writing zero to the flag.
	
	BORF: Brown-out Reset flag is set if a brown-out Reset happens.
	
	EXTRF: External Reset Flag
	
	PORF: Power-on Reset Flag

	NB: THe WDRF *MUST* be reset or the system will immediately reset and
	loop forever.

===========================================================================

	WDTCR Watchdog Timer Control Register (aka WDTCSR on ATtiny2313)
	128kHz oscillator
	---------------------------------------------------------------------
	Bit	|	7	|	6	|	5	|	4	|	3	|	2	|	1	|	0	|
	Item| WDIF	| WDIE	| WDP3	| WDCE	|  WDE	| WDP2	| WDP1	|  WDP0	|
	Set	|	0	|	0	|	0	|	0	|	0	|	0	|	1	|	0	|
	---------------------------------------------------------------------

	WDIF: Watchdog Timeout Interrupt Flag is set when a timeout occurs
	and the WDT is configured for interrupt.
	WDIE: Watchdog Timeout Interrupt Enable.
	WDP3: One of three prescaler bits (see below)
	WDCE: Watchdog Change Enable (must be set to change WDE)
	WDE: Watchdog enable
	WDP[3:0]: Timer prescaler bits

		=========================================
		WDP3	WDP2	WDP1	WDP0	Appx time
		-----------------------------------------
		 0		 0		 0		 0		16 ms
		 0		 0		 0		 1		32 ms
		 0		 0		 1		 0		64 ms
		 0		 0		 1		 1		0.125 s
		 0		 1		 0		 0		0.25 s
		 0		 1		 0		 1		0.5 s
		 0		 1		 1		 0		1.0 s
		 0		 1		 1		 1		2.0 s
		 1		 0		 0		 0		4.0 s
		 1		 0		 0		 1		8.0 s
		=========================================

	Recovering from a WDT reset is a little tricky. The steps:
		1. Clear the MCUSR watchdog reset flag (WDRF).
		2. In one operation, write logic one to both WDCE and WDE.
		3. In the very next line, write a logic 0 to WDE.


===========================================================================

*/

#define BAUDRATE 19200
#define MYUBRR ((F_CPU / 16 / BAUDRATE) -1)
#define WATCHDOG	1
#define POWERON		2
#define PUSHBUTTON	3
#define BROWNOUT	4

void init_ATtiny4313(void)
{

	uint8_t junk, resetType = 0;

	if (MCUSR & _BV(WDRF)) {	// If there was a watchdog reset...
		MCUSR = 0x00;			// Clear the WDRF flag (and everything else)
		WDTCR = 0x18;			// WDCE must be high to change WDE (WDEnable)
		WDTCR = 0x00;			// Now clear WDE
		resetType = WATCHDOG;
	} else if (MCUSR & _BV(PORF)) {
		resetType = POWERON;
	} else if (MCUSR & _BV(EXTRF)) {
		resetType = PUSHBUTTON;
	} else if (MCUSR & _BV(BORF)) {
		resetType = BROWNOUT;
	}
	MCUSR = 0x00;

	UBRRH = (uint8_t) (MYUBRR >> 8);	// Set baud rate
	UBRRL = (uint8_t) MYUBRR;
	UCSRB = 0b00011000;			// Enable transmit & receive
	UCSRC = 0b00000110;			// 8 data bits, no parity, one stop bit

	TCCR0A = 0b10000001;		// Phase-correct PWM on OC0A (see above)
	TCCR0B = 0b00000010;		// Select clock prescaler /8 (see above)

	DDRB  |= _BV(LEDPIN);		// LEDPIN is an output
	PORTD |= _BV(INTPIN);		// Pullup

	while (COMMANDSENT) {		// Clear the serial port
		junk = UDR;
	}

	switch (resetType) {
		case (WATCHDOG):
			sendString("S");
			break;
		case (BROWNOUT):
			sendString("BO");
			break;
		case (POWERON):
			sendString("UP");
			break;
		case (PUSHBUTTON):
			sendString("PB");
			break;
		default:
			break;
		}

	brightness(255);
	_delay_ms(100);
	brightness(0);

}
