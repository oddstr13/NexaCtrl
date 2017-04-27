#ifndef NexaCtrl_h
#define NexaCtrl_h

#include <Arduino.h>
#ifdef ARDUINO_ARCH_ESP8266
#include "interrupts.h"
#define ESP8266_FLASH_ATTR ICACHE_FLASH_ATTR
#else
#define ESP8266_FLASH_ATTR
#endif

class NexaCtrl
{
    private:
        int _tx_pin;
        int _led_pin;
        unsigned long _controller_id;
        int _transmitCount;
        int* low_pulse_array;

        static constexpr const unsigned long CONTROLLER_ID = 1234567;

        /*
         * critical pulse time interval lengths
         * v2 defines these intervals as 275,275,1225
         *
         * v3 has modified versions of these intervals following testing and comparison against a real world HE-100 remote control
         * HE-100 over the air pulses were captured with a Realtek RTL2832U SDR USB dongle
         * The goal was to replicate the signals sent from the HE-100 remote control as closely as possible
         */
        static constexpr const int PULSE_HIGH = 270;
        static constexpr const int PULSE_LOW = 280;
        static constexpr const int PULSE_LOW_2 = 1240;

        static constexpr const int PULSE_LENGTH = 64;

        /*
         * The actual message is 32 / 36 bits of data:
         * bits 0-25: the controllerId.
         * bit 26: group flag
         * bit 27: on/off/dim flag
         * bits 28-31: the device code - 4bit number.
         * bits 32-35 (optional): the dim level - 4bit number.
         */
        static constexpr const int CONTROLLER_ID_LENGTH = 26;
        static constexpr const int GROUP_FLAG_OFFSET = 26;
        static constexpr const int COMMAND_FLAG_OFFSET = 27;
        static constexpr const int DEVICE_ID_OFFSET = 28;
        static constexpr const int DEVICE_ID_LENGTH = 4;
        static constexpr const int DIM_OFFSET = 32;
        static constexpr const int DIM_LENGTH = 4;

        void ESP8266_FLASH_ATTR SetBit(unsigned int bit_index, bool value);
        void ESP8266_FLASH_ATTR SetDeviceBits(unsigned int device_id);
        void ESP8266_FLASH_ATTR SetControllerBits();

        void ESP8266_FLASH_ATTR itob(bool *bits, unsigned long, int length);
        unsigned long ESP8266_FLASH_ATTR power2(int power);

        void TransmitLatch(void);
        void Transmit(int pulse_length);

    public:
        ESP8266_FLASH_ATTR NexaCtrl(unsigned int tx_pin, unsigned long controller_id = 0, int transmitCount = 10, unsigned int led_pin = 0);

        void ESP8266_FLASH_ATTR Switch(unsigned int device_id, uint8_t value);
		void ESP8266_FLASH_ATTR DeviceOn(unsigned int device_id);
		void ESP8266_FLASH_ATTR DeviceOff(unsigned int device_id);
		void ESP8266_FLASH_ATTR DeviceDim(unsigned int device_id, unsigned int dim_level);
		void ESP8266_FLASH_ATTR GroupOn();
		void ESP8266_FLASH_ATTR GroupOff();
};

#endif /* NexaCtrl_h */
