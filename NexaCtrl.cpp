/**
 * NexaCtrl Library v3 - Nexa/HomeEasy control library with absolute dim support.
 * Original author of this library is James Taylor (http://jtlog.wordpress.com/)
 *
 * v2 by Carl Gunnarsson - refactoring and adding support for absolute dimming
 * v3 by Joe Lippa - made compatible with the ESP8266 arduino core + refactoring
 */

#include "NexaCtrl.h"

/*
 * constructor
 */
ESP8266_FLASH_ATTR NexaCtrl::NexaCtrl(
		unsigned int tx_pin,
		unsigned long controller_id,
		int transmitCount,
		unsigned int led_pin) {

	_tx_pin = tx_pin;
	pinMode(_tx_pin, OUTPUT);

	_controller_id = controller_id != 0 ? controller_id : NexaCtrl::CONTROLLER_ID;
	_transmitCount = transmitCount;

	if (led_pin != 0) {
		_led_pin = led_pin;
		pinMode(_led_pin, OUTPUT);
	}

	// PULSE_LENGTH + DIM_LENGTH because we need room for dim bits if
	// we want to transmit a dim signal
	low_pulse_array = (int*) calloc((NexaCtrl::PULSE_LENGTH + (2 * NexaCtrl::DIM_LENGTH)), sizeof(int));
}

/*
 * primary switch on/off public interface method
 */
void ESP8266_FLASH_ATTR NexaCtrl::Switch(unsigned int device_id, uint8_t value) {
	switch(value){
		case LOW:
			DeviceOff(device_id);
			break;
		case HIGH:
			DeviceOn(device_id);
			break;
		default:
		    DeviceDim(device_id, value);
		    break;
	}
}

void ESP8266_FLASH_ATTR NexaCtrl::DeviceOff(unsigned int device_id) {
	SetControllerBits();
	SetBit(NexaCtrl::GROUP_FLAG_OFFSET, 0);
	SetBit(NexaCtrl::COMMAND_FLAG_OFFSET, 0);
	SetDeviceBits(device_id);
	Transmit(NexaCtrl::PULSE_LENGTH);
}

void ESP8266_FLASH_ATTR NexaCtrl::DeviceOn(unsigned int device_id) {
	SetControllerBits();
	SetBit(NexaCtrl::GROUP_FLAG_OFFSET, 0);
	SetBit(NexaCtrl::COMMAND_FLAG_OFFSET, 1);
	SetDeviceBits(device_id);
	Transmit(NexaCtrl::PULSE_LENGTH);
}

void ESP8266_FLASH_ATTR NexaCtrl::DeviceDim(unsigned int device_id, unsigned int dim_level) {

	SetControllerBits();

	SetBit(NexaCtrl::GROUP_FLAG_OFFSET, 0);

	// Specific for dim
	low_pulse_array[(NexaCtrl::COMMAND_FLAG_OFFSET * 2)] = NexaCtrl::PULSE_LOW;
	low_pulse_array[(NexaCtrl::COMMAND_FLAG_OFFSET * 2) + 1] = NexaCtrl::PULSE_LOW;

	SetDeviceBits(device_id);

	// Normally, allowed input would be 0-15, but 0-100 makes more sense.
	dim_level *= (15 / 100);

	bool dim_bits[NexaCtrl::DIM_LENGTH];
	itob(dim_bits, dim_level, NexaCtrl::DIM_LENGTH);

	for (int bit = 0; bit < NexaCtrl::DIM_LENGTH; bit++) {
		SetBit(NexaCtrl::DIM_OFFSET + bit, dim_bits[bit]);
	}
	Transmit(NexaCtrl::PULSE_LENGTH + (NexaCtrl::DIM_LENGTH * 2));
}

void ESP8266_FLASH_ATTR NexaCtrl::GroupOn() {
	unsigned int device_id = 0;
	SetControllerBits();
	SetDeviceBits(device_id);
	SetBit(NexaCtrl::GROUP_FLAG_OFFSET, 1);
	SetBit(NexaCtrl::COMMAND_FLAG_OFFSET, 1);
	Transmit(NexaCtrl::PULSE_LENGTH);
}

void ESP8266_FLASH_ATTR NexaCtrl::GroupOff() {
	unsigned int device_id = 0;
	SetControllerBits();
	SetDeviceBits(device_id);
	SetBit(NexaCtrl::GROUP_FLAG_OFFSET, 1);
	SetBit(NexaCtrl::COMMAND_FLAG_OFFSET, 0);
	Transmit(NexaCtrl::PULSE_LENGTH);
}

// private methods
void ESP8266_FLASH_ATTR NexaCtrl::SetDeviceBits(unsigned int device_id) {
	bool device[NexaCtrl::DEVICE_ID_LENGTH];
	unsigned long ldevice_id = device_id;
	itob(device, ldevice_id, NexaCtrl::DEVICE_ID_LENGTH);
	for (int bit = 0; bit < NexaCtrl::DEVICE_ID_LENGTH; bit++) {
		SetBit(NexaCtrl::DEVICE_ID_OFFSET + bit, device[bit]);
	}
}

void ESP8266_FLASH_ATTR NexaCtrl::SetControllerBits() {
	bool controller[NexaCtrl::CONTROLLER_ID_LENGTH];
	itob(controller, _controller_id, NexaCtrl::CONTROLLER_ID_LENGTH);

	for (int bit = 0; bit < NexaCtrl::CONTROLLER_ID_LENGTH; bit++) {
		SetBit(bit, controller[bit]);
	}
}

void ESP8266_FLASH_ATTR NexaCtrl::SetBit(unsigned int bit_index, bool value) {
	// Each actual bit of data is encoded as two bits on the wire...
	if (!value) {
		// Data 0 = Wire 01
		low_pulse_array[(bit_index * 2)] = NexaCtrl::PULSE_LOW;
		low_pulse_array[(bit_index * 2) + 1] = NexaCtrl::PULSE_LOW_2;
	} else {
		// Data 1 = Wire 10
		low_pulse_array[(bit_index * 2)] = NexaCtrl::PULSE_LOW_2;
		low_pulse_array[(bit_index * 2) + 1] = NexaCtrl::PULSE_LOW;
	}
}

void ESP8266_FLASH_ATTR NexaCtrl::itob(bool *bits, unsigned long integer, int length) {
	for (int i = 0; i < length; i++) {
		if ((integer / power2(length - 1 - i)) == 1) {
			integer -= power2(length - 1 - i);
			bits[i] = 1;
		} else {
			bits[i] = 0;
		}
	}
}

unsigned long ESP8266_FLASH_ATTR NexaCtrl::power2(int power) { //gives 2 to the (power)
	return (unsigned long) 1 << power;
}

void NexaCtrl::TransmitLatch(void) {
	// high for a moment
	digitalWrite(_tx_pin, HIGH);
	delayMicroseconds(NexaCtrl::PULSE_LOW);
	// low for 2675
	digitalWrite(_tx_pin, LOW);
	delayMicroseconds(2675);
}

void NexaCtrl::Transmit(int pulse_length) {

    for (int transmit_count = 0; transmit_count < _transmitCount; transmit_count++) {

        {
            //timing critical start
#ifdef ARDUINO_ARCH_ESP8266
            InterruptLock lock;
#else
            noInterrupts();
#endif
            if (_led_pin > 0) {
                digitalWrite(_led_pin, HIGH);
            }

            TransmitLatch();

            /*
             * Transmit the actual message
             * every wire bit starts with the same short high pulse, followed
             * by a short or long low pulse from an array of low pulse lengths
             */
            for (int pulse_count = 0; pulse_count < pulse_length; pulse_count++) {
                digitalWrite(_tx_pin, HIGH);
                delayMicroseconds(NexaCtrl::PULSE_HIGH);
                digitalWrite(_tx_pin, LOW);
                delayMicroseconds(low_pulse_array[pulse_count]);
            }

            TransmitLatch();

            if (_led_pin > 0) {
                digitalWrite(_led_pin, LOW);
            }
#ifndef ARDUINO_ARCH_ESP8266
            interrupts();
#endif
        }

        delayMicroseconds(9500);
        yield();
    }
}

