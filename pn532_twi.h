#include <avr/io.h>

#define PN532_TWI_ADDRESS 	0x24
#define GETFIRMWAREVERSION	0x02
#define SAMCONFIG			0x14
#define INLISTPASSIVETARGET 0x4A
#define RFCONFIGURATION 	0x32
#define INDATAEXCHANGE		0x40
#define AUTHENTICATE_A		0x60
#define READBLOCK			0x30
#define WRITEBLOCK			0xA0

#define INTPIN				PD2

// GPIOR0 bit positions for error flags
#define RECVACK				0x01
#define FWVER				0x02
#define AUTHBLK				0x03
#define READBLK				0x04
#define WRITEBLK			0x05
#define SENDCMDACK			0x06
#define INITPN532			0x07

uint8_t pn532_twi_sendCommand(uint8_t, uint8_t*, uint8_t);
uint8_t pn532_buildFrame(uint8_t, uint8_t*, uint8_t*);
void pn532_twi_recvACK(uint8_t*);
uint8_t pn532_recvAck(uint8_t*);
uint8_t pn532_statusByte(void);
uint8_t pn532_notReady(void);
uint8_t usi_twi_master_recvByte(void);
uint8_t pn532_twi_sendCmdAck(uint8_t*, uint8_t);
uint8_t pn532_authenticateBlock(uint8_t*, uint8_t, uint8_t*);
uint8_t pn532_readBlock(uint8_t*, uint8_t, uint8_t*);
uint8_t pn532_writeBlock(uint8_t*, uint8_t, uint8_t*, uint8_t*);
