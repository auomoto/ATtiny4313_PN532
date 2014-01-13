#include "usi_twi_master.h"
#include "pn532_twi.h"
#include "pca9543.h"

uint8_t pn532_authenticateBlock(uint8_t cardID[], uint8_t blockNum, uint8_t frameBuf[])

{
	uint8_t i, len, command[15];

	command[0] = INDATAEXCHANGE;
	command[1] = 0x01;				// Target number
	command[2] = AUTHENTICATE_A;	// MiFare card key "A"
	command[3] = blockNum;
	for (i = 0; i < 6; i++) {
		command[4+i] = 0xFF;		// Authenticate key A
	}
	for (i = 0; i < 4; i++) {
		command[10+i] = cardID[i];
	}
	len = pn532_buildFrame(15, command, frameBuf);
	if (pn532_twi_sendCmdAck(frameBuf, len) == FALSE) {
		GPIOR0 |= _BV(AUTHBLK);
		return(FALSE);
	}
	while (pn532_notReady()) {
	}
	frameBuf[0] = 0x49;
	usi_twi_master_start();
	usi_twi_master_receive(frameBuf, 14);
	usi_twi_master_stop();
	if (frameBuf[9]) {
		GPIOR0 |= _BV(AUTHBLK);
		return(FALSE);
	}
	GPIOR0 &= ~_BV(AUTHBLK);
	return(TRUE);

}

uint8_t pn532_readBlock(uint8_t cardID[], uint8_t blockNum, uint8_t frameBuf[])
{

	uint8_t len, command[4];

	if (pn532_authenticateBlock(cardID, blockNum, frameBuf) == FALSE) {
		return(FALSE);
	}
	command[0] = INDATAEXCHANGE;
	command[1] = 0x01;				// Target number
	command[2] = READBLOCK;			// MiFare card key read 16 bytes
	command[3] = blockNum;
	len = pn532_buildFrame(5, command, frameBuf);
	if (pn532_twi_sendCmdAck(frameBuf, len) == FALSE) {
		GPIOR0 |= _BV(READBLK);
		return(FALSE);
	}
	while (pn532_notReady()) {
	}
	frameBuf[0] = 0x49;
	usi_twi_master_start();
	usi_twi_master_receive(frameBuf, 26);
	usi_twi_master_stop();
	if (frameBuf[9] != 0x00) {
		GPIOR0 |= _BV(READBLK);
		return(FALSE);
	} else {
		GPIOR0 &= ~_BV(READBLK);
		return(TRUE);
	}
}

uint8_t pn532_writeBlock(uint8_t cardID[], uint8_t blockNum, uint8_t data[], uint8_t frameBuf[])
{

	uint8_t i, len, command[25];

	if (pn532_authenticateBlock(cardID, blockNum, frameBuf) == FALSE) {
		GPIOR0 |= _BV(WRITEBLK);
		return(FALSE);
	}
	command[0] = INDATAEXCHANGE;
	command[1] = 0x01;				// Target number
	command[2] = WRITEBLOCK;		// MiFare card key write 16 bytes
	command[3] = blockNum;
	for (i = 0; i < 16; i++) {
		command[i+4] = data[i];
	}
	len = pn532_buildFrame(21, command, frameBuf);
	if (pn532_twi_sendCmdAck(frameBuf, len) == FALSE) {
		GPIOR0 |= _BV(WRITEBLK);
		return(FALSE);
	}

	while (pn532_notReady()) {
	}
	frameBuf[0] = 0x49;
	usi_twi_master_start();
	usi_twi_master_receive(frameBuf, 29);
	usi_twi_master_stop();
	if (frameBuf[9] != 0x00) {
		GPIOR0 |= _BV(WRITEBLK);
		return(FALSE);
	} else {
		GPIOR0 &= ~_BV(WRITEBLK);
		return(TRUE);
	}

}

/*

	Build a pn532 command frame. Does not read or write to the TWI bus.

	len is the command length, the count of tfi byte + command byte + data.

	command[] contains the command (zeroth byte) and following data.

	framebuf[] is the returned command array. framebuf[0] is always 0x48,
	the PN532 write address. If you want a read, add 1 (one) to framebuf[0]
	after this to set the read bit (but this never happens?).
	
	Returns the length of framebuf[].

*/
uint8_t pn532_buildFrame(uint8_t len, uint8_t command[], uint8_t framebuf[])
{

	uint8_t i, j;
	uint16_t temp;

	framebuf[0] = 0x48;						// pn532 address (write)
	framebuf[1] = 0x00;						// preamble
	framebuf[2] = 0x00;						// start of frame
	framebuf[3] = 0xff;						// start of frame
	framebuf[4] = len;						// commmand length
	framebuf[5] = ~len + 1;					// lcs (packet length checksum)
	framebuf[6] = 0xd4;						// tfi (frame identifier)
	temp = 0xd4;							// build the data checksum
	j = 7;
	for (i = 0; i < len-1; i++) {
		temp += command[i];
		framebuf[j++] = command[i];
	}
	framebuf[j++] = ~(temp & 0xff) + 1;	// dcs
	framebuf[j++] = 0x00;					// postamble
	return(j);

}

/*

	Puts a PN532 command on the TWI bus. This always initiates a start
	condition and always ends with a stop condition.

	cmd is the PN532 command. This routine knows the data required for
	each command.

	framebuf[] is a scratch array supplied by the calling routine.

	Returns FALSE if the PN532 does not ACK, either in the TWI sense
	or the PN532 sense. Returns FALSE if the command isn't available.

*/

uint8_t pn532_twi_sendCommand(uint8_t cmd, uint8_t frameBuf[], uint8_t blockNum)
{

	uint8_t firmwareVersion[] = {0x49, 0x01, 0x00, 0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03, 0x32, 0x01, 0x06, 0x07, 0xE8, 0x00};
	uint8_t i, frameLen, command[24];

	switch (cmd) {
	
		case (GETFIRMWAREVERSION):
			command[0] = GETFIRMWAREVERSION;
			frameLen = pn532_buildFrame(2, command, frameBuf);
			if (pn532_twi_sendCmdAck(frameBuf, frameLen) == FALSE) {
				return(FALSE);
			}

			while (pn532_notReady()) {
			}
			frameBuf[0] = 0x49;
			usi_twi_master_start();
			usi_twi_master_receive(frameBuf, 15);
			usi_twi_master_stop();

			for (i = 0; i < 15; i++) {
				if (firmwareVersion[i] != frameBuf[i]) {
					return(FALSE);
				}

			}

			break;
			
		case (SAMCONFIG):
			command[0] = SAMCONFIG;
			command[1] = 0x01;
			frameLen = pn532_buildFrame(3, command, frameBuf);

			if (pn532_twi_sendCmdAck(frameBuf, frameLen) == FALSE) {
				return(FALSE);
			}
			while (pn532_notReady()) {
			}
			frameBuf[0] = 0x49;
			usi_twi_master_start();
			usi_twi_master_receive(frameBuf, 11);
			usi_twi_master_stop();

			break;

		case (INLISTPASSIVETARGET):
			command[0] = INLISTPASSIVETARGET;
			command[1] = 0x01;		// MaxTg maximum # cards
			command[2] = 0x00;		// baud rate code; 0 -> Type A card
			frameLen = pn532_buildFrame(4, command, frameBuf);
			if (pn532_twi_sendCmdAck(frameBuf, frameLen) == FALSE) {
				return(FALSE);
			}
			while (pn532_notReady()) {
			}
			frameBuf[0] = 0x49;
			usi_twi_master_start();
			usi_twi_master_receive(frameBuf, 21);
			usi_twi_master_stop();
			break;

		case (RFCONFIGURATION):		// to set max retries
			command[0] = 0x32;
			command[1] = 0x05;
			command[2] = 0xFF;
			command[3] = 0x01;
			command[4] = 0x01;		// MxRtyPassiveActivation
			frameLen = pn532_buildFrame(6, command, frameBuf);
			if (pn532_twi_sendCmdAck(frameBuf, frameLen) == FALSE) {
				return(FALSE);
			}
			while (pn532_notReady()) {
			}
			frameBuf[0] = 0x049;
			usi_twi_master_start();
			usi_twi_master_receive(frameBuf, 21);
			usi_twi_master_stop();
			break;

		default:
			return (FALSE);

	}
	return(TRUE);
}


uint8_t pn532_twi_sendCmdAck(uint8_t frameBuf[], uint8_t frameLen)
{

	usi_twi_master_start();

	if (usi_twi_master_transmit(frameBuf, frameLen) == FALSE) {
		usi_twi_master_stop();
		GPIOR0 |= _BV(SENDCMDACK);
		return(FALSE);
	}
	usi_twi_master_stop();

	if (pn532_recvAck(frameBuf) == FALSE) {
		GPIOR0 |= _BV(SENDCMDACK);
		return(FALSE);
	}

	GPIOR0 &= ~_BV(SENDCMDACK);
	return(TRUE);
}

/*

	Receives and verifies the PN532 acknowledge frame.

*/
uint8_t pn532_recvAck(uint8_t frameBuf[])
{

	const uint8_t ackFrame[] = {0x49, 0x01, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00};
	uint8_t i;

	i = 0;
	while (pn532_notReady()) {
		i++;
		_delay_us(100);
		if (i > 127) {
			GPIOR0 |= _BV(RECVACK);
			return(FALSE);
		}
	}

	frameBuf[0] = 0x49;
	usi_twi_master_start();
	usi_twi_master_receive(frameBuf, 8);
	usi_twi_master_stop();

	for (i = 0; i < 8; i++) {
		if (frameBuf[i] != ackFrame[i]) {
			GPIOR0 |= _BV(RECVACK);
			return(FALSE);
		}
	}

	GPIOR0 &= ~_BV(RECVACK);
	return (TRUE);

}

/*
	Check the interruput pin to see if the PN532 is ready to send data.
	The logic is backwards here; don't get confused.
*/
uint8_t pn532_notReady()
{

	if ((PIND & _BV(INTPIN))) {			// No interrupt from the card
		return(TRUE);
	}

	if (pca9543InterruptChannel() != (GPIOR0 & 0x01)) {	// Wrong card
		return (TRUE);
	}

	return(FALSE);

}
