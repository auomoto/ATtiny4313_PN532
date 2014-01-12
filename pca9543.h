/*
	The PCA9543 is an I2C bus switch. We're using it here to
	select between two I2C items (PN532 NFC reader) that does
	not have selectable I2C addresses.
*/

//#include <avr/io.h>
#include <stdint.h>
#define PCA9543_BASEADDR 0x70		// A0 and A1 grounded. Shift left one and add r/w bit.

void pca9543SelectChannel(uint8_t);
uint8_t pca9543InterruptChannel(void);
uint8_t pca9543ControlRegister(void);
