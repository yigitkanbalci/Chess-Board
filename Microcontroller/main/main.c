#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "ds18b20.h"

#define ONE_WIRE_GPIO 36 // Define the GPIO pin for DS18B20
#define MAX_SENSORS 1    // Maximum number of sensors connected
#define READROM 0x33     // Command to read ROM
void scanBoard(char * boardState);

void app_main(void) {
    printf("Hello world!\n");

    // Initialize the DS18B20 on the specified GPIO pin
    ds18b20_init(ONE_WIRE_GPIO);

    // Configure GPIO for select pins

    gpio_config_t outputPinConfig = {};
    outputPinConfig.intr_type = GPIO_INTR_DISABLE; //Don't need interrupts since we're using it for output
    outputPinConfig.pull_down_en = 1; //make sure pins are set low
    outputPinConfig.pin_bit_mask = 63; //0000000000000000000000000000000000111111
    outputPinConfig.mode = GPIO_MODE_OUTPUT;
    gpio_config(&outputPinConfig);

    // Attempt to reset and detect the presence of DS18B20
    unsigned long long boardState [64] = {0};
    scanBoard(boardState);

    // Print current board state

    // Main loop
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
    }
}

void scanBoard(char * boardState){
    for(int i=0; i < 64; i++){ //cycle through all 64 tiles
        REG_WRITE(GPIO_OUT_W1TC, 63);
        REG_WRITE(GPIO_OUT_W1TS_REG, i);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        unsigned long long romCode = 0; // 64bit var

        if (ds18b20_reset() == 1) { // Check if the presence pulse is detected
            printf("DS18B20 presence detected.\n");

            // Send the READROM command to read the sensor's unique ROM code
            ds18b20_write_byte(READROM);

            //unsigned char romCode[8]; // Array to store the ROM code
            printf("Reading DS18B20 ROM code: ");
            for (int j = 7; j >= 0; j--) {
                romCode = romCode | ds18b20_read_byte()<<j; // Read each byte of the ROM code
                //printf("%02x", romCode[i]); // Print each byte in hex format
            }
            printf("\n");
        }
        else{
            romCode = 0;
        }
    printf("ID: %llx\n", romCode);
    boardState[i] = romCode;
    break;
    }
}