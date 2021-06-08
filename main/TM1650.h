#ifndef TM1650_h
#define TM1650_h


//#include "IntexSWG.h"

#define TM1650_MAX_POS 4

// TM1650 has two display modes: 8 seg x 4 grd and 7 seg x 4 grd
#define TM1650_DISPMODE_4x8 0x01
#define TM1650_DISPMODE_4x7 0x09

#define TM1650_CMD_MODE  0x48
#define TM1650_CMD_DATA_READ  0x4F//0x49
#define TM1650_CMD_ADDRESS  0x68

#include <stdio.h>

class TM1650
{
  public:
    /** Instantiate a TM1650 module specifying the  data and clock pins, number of digits, display state, the starting intensity (0-7). */
    TM1650(gpio_num_t dataPin, gpio_num_t clockPin, uint8_t numDigits=4, bool activateDisplay=true, uint8_t intensity=7, uint8_t displaymode = TM1650_DISPMODE_4x8);
    void clearDisplay();
    void setupDisplay(bool active, uint8_t intensity);
    uint8_t getButtonPressedCode();
    void send(uint8_t data);
    void setSegments(uint8_t segments, uint8_t position);

  protected:
#if defined(__AVR_ATtiny85__) ||  defined(__AVR_ATtiny13__) ||  defined(__AVR_ATtiny44__)
		// On slow processors we may not need this bitDelay, so save some flash
#else
    void bitDelay();
#endif
    void start();
    void stop();    
    void sendData(uint8_t address, uint8_t data);
    uint8_t receive();
    uint8_t _maxDisplays=2;		// maximum number of digits (grids), chip-dependent
    uint8_t _maxSegments=8;		// maximum number of segments per display, chip-dependent

    uint8_t digits;		// number of digits in the display, module dependent
    gpio_num_t dataPin;
    gpio_num_t clockPin;
    gpio_num_t strobePin;    
};

#endif