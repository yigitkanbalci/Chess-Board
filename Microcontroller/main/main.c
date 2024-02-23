#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "ds18b20.h"

#define ONE_WIRE_GPIO 36 // Define the GPIO pin for DS18B20
#define MAX_SENSORS 1    // Maximum number of sensors connected
#define READROM 0x33     // Command to read ROM

void app_main(void) {
    printf("Hello world!\n");

    // Initialize the DS18B20 on the specified GPIO pin
    ds18b20_init(ONE_WIRE_GPIO);

    // Attempt to reset and detect the presence of DS18B20
    if (ds18b20_reset() == 1) { // Check if the presence pulse is detected
        printf("DS18B20 presence detected.\n");

        // Send the READROM command to read the sensor's unique ROM code
        ds18b20_write_byte(READROM);

        unsigned char romCode[8]; // Array to store the ROM code
        printf("Reading DS18B20 ROM code: ");
        for (int i = 0; i < 8; i++) {
            romCode[i] = ds18b20_read_byte(); // Read each byte of the ROM code
            printf("%02x", romCode[i]); // Print each byte in hex format
        }
        printf("\n");
    } else {
        printf("DS18B20 not detected, check wiring and pull-up resistor.\n");
    }

    // Main loop
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
    }
}

