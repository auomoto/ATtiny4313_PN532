#include "pca9543.h"
#include "usi_twi_master.h"

void pca9543SelectChannel(uint8_t channel);
uint8_t pca9543InterruptChannel(void);

void pca9543SelectChannel(uint8_t channel)
{

	uint8_t cmdArray[2];

	cmdArray[0] = (PCA9543_BASEADDR << 1);
	cmdArray[1] = (1 << channel);

	usi_twi_master_start();
	usi_twi_master_transmit(cmdArray, 2);
	usi_twi_master_stop();

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

	uint8_t cmdArray[2];

	cmdArray[0] = ((PCA9543_BASEADDR << 1) | 0x01);
	usi_twi_master_start();
	usi_twi_master_receive(cmdArray, 2);
	usi_twi_master_stop();

	switch (cmdArray[1] & 0x30) {

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