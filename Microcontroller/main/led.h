#ifndef LED_H
#define LED_H

#include <stdio.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_num.h"
#include "rom/ets_sys.h"
#include "esp_task_wdt.h"

///Users/ryansui/led/main/hello_world_main.c
#define DATA GPIO_NUM_20
#define STORE GPIO_NUM_18
#define CLOCK GPIO_NUM_15
#define BIT_ORDER MSBFIRST

typedef enum {
    LSBFIRST = 0,
    MSBFIRST = 1
} BitOrder;

uint8_t img[8];

void shiftOut(uint8_t dataPin, uint8_t clockPin, BitOrder bitOrder, uint8_t val);
void store();
void led_initialize();
void convertMatrixToHex(int mat[8][8], uint8_t img[8]);
void led_loop(uint8_t img[8]);

#endif
