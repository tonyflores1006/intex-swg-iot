#ifndef IntexSWG_h
#define IntexSWG_h

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_SIZE 4096

#define CLOCKS_1_ns 240
#define CLOCKS_2_ns 480
#define CLOCKS_3_ns 720
#define CLOCKS_4_ns 960
#define CLOCKS_5_ns 1200
#define CLOCKS_100_ns 23998
#define CLOCKS_500_ns 119990
#define CLOCKS_1_us 239981 
#define CLOCKS_2_us 479961
#define CLOCKS_3_us 719942
#define CLOCKS_4_us 959923
#define CLOCKS_5_us 1199904


#define clockPin        GPIO_NUM_19
#define dataPin         GPIO_NUM_18
#define clockDispPin    GPIO_NUM_17
#define dataDispPin     GPIO_NUM_16
#define powerRelayPin   GPIO_NUM_0

// Segment-bit: E D G F B A DP C
#define DISP_BLANK  0b00000000
#define DISP_0      0b11011101
#define DISP_1      0b00001001
#define DISP_2      0b11101100
#define DISP_3      0b01101101
#define DISP_4      0b00111001
#define DISP_5      0b01110101
#define DISP_6      0b11110101
#define DISP_7      0b00001101
#define DISP_8      0b11111101
#define DISP_9      0b01111101
#define DISP_DP     0b00000010
#define DISP_A      0b10111101
#define DISP_P      0b10111100
#define DISP_1_CLEAN_06P 0b11110111
#define DISP_1_CLEAN_10P 0b11011111
#define DISP_1_CLEAN_14P 0b00111011


#define LED_OZONE           0   // 0b00000001
#define LED_SLEEP           1   // 0b00000010
#define LED_BOOST           2   // 0b00000100
#define LED_POWER           3   // 0b00001000
#define LED_PUMP_LOW_FLOW   4   // 0b00010000 12???
#define LED_LOW_SALT        5   // 0b00100000
#define LED_HIGH_SALT       6   // 0b01000000
#define LED_SERVICE         7   // 0b10000000

#define LEFT_DIGIT  1
#define RIGHT_DIGIT 0
#define STATUS_LEDS 2

#define POWER_STATUS_OFF        0
#define POWER_STATUS_STANDBY    1
#define POWER_STATUS_ON         2
#define POWER_STATUS_BOOTING    3
#define POWER_STATUS_BUS_ERROR  4

#define DIGIT1              0x68
#define DIGIT2              0x6A
#define DIGIT3              0x6C

#define BUTTON_BOOST        0x44
#define BUTTON_TIMER        0x46
#define BUTTON_POWER        0x4C
#define BUTTON_LOCK         0x4E
#define BUTTON_SELF_CLEAN   0x74

extern volatile bool displayON;
extern volatile bool machineON;
extern volatile uint8_t displayIntensity; // (range 0-7)
extern volatile uint8_t statusLedsIntensity; // (range 0-7)
extern volatile bool sendingKeyCode;
extern volatile uint8_t buttonStatus;
extern volatile uint8_t nextButtonStatus;
extern volatile uint8_t prevButtonStatus;
extern volatile bool keyCodeSetByAPI;
extern volatile uint8_t statusDigit1;
extern volatile uint8_t statusDigit2;
extern volatile uint8_t statusDigit3;
extern volatile uint8_t displayingDigit1;
extern volatile uint8_t displayingDigit2;
extern volatile uint8_t powerStatus;
extern volatile uint16_t virtualPressButtonTime;
extern volatile bool displayBlinking;
extern volatile bool removeWifiConfig;
extern volatile bool otaUpdating;
extern volatile bool wifiReconnecting;
extern volatile bool delayedPowerOff;
extern volatile bool readingMaster;
extern volatile uint8_t selfCleanTime;

extern volatile uint8_t dataReceivedBuffer[128][2];

//const std::string hostname = "INTEX-SWG";


char getDisplayDigitFromCode(uint8_t code);
uint8_t getCodeFromDisplayDigit(char displayDigit);
void IRAM_ATTR machinePower(bool powerON);


#ifdef __cplusplus
}
#endif

#endif //IntexSWG_h