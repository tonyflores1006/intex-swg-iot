#include <stdint.h>
#include "driver/gpio.h"
#include "esp_event.h"

#include "utils.h"

uint64_t millis() {
    return esp_timer_get_time()/1000l;
}

uint64_t micros() {
    return esp_timer_get_time();
}

void delayMicroseconds(uint32_t us)
{
    if(us){
    uint32_t m = micros();
    while( (micros() - m ) < us ){
        asm(" nop");
    }
    }
}



/*
void pinMode(gpio_num_t pin, gpio_mode_t mode) {
    //gpio_reset_pin(pin);
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = mode;
	io_conf.pin_bit_mask = (1ULL<<pin) ;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	//io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);
}
*/

void digitalWrite(uint8_t pin, uint8_t state) {
    if (state == HIGH) {
        GPIO_Set(pin);
    }
    else {
        GPIO_Clear(pin);
    }
}

/*
String checkLedStatus(byte statusDigit, byte ledMask) {
    unsigned long time1 = 0;
    unsigned long time2 = 0;    
    bool ledStatus = false;
    bool prevLedStatus = false;
    uint8_t ledBlinks = 0;

    time1 = time2 = millis();
    while (time2-time1 < 800) {
        ledStatus = (statusDigit & (0x01 << ledMask)) >> ledMask == 1 ? true : false;
        if (prevLedStatus != ledStatus) {
            ledBlinks++;
            prevLedStatus = ledStatus;
        }        
        time2 = millis();
    }
    return (ledBlinks == 0) ? "OFF" : (ledBlinks == 1) ? "ON" : "BLINKING";
}

bool isDisplayBlinking(byte displayDigit) {
    unsigned long time1 = 0;
    unsigned long time2 = 0;    
    
    bool displayStatus = false;
    bool prevDisplayStatus = false;
    uint8_t displayBlinks = 0;

    time1 = time2 = millis();
    while (time2-time1 < 800) {
        displayStatus = displayDigit > 0 ? true : false;
        if (prevDisplayStatus != displayStatus) {
            displayBlinks++;
            prevDisplayStatus = displayStatus;
        }        
        time2 = millis();
    }
    return (displayBlinks > 0);
}
*/