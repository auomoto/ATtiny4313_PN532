#include <string.h>				// memset()
#include "pca9543.h"
#include "usi_twi_master.h"

/*
	Selects the channel (0 or 1) that the PCA9543
	I2C bus switch will connect. The PCA9543 will
	allow both channels but we won't permit that.
*/
void pca9543SelectChannel(uint8_t channel)
{

	uint8_t cmdArray[2];

	cmdArray[0] = (PCA9543_BASEADDR << 1);
	cmdArray[1] = (1 << channel);

	usi_twi_master_start();
	usi_twi_master_transmit(cmdArray, 2);
	usi_twi_master_stop();

	if (channel == 1) {
		GPIOR0 |= 0x01;
	} else {
		GPIOR0 &= 0xFE;
	}

}

/*
	Returns the flagged interrupt channel. Possible replies are:
	0 - Only channel 0 is flagged
	1 - Only channel 1 is flagged
	2 - Both channels flagged
	3 - Neither channel flagged
*/
uint8_t pca9543InterruptChannel()
{

	uint8_t reg;

	reg = pca9543ControlRegister();

	switch (reg & 0x30) {

		case (0x10):
			return(0);
			break;

		case (0x20):
			return(1);
			break;

		case (0x30):
			return(2);
			break;

		default:
			return(3);

	}

}

/*
	Returns the pca9543 control register
*/

uint8_t pca9543ControlRegister()
{

	uint8_t cmdArray[2];

	cmdArray[0] = ((PCA9543_BASEADDR << 1) | 0x01);
	usi_twi_master_start();
	usi_twi_master_receive(cmdArray, 2);
	usi_twi_master_stop();

	return(cmdArray[1]);

}