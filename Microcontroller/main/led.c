#include "led.h"

void shiftOut(uint8_t dataPin, uint8_t clockPin, BitOrder bitOrder, uint8_t val) {
    for (uint8_t i = 0; i < 8; i++)  {
        if (bitOrder == LSBFIRST)
            gpio_set_level(dataPin, !!(val & (1 << i)));
        else
            gpio_set_level(dataPin, !!(val & (1 << (7 - i))));

        gpio_set_level(clockPin, 1);
        vTaskDelay(1 / portTICK_PERIOD_MS); // Small delay
        gpio_set_level(clockPin, 0);
    }
}

void store(){
    gpio_set_level(STORE, 1);
    ets_delay_us(10);
    gpio_set_level(STORE, 0);
    ets_delay_us(10);
}

void led_initialize() {
    gpio_set_direction(DATA, GPIO_MODE_OUTPUT);
    gpio_set_direction(CLOCK, GPIO_MODE_OUTPUT);
    gpio_set_direction(STORE, GPIO_MODE_OUTPUT);
}

void convertMatrixToHex(int mat[8][8], uint8_t img[8]) {
    for (int i = 0; i < 8; i++) {
        int hex_value = 0;
        for (int j = 0; j < 8; j++) {
            hex_value = hex_value * 2 + mat[i][j];
        }
        img[i] = hex_value;  // Store the hex value in the img array
    }
}

void led_loop(uint8_t img[8]){
    while(1) {
        for (int i = 0; i < 8; i++){
            shiftOut(DATA, CLOCK, LSBFIRST, ~img[i]);
            shiftOut(DATA, CLOCK, LSBFIRST, 128 >> i);
            store();
        }
    }
}
