This program controls an Adafruit PN532 RFID/NFC Shield v1.3 from an ATtiny4313.

The target operation is automated filter identification in a telescope filter wheel. Filter holders will have MiFare Classic tags permanently attached. The tags will hold filter identification information.

The code is written for avr-gcc.

To-do:
	o Add second controller
	o Add I2C bus switch
	o Add RS-422 transceiver and tie the handshaking lines

