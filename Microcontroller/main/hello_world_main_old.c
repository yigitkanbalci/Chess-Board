/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */


/*
    pins 0-2 are row select outputs
    3-5 are column select outputs
    6 is the output signal read
    7-14 are going to be test signal outputs, representing the connector signals

    You should make the 7-14 act like those, and sweep through 0-64 in binary using 
    pins 0-7, and be able to read the output on pin 6, which should correspond to 
    whichever 7-14 pin is currently selected, or nothing at all 7/8 of the time
*/


/*
typedef struct {
    uint64_t pin_bit_mask;          // GPIO pin: set with bit mask, each bit maps to a GPIO 
    gpio_mode_t mode;               // GPIO mode: set input/output mode                     
    gpio_pullup_t pull_up_en;       // GPIO pull-up                                         
    gpio_pulldown_t pull_down_en;   // GPIO pull-down                                       
    gpio_int_type_t intr_type;      // GPIO interrupt type                                  

} gpio_config_t;

*/

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "ds18b20.h" //ds18b20 library made by Filip Grzywok and David Lehrian. https://github.com/feelfreelinux/ds18b20

#define READROM			0x33  // command can be used on a bus with a single slave to read the 64-bit unique identifier 

DeviceAddress* ds18b20_getAddr(void);

void app_main(void)
{
    printf("Hello world!\n");

    //int row = 0;
    //int col = 0;
    /*
    gpio_config_t outputPinConfig = {};
    outputPinConfig.intr_type = GPIO_INTR_DISABLE; //Don't need interrupts since we're using it for output
    outputPinConfig.pull_down_en = 1; //make sure pins are set low
    outputPinConfig.pin_bit_mask = 63; //0000000000000000000000000000000000111111
    outputPinConfig.mode = GPIO_MODE_OUTPUT;
    gpio_config(&outputPinConfig);

    for(int i=0; i < 64; i++){ //cycle through all 64 tiles
        REG_WRITE(GPIO_OUT_W1TC, 63);
        REG_WRITE(GPIO_OUT_W1TS_REG, i);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }*/
    
    printf("Finished scanning, starting 1-wire");

    //int oneWirePin = 37;
    //gpio_reset_pin(oneWirePin);
    ds18b20_init(37);
    DeviceAddress* romAdd = ds18b20_getAddr();
    printf("\ndone\n");
    while(true){ //spin forever 
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

DeviceAddress* ds18b20_getAddr(void){
	unsigned char pres ds18b20_reset();
    ds18b20_write_byte(READROM);
    printf("\nfinished write_byte READROM\n");
    unsigned char romAdd[9];
    for (int i=0; i < 8; i++){
        romAdd[i] = ds18b20_read_byte();
    }
    romAdd[8] = '\0';
    printf("here is the address: ");
    printf("%s", romAdd);
    printf("|");
    return NULL;
}