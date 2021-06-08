#ifndef utils_h
#define utils_h

#define GPIO_Set(x)             REG_WRITE(GPIO_OUT_W1TS_REG, 1<<x) 
#define GPIO_Clear(x)           REG_WRITE(GPIO_OUT_W1TC_REG, 1<<x) 
#define GPIO_IN_Read(x)         REG_READ(GPIO_IN_REG) & (1<<x) 
#define GPIO_IN_ReadAll()       REG_READ(GPIO_IN_REG)
#define GPIO_IN_Get(x)          (REG_READ(GPIO_IN_REG) & (1<<x))>>x 
#define pinMode(a,b)            gpio_set_direction(a, b);
#define digitalRead(x)          GPIO_IN_Read(x)

#define INPUT GPIO_MODE_INPUT
#define OUTPUT GPIO_MODE_OUTPUT
#define LOW 0x00
#define HIGH 0x01

uint64_t millis();
uint64_t micros();
void delayMicroseconds(uint32_t us);
void digitalWrite(uint8_t pin, uint8_t state);
//void pinMode(gpio_num_t pin, gpio_mode_t mode);
//String checkLedStatus(byte statusDigit, byte ledMask);
//bool isDisplayBlinking(byte displayDigit);

#endif