This program controls an Adafruit PN532 RFID/NFC Shield v1.3 from an ATtiny4313.

The target operation is automated filter identification in a telescope filter wheel. Filter holders will have MiFare Classic tags permanently attached. The tags will hold filter identification information.

The Adafruit Shield (not their PN532 breakout board) has built-in 5V to 3.3V level shifters that let us run the PN532 from a 5V environment. I want to run the ATtiny4313 at 14.7456 MHz, which requires a 5V supply. 

The code is written for avr-gcc and has those quirks.
