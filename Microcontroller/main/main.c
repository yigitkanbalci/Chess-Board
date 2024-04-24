#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "ds18b20.h"
#include "pieces.h"
#include "esp_task_wdt.h"
#include "cJSON.h"
#include "led.c"
#include <stdbool.h>

void setup_peripherals(void);
void uart_event_task(void *pvParameters);
void board_scan_task(void *pvParameters);
void uart_send_data(const char* logName, const char* data);
void select_pins_init(void);
void set_multiplexer_channel(int channel);
void decoder_select_pins_init(void);
unsigned long long read_sensor_eeprom_id(int sensorIndex);
void set_sensor_rom(int sensorIndex, char* string);
void updateTile(Tile* tile, unsigned long long romCode);
int scanBoard(unsigned long long *array, Tile* prevTiles[8][8], Tile* tiles[8][8], int* total, int* moveState);
int init_mapping(unsigned long long *array, int size);
void reset_board(void);
void listen_task(void *pvParameters);
void app_main(void);
int load_game_state(Tile* tiles[8][8]);
void load_board();
void load_mapping();
void parseChessBoard(const char* jsonData);


#define ONE_WIRE_GPIO 9  // Define the GPIO pin for DS18B20
#define SELECT_PIN_0 34  // GPIO pin for S0
#define SELECT_PIN_1 21  // GPIO pin for S1
#define SELECT_PIN_2 19  // GPIO pin for S2

#define DECODER_SELECT_A 38  // GPIO pin for A input of 74HC138
#define DECODER_SELECT_B 37  // GPIO pin for B input of 74HC138
#define DECODER_SELECT_C 36  // GPIO pin for C input of 74HC138

#define UART_NUM UART_NUM_0
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE (1024)

#define LED_GPIO_PIN GPIO_NUM_7 // Adjust the pin number according to your setup

// Communication protocol header mask
#define HEADER_ERROR 0x01
#define HEADER_START 0x02
#define HEADER_MOVE 0x03
#define HEADER_LEGAL 0x04
#define HEADER_SUGGESTION 0x05
#define HEADER_END 0x06
#define HEADER_VALIDATE 0x07
#define HEADER_LOAD 0x08
#define HEADER_RESET 0x09

SemaphoreHandle_t scan_semaphore;
SemaphoreHandle_t led_semaphore;
static volatile bool allow_scanning = false;  // Control flag for scanning
static QueueHandle_t uart_queue;

unsigned long long rom_array[32];
Tile* prevTiles[8][8];
Tile* intermediateTiles[8][8];
Tile* tiles[8][8];
uint8_t img[8];

int total_pieces = 32;
int state = 0;
int moveState = 0;

void uart_send_data(const char* logName, const char* data) {
    if (data == NULL) return; // Safety check

    const int len = strlen(data);
    if (len > 0) {
        // Send the data as is
        uart_write_bytes(UART_NUM, data, len);
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // Delay for 10 milliseconds
}

// Function to initialize select pins
void select_pins_init() {
    gpio_set_direction(SELECT_PIN_0, GPIO_MODE_OUTPUT);
    gpio_set_direction(SELECT_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(SELECT_PIN_2, GPIO_MODE_OUTPUT);
}

// Function to set the multiplexer channel
void set_multiplexer_channel(int channel) {
    gpio_set_level(SELECT_PIN_0, (channel & 1) ? 1 : 0);
    gpio_set_level(SELECT_PIN_1, (channel & 2) ? 1 : 0);
    gpio_set_level(SELECT_PIN_2, (channel & 4) ? 1 : 0);
}

void decoder_select_pins_init() {
    gpio_set_direction(DECODER_SELECT_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(DECODER_SELECT_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(DECODER_SELECT_C, GPIO_MODE_OUTPUT);
}

// Returns the EEPROM ID read from the sensor, or 0 if none is detected.
unsigned long long read_sensor_eeprom_id(int sensorIndex) {
    int channel = sensorIndex % 8;
    set_multiplexer_channel(channel);
    if (ds18b20_reset() != 1) {
        return 0; // No sensor detected
    }
    ds18b20_write_byte(0x33); // READ ROM command
    unsigned long long romCode = 0;
    for (int i = 0; i < 8; i++) {
        unsigned char byte = ds18b20_read_byte();
        romCode |= ((unsigned long long)byte << (i * 8));
    }
    return romCode;
}

void set_sensor_rom(int sensorIndex, char* string) {
    int channel = sensorIndex % 8; // Determines which channel of the mux to use
    set_multiplexer_channel(channel); // Now, set the channel within that mux
    unsigned long long romCode = 0;  // Variable to store the 64-bit ROM code
    if (ds18b20_reset() == 1) {
        printf("\nDS18B20 on channel %d presence detected.\n", channel);

        ds18b20_write_byte(0x33);  // 0x33 is the READ ROM command

        //printf("Reading DS18B20 ROM code: ");
        for (int i = 0; i < 8; i++) {
            unsigned char byte = ds18b20_read_byte();
            romCode |= ((unsigned long long)byte << (i * 8));
            //printf("%02x", byte);
        }
        //printf("\nID: %llu\n", romCode);
        if (strcmp(string, "w") == 0) {
            assignEEPROMIDToPiece(romCode, sensorIndex, WhiteChessPieces);
        
        } else {
            assignEEPROMIDToPiece(romCode, sensorIndex, BlackChessPieces);
        }
    } else {
        //uart_send_data("TX_TASK", "No sensor detected");
    }
}

void updateTile(Tile* tile, unsigned long long romCode) {
    if (romCode == 0) {
        tile->piece = NULL;
        tile->eeprom_id = 0;
    } else {
        ChessPiece* piece = findPieceByEEPROMID(romCode, AllPieces);
        tile->piece = piece;
        tile->eeprom_id = romCode;
    }
}

int scanBoard(unsigned long long *array, Tile* prevTiles[8][8], Tile* tiles[8][8], int* total, int* moveState) {
    int currCount = 32;
    unsigned long long romCode;
    deepCopyTiles(prevTiles, tiles);
    gpio_set_level(DECODER_SELECT_A, 0);
    gpio_set_level(DECODER_SELECT_B, 1);
    gpio_set_level(DECODER_SELECT_C, 0);
    //printf("Reading from first mux:\n");
    int row = 0;
    for(int i = 0; i < 8; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
        
    }
    row++;
    //vTaskDelay(3000 / portTICK_PERIOD_MS);

    gpio_set_level(DECODER_SELECT_A, 1);
    gpio_set_level(DECODER_SELECT_B, 1);
    gpio_set_level(DECODER_SELECT_C, 0);
    //printf("Reading from second mux:\n");
    for(int i = 8; i < 16; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
    }
    row++;
    //vTaskDelay(3000 / portTICK_PERIOD_MS);

    gpio_set_level(DECODER_SELECT_A, 0);
    gpio_set_level(DECODER_SELECT_B, 0);
    gpio_set_level(DECODER_SELECT_C, 1);
    //printf("Reading from third mux:\n");
    for (int i = 16; i < 24; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
    }
    row++;

    //vTaskDelay(3000 / portTICK_PERIOD_MS);

    gpio_set_level(DECODER_SELECT_A, 1);
    gpio_set_level(DECODER_SELECT_B, 0);
    gpio_set_level(DECODER_SELECT_C, 1);
    //printf("Reading from fourth mux:\n");
    for (int i = 24; i < 32; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
    }
    row++;

    gpio_set_level(DECODER_SELECT_A, 0);
    gpio_set_level(DECODER_SELECT_B, 1);
    gpio_set_level(DECODER_SELECT_C, 1);

    for (int i = 32; i < 40; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
    }
    row++;

    gpio_set_level(DECODER_SELECT_A, 1);
    gpio_set_level(DECODER_SELECT_B, 1);
    gpio_set_level(DECODER_SELECT_C, 1);

    for (int i = 40; i < 48; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
    }
    row++;

    gpio_set_level(DECODER_SELECT_A, 1);
    gpio_set_level(DECODER_SELECT_B, 0);
    gpio_set_level(DECODER_SELECT_C, 0);

    for (int i = 48; i < 56; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
    }
    row++;

    gpio_set_level(DECODER_SELECT_A, 0);
    gpio_set_level(DECODER_SELECT_B, 0);
    gpio_set_level(DECODER_SELECT_C, 0);

    for (int i = 56; i < 64; i++) {
        int col = i % 8;
        romCode = read_sensor_eeprom_id(i);
        Tile* tile = tiles[row][col];
        updateTile(tile, romCode);
    }
    row++;

    currCount = countPieces(tiles); // Count pieces after the scan
        // Detect piece lifted
    if (currCount < *total) {
        if (*moveState == 0) {
            printf("Piece lifted\n");
            *moveState = 1; // Move to state indicating a piece has been lifted.
            currCount--;                
            return 1; // Early indication of a move.
        }
    } 
    else if (currCount == *total) {
        if (*moveState == 1) {
            printf("Piece replaced or moved without capture\n");
            currCount++;
            *moveState = 0; // Reset moveState.
            return 2; // Indicate that a move has completed without capture.
        }
    }

    // If we're in state 1 and a subsequent scan shows a decrease in total pieces, 
    // it indicates a capture has likely occurred.
    if (*moveState == 1 && currCount < (*total) - 1) {
        printf("Capture likely detected\n");
        printf("Total pieces: %d\n", *total);
        printf("Current pieces: %d\n", currCount);
        //*total = currCount; // Adjust the total for the potential capture
        *moveState = 3; // Move to a "buffer" state waiting for the next scan to confirm.
        return 3; // Preemptively indicate a capture/move completion.
    }

    // Confirm capture if we're in the buffer state and no piece has been replaced.
    if (*moveState == 3 && currCount == *total - 1) {
        printf("Capture confirmed\n");
        (*total)--;
        *moveState = 0; // Reset moveState after confirming the capture.
        return 4; // Indicate capture completion.
    }

    return 0;
}
    

int init_mapping(unsigned long long *array, int size) {

    initializeChessPieces(); 

    gpio_set_level(DECODER_SELECT_A, 0);
    gpio_set_level(DECODER_SELECT_B, 0);
    gpio_set_level(DECODER_SELECT_C, 0);
    //printf("Reading from first mux:\n");
    for(int i = 0; i < 8; i++) {
        set_sensor_rom(i, "w");
    }

    // Delay for a bit before switching muxes
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    gpio_set_level(DECODER_SELECT_A, 1);
    gpio_set_level(DECODER_SELECT_B, 0);
    gpio_set_level(DECODER_SELECT_C, 0);
    //printf("Reading from second mux:\n");
    for(int i = 8; i < 16; i++) {
        set_sensor_rom(i, "w");
    }

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    gpio_set_level(DECODER_SELECT_A, 0);
    gpio_set_level(DECODER_SELECT_B, 1);
    gpio_set_level(DECODER_SELECT_C, 0);
    //printf("Reading from third mux:\n");
    for(int i = 0; i < 8; i++) {
        set_sensor_rom(i, "b");
    }

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    gpio_set_level(DECODER_SELECT_A, 1);
    gpio_set_level(DECODER_SELECT_B, 1);
    gpio_set_level(DECODER_SELECT_C, 0);
    //printf("Reading from fourth mux:\n");
    for(int i = 8; i < 16; i++) {
        set_sensor_rom(i, "b");
    }
    int res1 = validateIDs(WhiteChessPieces);
    int res2 = validateIDs(BlackChessPieces);
    if(res1 == 1 && res2 == 1) {
        printf("Mapping is valid!\n");
    } else {
        printf("Mapping is invalid!\n");
        return 1;
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
        
    for(int i = 0; i < 16; i++) {
        char buffer_white[256]; // Allocate a buffer for the serialized data

        sendChessPiece(&WhiteChessPieces[i], buffer_white, sizeof(buffer_white));
        //uart_send_data("TX_TASK", buffer_white);
        array[i] = WhiteChessPieces[i].eeprom_id;
        AllPieces[i] = WhiteChessPieces[i];
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
    for(int i = 0; i < 16; i++) {
        char buffer_black[256]; // Allocate a buffer for the serialized data

        sendChessPiece(&BlackChessPieces[i], buffer_black, sizeof(buffer_black));
        //uart_send_data("TX_TASK", buffer_black);
        array[i + 16] = BlackChessPieces[i].eeprom_id;
        AllPieces[i + 16] = BlackChessPieces[i];
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);

    return 0;
}

void setup_peripherals(void) {
    select_pins_init();
    decoder_select_pins_init();
    ds18b20_init(ONE_WIRE_GPIO);
    led_initialize();

    vTaskDelay(100 / portTICK_PERIOD_MS);
    reset_board();
}

int load_game_state(Tile* tiles[8][8]) {
    int errors = 0;
    // Define the mux settings for each row, organized as A, B, C
    int muxSettings[4][3] = {
        {0, 1, 0},
        {1, 1, 0},
        {0, 0, 1},
        {1, 0, 1},
        {0, 1, 1},
        {1, 1, 1},
        {1, 0, 0},  // First mux
        {0, 0, 0},  // Second mux
    };

// Iterate through each mux setting
    for (int mux = 0; mux < 8; mux++) {
        gpio_set_level(DECODER_SELECT_A, muxSettings[mux][0]);
        gpio_set_level(DECODER_SELECT_B, muxSettings[mux][1]);
        gpio_set_level(DECODER_SELECT_C, muxSettings[mux][2]);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Read sensors and validate for each row controlled by this mux
        for (int i = 0; i < 8; i++) {
            int row = i % 8;

            // Read the EEPROM ID or sensor state
            unsigned long long eeprom_id = read_sensor_eeprom_id(i);

            if (tiles[row][i] != NULL && tiles[row][i]->piece != NULL && eeprom_id != 0) {
                tiles[row][i]->eeprom_id = eeprom_id;
                tiles[row][i]->piece->eeprom_id = eeprom_id;

            } else if (tiles[row][i]->piece != NULL && eeprom_id == 0) {
                errors++;
            } else {
                continue;
            }
        }
        
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (errors == 0) {
        printf("Board initialized and validated successfully.\n");
        return 0;
    } else {
        printf("Board validation failed with %d errors.\n", errors);
        return 1;
    }
}

void board_scan_task(void *pvParameters) {
    printf("Board scan task started\n");
    while (true) {
        xSemaphoreTake(scan_semaphore, portMAX_DELAY);
        while (allow_scanning) {
            vTaskDelay(500 / portTICK_PERIOD_MS); // Send every half seconds
            state = scanBoard(&rom_array, prevTiles, tiles, &total_pieces, &moveState);
            if (state == 1) {
                // Piece is lifted
                deepCopyTiles(intermediateTiles, prevTiles);
                char* liftedPiece = getSourceTileName(intermediateTiles, tiles);
                printf("Piece picked up at tile: %s\n", liftedPiece);
                char buffer[256];
                int bufferSize = sizeof(buffer);
                snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"tile\": \"%s\"}\n",
                        "tile", liftedPiece);
                uart_send_data("TX_TASK", buffer);
            } else if (state == 2) {
                //move no capture
                char* move = getMoveMade(intermediateTiles, tiles);
                if (move != NULL) {
                    printf("Move made: %s\n", move);
                    char buffer[256];
                    int bufferSize = sizeof(buffer);
                    snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"move\": \"%s\"}\n", 
                    "move", move);
                    uart_send_data("TX_TASK", buffer);
                } else {
                    char buffer[256];
                    int bufferSize = sizeof(buffer);
                    snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"move\": \"%s\"}\n",
                    "move", "No move");
                    uart_send_data("TX_TASK", buffer);
                }
                printTiles(intermediateTiles);
                printTiles(tiles);
                freeTiles(intermediateTiles);
            } else if (state == 4) {
                //move with capture
                char* move = getMoveMade(intermediateTiles, tiles);
                if (move != NULL) {
                    printf("Capture made: %s\n", move);
                    char buffer[256];
                    int bufferSize = sizeof(buffer);
                    snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"move\": \"%s\"}\n", 
                    "move", move);
                    uart_send_data("TX_TASK", buffer);
                
                }
                printTiles(intermediateTiles);
                printTiles(tiles);
                freeTiles(intermediateTiles);
            }
            freeTiles(prevTiles);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void listen_task(void *pvParameters) {
    uint8_t header;
    char data[1500];

    while (1) {
        memset(data, 0, sizeof(data));  // Clear the buffer before reading
        int headerLen = uart_read_bytes(UART_NUM_0, &header, 1, portMAX_DELAY);
        if (headerLen > 0) {
            int len = uart_read_bytes(UART_NUM_0, data, sizeof(data), pdMS_TO_TICKS(1000));
            if (len < 0 || len >= sizeof(data)) {
                printf("Invalid data length: %d\n", len);
                return;
            }
            data[len] = '\0';  // Null-terminate to safely print
            switch (header)
            {
            case HEADER_ERROR:
                allow_scanning = false;
                xSemaphoreGive(scan_semaphore);
                break;
            case HEADER_START:
                setup_peripherals();
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (tiles[i][j] == NULL) {
                            printf("Tile at [%d][%d] is uninitialized.\n", i, j);
                            return;
                        }
                    }
                }
                bool res = validateBoardState(tiles);
                if (res) {
                    char buffer[256];
                    int bufferSize = sizeof(buffer);
                    snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"type\": \"%s\", \"message\": \"%s\"}\n", 
                    "echo", "valid", "Board is valid");
                    uart_send_data("TX_TASK", buffer);
                    allow_scanning = true;
                } else {
                    char buffer[256];
                    int bufferSize = sizeof(buffer);
                    snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"title\": \"%s\", \"text\": \"%s\"}\n", 
                    "error", "Mapping Error at Start Game", "Mapping is invalid, please reset the board to initial positions and try again!");
                    uart_send_data("TX_TASK", buffer);
                    allow_scanning = false;
                }
                //printf("Received header: %d\n", header);
                //printf("Received data: %s\n", data);
                //uart_send_data("TX_TASK", data);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                xSemaphoreGive(scan_semaphore);
                //uart_send_data("TX_TASK", data);
                break;
            case HEADER_MOVE:
                allow_scanning = true;
                xSemaphoreGive(scan_semaphore);
                break;
            case HEADER_LEGAL:
                led_initialize();
                uart_send_data("TX_TASK", "Legal moves received");
                int led_matrix[8][8];
                //set_led_matrix(data, tiles, &led_matrix);
                //print_led_matrix(&led_matrix);
                int mat[8][8] = {
                    {0, 0, 0, 1, 1, 0, 0, 0}, 
                    {1, 1, 0, 0, 0, 0, 1, 1},
                    {1, 0, 0, 1, 1, 0, 0, 1}, 
                    {1, 1, 1, 1, 0, 0, 0, 0},
                    {0, 0, 0, 0, 1, 1, 1, 1}, 
                    {0, 1, 1, 0, 0, 1, 1, 0},
                    {0, 1, 0, 1, 0, 1, 0, 1}, 
                    {1, 0, 1, 0, 1, 0, 1, 0}
                };
                convertMatrixToHex(mat, &img);
                //uart_send_data("TX_TASK", img);
                allow_scanning = false;
                xSemaphoreGive(led_semaphore);
                break;
            case HEADER_SUGGESTION:
                allow_scanning = true;
                xSemaphoreGive(scan_semaphore);
                break;
            case HEADER_END:
                allow_scanning = false;
                xSemaphoreGive(scan_semaphore);
                break;
            case HEADER_LOAD:
                uart_send_data("TX_TASK", "\n{\"data\": \"load\", \"message\": \"Initializing pieces... Place all 32 pieces in starting position\"}");
                setup_peripherals();
                uart_send_data("TX_TASK", "\n{\"data\": \"load\", \"message\": \"Pieces Initialized. Loading game state... Place pieces in the shown positions\"}");
                parseChessBoard(data);
                int flag = 1;
                while(flag > 0) {
                    vTaskDelay(15000 / portTICK_PERIOD_MS);
                    uart_send_data("TX_TASK", "\n{\"data\": \"load\", \"message\": \"Pieces Initialized. Loading game state... Place pieces in the shown positions\"}");
                    flag = load_game_state(tiles);
                }
                uart_send_data("TX_TASK", "\n{\"data\": \"load-success\", \"message\": \"Loading complete. Game is ready to play\"}");
                allow_scanning = true;
                xSemaphoreGive(scan_semaphore);
            case HEADER_RESET:
                allow_scanning = false;
                xSemaphoreGive(scan_semaphore);
                esp_restart();
                break;
            default:
                allow_scanning = false;
                xSemaphoreGive(scan_semaphore);
                break;
            }
        } else {
            printf("No header received. Length: %d\n", headerLen);
        }
        
        // Reset watchdog if used
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void led_matrix_task(void *pvParameters) {
    if (xSemaphoreTake(led_semaphore, portMAX_DELAY) == pdTRUE) {
        led_loop(img);
    }
}

void parseChessBoard(const char* jsonData) {
    cJSON *root = cJSON_Parse(jsonData);
    if (root == NULL) return;

    for (int i = 0; i < 8; i++) {
        cJSON *row = cJSON_GetArrayItem(root, i);
        if (row == NULL) continue;

        for (int j = 0; j < 8; j++) {
            cJSON *item = cJSON_GetArrayItem(row, j);
            if (item == NULL || cJSON_IsNull(item)) {
                continue;
            }

            const char* square = cJSON_GetObjectItem(item, "square")->valuestring;
            const char* typeStr = cJSON_GetObjectItem(item, "type")->valuestring;
            const char* colorStr = cJSON_GetObjectItem(item, "color")->valuestring;

            ChessPiece* piece = malloc(sizeof(ChessPiece));
            piece->type = getPieceType(typeStr);
            piece->color = getPieceColor(colorStr);
            piece->eeprom_id = 0;  // Set this appropriately if you have this info

            Tile* tile = malloc(sizeof(Tile));
            tile->tileName = strdup(square);
            tile->row = i;
            tile->col = j;
            tile->eeprom_id = 0;  // Set this appropriately if you have this info
            tile->piece = piece;

            tiles[i][j] = tile;
        }
    }
    cJSON_Delete(root);
}

void reset_board(void) {
    for (int i = 0; i < 32; i++) {
        rom_array[i] = 0;
    }
    int res = init_mapping(rom_array, 32);
    if (res == 1) {
        printf("Mapping is invalid!\n");
        char buffer[256];
        int bufferSize = sizeof(buffer);
        snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"title\": \"%s\", \"text\": \"%s\"}\n",
        "error", "Mapping Error at Start Game", "Mapping is invalid, please reset the board to initial positions and try again!\n");
        uart_send_data("TX_TASK", buffer);
        return;
    }

    // for (int i = 0; i < 8; i++) {
    //     for (int j = 0; j < 8; j++) {
    //         tiles[i][j] = malloc(sizeof(Tile)); // Allocate each Tile structure
    //         if (tiles[i][j] == NULL) {
    //             printf("Error allocating memory for tile [%d][%d]\n", i, j);
    //         }
    //     }
    // }
    initTiles(tiles);
}


void app_main() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    // Check the return values of these functions for any error
    esp_err_t config_ok = uart_param_config(UART_NUM_0, &uart_config);
    esp_err_t driver_ok = uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);


    if (config_ok != ESP_OK || driver_ok != ESP_OK) {
        printf("UART setup failed!\n");
    }

    scan_semaphore = xSemaphoreCreateBinary();
    //led_semaphore = xSemaphoreCreateBinary();
    //xSemaphoreTake(led_semaphore, portMAX_DELAY);

    xTaskCreate(listen_task, "uart_listen_task", 4096, NULL, 10, NULL);
    xTaskCreate(board_scan_task, "board_scan_task", 4096, NULL, 10, NULL);
    //xTaskCreate(led_matrix_task, "led_task", 2048, NULL, 10, NULL);

}


