/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_task_wdt.h"
#include "soc/rtc_wdt.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "freertos/portmacro.h"
#include <soc/rtc.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/timer.h"
#include "esp32/rom/uart.h"
#include "wifi_manager.h"

#include <string.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "esp_system.h"
#include "esp_log.h"

#include <inttypes.h>

#include "RestServer.h"
//#include "OTAServer.h"
#include "IntexSWG.h"
#include "TM1650.h"
#include "utils.h"


// DIO=18, CLK=19, Digits=2, ActivateDisplay=true, Intensity=3, DisplayMode=4x8
TM1650 module(dataDispPin, clockDispPin, 2, true, 3, TM1650_DISPMODE_4x8);

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "main";

TaskHandle_t xHandle1;
TaskHandle_t xHandle2;
TaskHandle_t TaskA;

volatile bool displayON = true;
volatile bool machineON = false;
volatile uint8_t displayIntensity = 3;      // (range 0-7)
volatile uint8_t statusLedsIntensity = 3;   // (range 0-7)
volatile bool sendingKeyCode = false;
volatile bool keyCodeSetByAPI = false;
volatile uint8_t buttonStatus = 0x2E;
volatile uint8_t nextButtonStatus = 0x00;
volatile uint8_t prevButtonStatus = 0x00;
volatile uint8_t dataReceived[2];
volatile uint8_t statusDigit1;
volatile uint8_t statusDigit2;
volatile uint8_t statusDigit3;
volatile uint8_t displayingDigit1;
volatile uint8_t displayingDigit2;
volatile uint8_t powerStatus;
volatile bool displayBlinking;
volatile bool removeWifiConfig = false;
volatile bool otaUpdating = false;
volatile bool wifiReconnecting = false;
volatile bool delayedPowerOff = false;
volatile bool readingMaster = false;
volatile uint8_t selfCleanTime = 10;

volatile uint16_t virtualPressButtonTime = 250;
volatile uint32_t checkpoint=0;
volatile bool waitForWifiConfig = true;
volatile uint8_t dataReceivedBuffer[128][2];
volatile int totalbytes = 0;


char getDisplayDigitFromCode(uint8_t code) {
    switch (code) {
        case DISP_BLANK: return 0;
        case DISP_0: return '0';
        case DISP_1: return '1';
        case DISP_2: return '2';
        case DISP_3: return '3';
        case DISP_4: return '4';
        case DISP_5: return '5';
        case DISP_6: return '6';
        case DISP_7: return '7';
        case DISP_8: return '8';
        case DISP_9: return '9';
        case DISP_DP: return '.';
        case DISP_1_CLEAN_06P: return '6';
        case DISP_1_CLEAN_10P: return '0';
        case DISP_1_CLEAN_14P: return '4';
        default: return 0;
    }
}

uint8_t getCodeFromDisplayDigit(char displayDigit) {
    switch (displayDigit) {
        case 0: return DISP_BLANK;
        case '0': return DISP_0;
        case '1': return DISP_1;
        case '2': return DISP_2;
        case '3': return DISP_3;
        case '4': return DISP_4;
        case '5': return DISP_5;
        case '6': return DISP_6;
        case '7': return DISP_7;
        case '8': return DISP_8;
        case '9': return DISP_9;
        case '.': return DISP_DP;
        default: return DISP_BLANK;
    }
}

void IRAM_ATTR machinePower(bool powerON) {
    if (powerON) { 
        machineON = true;
        delayMicroseconds(2500);
        GPIO_Clear(powerRelayPin);        
    }
    else {
        GPIO_Set(powerRelayPin);
        machineON = false;
        delayMicroseconds(200);
        statusDigit1 = statusDigit2 = statusDigit3 = DISP_BLANK;
    }
}

void feedTheDog(){
    // feed dog 0
    TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
    TIMERG0.wdt_feed=1;                       // feed dog
    TIMERG0.wdt_wprotect=0;                   // write protect
    // feed dog 1
    TIMERG1.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
    TIMERG1.wdt_feed=1;                       // feed dog
    TIMERG1.wdt_wprotect=0;                   // write protect
}

inline uint32_t IRAM_ATTR clocks()
{
    uint32_t ccount;
    asm volatile ( "rsr %0, ccount" : "=a" (ccount) );
    return ccount;
}

inline void delayClocks(uint32_t clks)
{
    uint32_t c = clocks();
    while( (clocks() - c ) < clks ){
        asm(" nop");
    }
}

/**
 * @brief Task in charge of ESP32<->SWG serial BUS
 */
void IRAM_ATTR Core1( void* p) {

    vTaskDelay(1000);    
    machinePower(true);

    portDISABLE_INTERRUPTS();
    register uint8_t sdaValue = 1, sclValue = 0, prevSda = 1, prevScl = 0, totalClocks = 0, totalBitsSent = 0;
    register bool receivingData = false, bytePosition = false, sendingACK = false;
    uint8_t receivedByte = 0; 

    sdaValue = prevSda = GPIO_IN_Get(dataPin);
    prevScl = GPIO_IN_Get(clockPin); 

    while(1) {

        if (totalbytes == 127) {
            totalbytes = 0;
        }
        
        if (otaUpdating || removeWifiConfig || !machineON || wifiReconnecting) {
            readingMaster = false;
            portENABLE_INTERRUPTS();

            sdaValue = 1, sclValue = 0, prevSda = 1, prevScl = 0, totalClocks = 0, totalBitsSent = 0;
            receivingData = false, bytePosition = true, sendingACK = false, sendingKeyCode = false;
            receivedByte = 0;

            while (otaUpdating || removeWifiConfig || !machineON || wifiReconnecting) {
                feedTheDog();
            }
            vTaskDelay(1000);
            portDISABLE_INTERRUPTS();

            sdaValue = prevSda = GPIO_IN_Get(dataPin);
            prevScl = GPIO_IN_Get(clockPin); 
        }
        readingMaster = true;

        // Get next SCL and SDA values
        if (!sendingKeyCode && !sendingACK) {
            sdaValue = GPIO_IN_Get(dataPin);
        }
        sclValue = GPIO_IN_Get(clockPin);


        // START Condition **********************************************************************************************************
        if (sclValue == HIGH && prevSda == HIGH && sdaValue == LOW) {
            receivingData = true;
            bytePosition = false;            
        }
        // START Condition **********************************************************************************************************


        // STOP Condition ***********************************************************************************************************
        else if (sclValue == HIGH && prevSda == LOW && sdaValue == HIGH) {            
            receivingData = false;
            sendingKeyCode = false;
            bytePosition = false;
            totalClocks = 0;
            receivedByte = 0;
        }
        // STOP Condition ***********************************************************************************************************            


        // Detect CLK RISING EDGE AFTER START CONDITION (0 -> 1) ********************************************************************
        else if (prevScl == LOW && sclValue == HIGH && receivingData) {
            if (!sendingKeyCode) {
                // RISING edge 1-8                    
                if (totalClocks < 8) {
                    // Read SDA bit
                    receivedByte <<= 1;   // MSB first on TM1650, so shift left                            
                    if (sdaValue == HIGH) {
                        receivedByte |= 1;	// MSB first on TM1650, so set lowest bit
                    }                                         
                }
                // RISING edge 9
                else {                      
                    // Save received byte
                    dataReceived[(int)bytePosition] = receivedByte;                                                                                                

                    // Position 0 (first byte)
                    if (!bytePosition) {
                        dataReceivedBuffer[totalbytes][0] = receivedByte;                        
                        // Read Keyboard code request received
                        if (dataReceived[0] == 0x4F) {
                            sendingKeyCode = true;
                            bytePosition = !bytePosition;
                            dataReceivedBuffer[totalbytes][1] = buttonStatus;
                            totalbytes++;            
                        }
                    }
                    // Position 1 (second byte)
                    else {
                        dataReceivedBuffer[totalbytes][1] = receivedByte;
                        totalbytes++;
                        // Check received byte for corresponding digit
                        switch (dataReceived[0])
                        {
                            case DIGIT1:
                                statusDigit1 = dataReceived[1];
                                break;
                            case DIGIT2:
                                statusDigit2 = dataReceived[1];
                                break;
                            case DIGIT3:
                                statusDigit3 = dataReceived[1];
                                break;                            
                            default:
                                break;
                        }
                    }               
                    receivedByte = 0;                    
                    bytePosition = !bytePosition;
                }
                totalClocks++;
            }            
        }
        // Detect CLK RISING EDGE AFTER START CONDITION (0 -> 1) ********************************************************************


        // Detect CLK FALLING EDGE (1 -> 0) *****************************************************************************************
        else if (prevScl == HIGH && sclValue == LOW  && receivingData) {

            // FALLING edge 8
            if (totalClocks == 8 && !sendingKeyCode) {
                // BEGIN SEND ACK
                digitalWrite(dataPin, LOW);
                pinMode(dataPin, GPIO_MODE_OUTPUT);
                sendingACK = true;
            }

            // FALLING edge 9
            else if (sendingACK && totalClocks == 9) {
                // STOP SENDING ACK (Keep dataPin as output if we are about to send key code to master)
                if (sendingKeyCode == false) {
                    // Release SDA, Stop Send ACK 
                    pinMode(dataPin, GPIO_MODE_INPUT);
                    digitalWrite(dataPin, HIGH);
                }
                sendingACK = false;
                totalClocks = 0;
            }

            // SEND KEY CODE (Master requested to read keys with command 0x4F)
            // First bit to send in FALLING edge 9 of first received BYTE
            if (sendingKeyCode) {                  
                if (totalBitsSent < 8) {                    
                    digitalWrite(dataPin, (buttonStatus << totalBitsSent) & 0x80 ? HIGH : LOW);
                    totalBitsSent++;
                }
                // FALLING edge 8 while sending keycode
                else if (totalBitsSent == 8) {
                    // Receive ACK
                    pinMode(dataPin, GPIO_MODE_INPUT);                        
                    digitalWrite(dataPin, HIGH);
                    totalBitsSent++;                    
                }
                // FALLING edge 9 while sending keycode
                else {
                    sendingKeyCode = false;
                    totalBitsSent = 0;
                }
            }
        }
        // Detect CLK FALLING EDGE (1 -> 0) *****************************************************************************************


        // Save previous SDA and SCL values
        if (!sendingKeyCode) {
            prevSda = sdaValue;            
        }        
        prevScl = sclValue;        
        
        feedTheDog();   
    }
}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ok(void *pvParameter) {
    //wifiReconnecting = false;
	//ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	//char str_ip[16];
	//esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	//ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);    
}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ko(void *pvParameter) {    
    esp_wifi_connect();
}


/**
 * @brief Task that shows a countdown en restarts ESP32 when wifi configuration has been saved successfully
 */
void reset_esp(void *pvParameter){
    waitForWifiConfig = false;
    statusDigit2 = DISP_0;
    for (int i = 3; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        switch (i)
        {
        case 3:
            statusDigit1 = DISP_3;
            break;
        case 2:
            statusDigit1 = DISP_2;
            break;
        case 1:
            statusDigit1 = DISP_1;
            break;
        case 0:
            statusDigit1 = DISP_0;
            break;        
        default:
            break;
        }        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        feedTheDog();
    }
    otaUpdating = false;
    removeWifiConfig = false;
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

void sendDataToDisplay(uint8_t digit, uint8_t value, uint8_t intensity) {
    if (!keyCodeSetByAPI) buttonStatus = module.getButtonPressedCode();
    module.setupDisplay(displayON, intensity);
    module.setSegments(value, (digit & 0b111) >> 1);    
}

/**
 * @brief Task in charge of ESP32<->Display serial BUS
 */
void RTOS_1(void *p) {

    vTaskDelay(1000);

    while(1) {

        sendDataToDisplay(DIGIT1, statusDigit1, displayIntensity);
        vTaskDelay(10);
        sendDataToDisplay(DIGIT2, statusDigit2, displayIntensity);
        vTaskDelay(10);
        sendDataToDisplay(DIGIT3, statusDigit3, statusLedsIntensity);           
        vTaskDelay(10);
        
        taskYIELD();
    }
}


void select_self_clean_period() {
    unsigned long time1;
    unsigned long time2;
    time1 = time2 = millis();
    if (selfCleanTime != displayingDigit1) {
        buttonStatus = 0x00;
        ESP_LOGI(TAG, "selfCleanTime = 0x%02X / displayingDigit1 = 0x%02X", selfCleanTime, displayingDigit1);
        while(selfCleanTime != displayingDigit1) {
            displayingDigit1 = (statusDigit2 > 0x00) ? statusDigit1 : displayingDigit1;
            if (time2 - time1 > 250) {
                buttonStatus = (buttonStatus == 0x00) ? BUTTON_SELF_CLEAN : 0x00;                
                time1 = time2 = millis();
            } 
            else {
                time2 = millis();
            }
        }
    }
}

void press_lock_button() {
    unsigned long time1;
    unsigned long time2;
    time1 = time2 = millis();    
    buttonStatus = BUTTON_LOCK;
    while(time2 - time1 < 250) {
        time2 = millis();            
    }
    buttonStatus = 0x00;    
}


/**
 * @brief Task that controls virtual key press (API), power status info and display information to be retrieved by API
 */
void RTOS_2(void *p) {

    vTaskDelay(1000);
    
    unsigned long time1_keycode_api = 0;
    unsigned long time2_keycode_api = 0;
    //unsigned long time1_power = 0;
    //unsigned long time2_power = 0;
    unsigned long time1_displ = 0;
    unsigned long time2_displ = 0;
    unsigned long time1_delayed_keycode = 0;
    unsigned long time2_delayed_keycode = 0;
    unsigned long time1_delayed_off = 0;
    unsigned long time2_delayed_off = 0;    
    //bool statusPowerLed = false;
    //bool prevStatusPowerLed = false;
    bool statusDisplayLed = false;
    bool prevStatusDisplayLed = false;
    //uint8_t powerBlinks = 0;
    uint8_t displayBlinks = 0;
    /*time1_power = time2_power = */time1_displ = time2_displ = millis();
    //powerBlinks = 0;

    while(1) { 
        if (removeWifiConfig) {            
            wifi_manager_clear_wifi_configuration();
            reset_esp(NULL);
        }
        
        // TODO: Supress booting status, or simulate according to the time it takes to start !!!
        // Check and update Power status ***********************************************************
        /*
        if (time2_power - time1_power < 1500) {        
            statusPowerLed = (statusDigit3 & (0x01 << LED_POWER)) >> LED_POWER == 1 ? true : false;
            if (prevStatusPowerLed != statusPowerLed) {
                powerBlinks++;
                prevStatusPowerLed = statusPowerLed;
            }
            time2_power = millis();
        }
        else {
            powerStatus = (powerBlinks > 1) ? POWER_STATUS_BOOTING : (powerBlinks == 1) ? POWER_STATUS_ON : (statusDigit2 == DISP_DP) ? POWER_STATUS_STANDBY : POWER_STATUS_OFF;
            powerBlinks = 0;
            time1_power = time2_power = millis();
        }
        */
        powerStatus = (machineON) ? ((statusDigit2 == DISP_DP) ? POWER_STATUS_STANDBY : (statusDigit2 != DISP_BLANK) ? POWER_STATUS_ON : POWER_STATUS_BUS_ERROR) : POWER_STATUS_OFF;
        //powerStatus = (powerBlinks == 1) ? POWER_STATUS_ON : (statusDigit2 == DISP_DP) ? POWER_STATUS_STANDBY : POWER_STATUS_OFF;
        // Check and update Power status ***********************************************************

        // Check and update Display status ***********************************************************
        displayingDigit1 = (statusDigit1 == 0x00 && displayBlinking) ? displayingDigit1 : statusDigit1;
        displayingDigit2 = (statusDigit2 == 0x00 && displayBlinking) ? displayingDigit2 : statusDigit2;
        if (time2_displ - time1_displ < 600) {
            statusDisplayLed = (statusDigit2 > 0x00) ? true : false;
            if (prevStatusDisplayLed != statusDisplayLed) {
                displayBlinks++;
                prevStatusDisplayLed = statusDisplayLed;
            }
            time2_displ = millis();
        }
        else {
            displayBlinking = (displayBlinks > 0) ? true : false;
            displayBlinks = 0;
            time1_displ = time2_displ = millis();
        }
        // Check and update Display status ***********************************************************

        // Delayed Virtual Button press **************************************************************
        if (nextButtonStatus > 0) {
            // Start counting time
            if (time1_delayed_keycode == 0 && time2_delayed_keycode == 0) {
                time1_delayed_keycode = time2_delayed_keycode = millis();
            }
            else if (time2_delayed_keycode - time1_delayed_keycode > 2000) {
                buttonStatus = nextButtonStatus;
                nextButtonStatus = 0x00;
                keyCodeSetByAPI = true;
                time1_delayed_keycode = time2_delayed_keycode = 0;
            } 
            else {
                time2_delayed_keycode = millis();
            } 
        }
        // Delayed Virtual Button press **************************************************************        

        // Keep API keycode for ms (Virtual Press Button defined in virtualPressButtonTime) **********
        if (keyCodeSetByAPI) {
            // Start counting time
            if (time1_keycode_api == 0 && time2_keycode_api == 0) {
                time1_keycode_api = time2_keycode_api = millis();
            }
            if (time2_keycode_api - time1_keycode_api > virtualPressButtonTime) {
                // Self cleaning macro
                if (buttonStatus == BUTTON_SELF_CLEAN) {
                    select_self_clean_period();
                    press_lock_button();
                }
                // TODO: Manage programming macro
                keyCodeSetByAPI = false;
                time1_keycode_api = time2_keycode_api = 0;
            } 
            else {
                time2_keycode_api = millis();
            }
        }
        // Keep API keycode for ms (Virtual Press Button defined in virtualPressButtonTime) **********

        // Turn off the machine after a delay of 2 sec ***********************************************
        if (delayedPowerOff) {
            // Start counting time
            if (time1_delayed_off == 0 && time2_delayed_off == 0) {
                time1_delayed_off = time2_delayed_off = millis();
            }
            else if (time2_delayed_off - time1_delayed_off > 2000) {
                delayedPowerOff = false;                
                time1_delayed_off = time2_delayed_off = 0;
                machinePower(false);
            } 
            else {
                time2_delayed_off = millis();
            }           
        }
        // Turn off the machine after a delay of 2 sec ***********************************************        
        
        taskYIELD();
    }
}


/**
 * @brief Show blinking "AP" in the display while waiting for wifi configuration
 */
void ConfigureWifi(void *p) {
    vTaskDelay(1000);
    int i = 0;
    while(waitForWifiConfig) {
        //statusDigit2 = (i % 2 == 0) ? 0x02 : 00;
        statusDigit1 = (i % 2 == 0) ? DISP_P : DISP_BLANK;
        statusDigit2 = (i % 2 == 0) ? DISP_A : DISP_BLANK;
        //statusDigit3 = 1 << i;
        i = (i < 7) ? i + 1 : 0;
        delayMicroseconds(300000);
        feedTheDog();
        taskYIELD();
    } 
    vTaskDelete( NULL );
}


void startCore0a(void) {
    xTaskCreatePinnedToCore(
        RTOS_1,                 // Function that implements the task.
        "RTOS-1",               // Text name for the task.
        STACK_SIZE,             // Stack size in bytes, not words.
        ( void * ) 1,           // Parameter passed into the task.
        tskIDLE_PRIORITY + 4,   // Priority
        &xHandle1,              // Variable to hold the task's data structure.
        0);
}

void startCore0b(void) {
    xTaskCreatePinnedToCore(
        RTOS_2,                 // Function that implements the task.
        "RTOS-2",               // Text name for the task.
        STACK_SIZE,             // Stack size in bytes, not words.
        ( void * ) 1,           // Parameter passed into the task.
        tskIDLE_PRIORITY + 3,   // Priority
        &xHandle2,              // Variable to hold the task's data structure.
        0);
}

// Function that creates the superloop Core1 to be pinned at Core 1
void startCore1( void )
{
    xTaskCreatePinnedToCore(
        Core1,                  // Function that implements the task.
        "Core1",                // Text name for the task.
        STACK_SIZE,             // Stack size in bytes, not words.
        ( void * ) 1,           // Parameter passed into the task.
        tskIDLE_PRIORITY + 5,   // Priority
        &TaskA,                 // Task
        1);                     // Core 1
}

// Function that creates the superloop Core1 to be pinned at Core 1
void configureWifiTask( void )
{
    xTaskCreate(
        ConfigureWifi,                  // Function that implements the task.
        "ConfigureWifi",                // Text name for the task.
        STACK_SIZE,             // Stack size in bytes, not words.
        ( void * ) 1,           // Parameter passed into the task.
        tskIDLE_PRIORITY + 2,   // Priority
        &TaskA);                 // Task
}


extern "C" void app_main(void)
{
    pinMode(dataPin, GPIO_MODE_INPUT);
    GPIO_Set(dataPin);

    pinMode(clockPin, GPIO_MODE_INPUT);
    GPIO_Set(clockPin);

    pinMode(dataDispPin, GPIO_MODE_OUTPUT);
    pinMode(clockDispPin, GPIO_MODE_OUTPUT);

    pinMode(powerRelayPin, GPIO_MODE_OUTPUT);
    GPIO_Set(powerRelayPin);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Free heap: %d\n", esp_get_free_heap_size());


/*
    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
*/

    /* start the wifi manager */
	wifi_manager_start();

    //wifi_manager_clear_wifi_configuration();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	//wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);    

	/* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	//xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 0);
    /*
    while (!wifi_manager_fetch_wifi_sta_config())
    {
        delayMicroseconds(10000000);
    }
    */
    
    if (wifi_manager_fetch_wifi_sta_config())
    {
        //wifi_manager_set_callback(WM_EVENT_WIFI_CONFIG_CLEARED, &reset_esp);

        wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &cb_connection_ko);
        wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

        startCore1();
        xTaskCreatePinnedToCore(&systemRebootTask, "rebootTask", 2048, NULL, 5, NULL, 0);
        start_rest_server(8080);
    }
    else {
        wifi_manager_set_callback(WM_EVENT_WIFI_CONFIG_SAVED, &reset_esp);
        configureWifiTask();
    }

    startCore0a();    
    startCore0b();
}
