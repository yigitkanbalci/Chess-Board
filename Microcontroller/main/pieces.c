#include "pieces.h"

ChessPiece WhiteChessPieces[16]; // White pieces
ChessPiece BlackChessPieces[16]; //Black pieces
ChessPiece AllPieces[32];

ChessPiece* Board[8][8];

void initializeChessPieces() {
    // Init white piees
    WhiteChessPieces[0] = (ChessPiece){.type = ROOK, .color = WHITE, .eeprom_id = 0};
    WhiteChessPieces[1] = (ChessPiece){.type = KNIGHT, .color = WHITE, .eeprom_id = 0};
    WhiteChessPieces[2] = (ChessPiece){.type = BISHOP, .color = WHITE, .eeprom_id = 0};
    WhiteChessPieces[3] = (ChessPiece){.type = QUEEN, .color = WHITE, .eeprom_id = 0};
    WhiteChessPieces[4] = (ChessPiece){.type = KING, .color = WHITE, .eeprom_id = 0};
    WhiteChessPieces[5] = (ChessPiece){.type = BISHOP, .color = WHITE, .eeprom_id = 0};
    WhiteChessPieces[6] = (ChessPiece){.type = KNIGHT, .color = WHITE, .eeprom_id = 0};
    WhiteChessPieces[7] = (ChessPiece){.type = ROOK, .color = WHITE, .eeprom_id = 0};

    // Init black pieces
    BlackChessPieces[0] = (ChessPiece){.type = ROOK, .color = BLACK, .eeprom_id = 0};
    BlackChessPieces[1] = (ChessPiece){.type = KNIGHT, .color = BLACK, .eeprom_id = 0};
    BlackChessPieces[2] = (ChessPiece){.type = BISHOP, .color = BLACK, .eeprom_id = 0};
    BlackChessPieces[3] = (ChessPiece){.type = QUEEN, .color = BLACK, .eeprom_id = 0};
    BlackChessPieces[4] = (ChessPiece){.type = KING, .color = BLACK, .eeprom_id = 0};
    BlackChessPieces[5] = (ChessPiece){.type = BISHOP, .color = BLACK, .eeprom_id = 0};
    BlackChessPieces[6] = (ChessPiece){.type = KNIGHT, .color = BLACK, .eeprom_id = 0};
    BlackChessPieces[7] = (ChessPiece){.type = ROOK, .color = BLACK, .eeprom_id = 0};

    // Initialize pawn pieces
    for(int i = 0; i < 8; i++){
        WhiteChessPieces[i+8] = (ChessPiece){.type = PAWN, .color = WHITE, .eeprom_id = 0};
        BlackChessPieces[i+8] = (ChessPiece){.type = PAWN, .color = BLACK, .eeprom_id = 0};
    }
        
}

void initializeBoard(Tile* tiles[8][8]) {
    for (int i=0; i<1; i++) {
        for (int j=0; j<8; j++) {
            tiles[i][j]->piece = &BlackChessPieces[j];
            tiles[i][j]->eeprom_id = BlackChessPieces[j].eeprom_id;
            tiles[i + 1][j]->piece = &BlackChessPieces[j + 8];
            tiles[i + 1][j]->eeprom_id = BlackChessPieces[j + 8].eeprom_id;
        }
    }

    for (int i=7; i<8; i++) {
        for (int j=0; j<8; j++) {
            tiles[i][j]->piece = &WhiteChessPieces[j];
            tiles[i][j]->eeprom_id = WhiteChessPieces[j].eeprom_id;
            tiles[i - 1][j]->piece = &WhiteChessPieces[j + 8];
            tiles[i - 1][j]->eeprom_id = WhiteChessPieces[j + 8].eeprom_id;
        }
    }

    for (int i=2; i<6; i++) {
        for (int j=0; j<8; j++) {
            tiles[i][j]->piece = NULL;
            tiles[i][j]->eeprom_id = 0;
        }
    }

    //printBoard(Board);
}

void initTiles(Tile* tiles[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            tiles[i][j] = malloc(sizeof(Tile));
            if (tiles[i][j] != NULL) {
                // Allocate memory for the tileName
                tiles[i][j]->tileName = malloc(sizeof(char) * 3);
                if (tiles[i][j]->tileName != NULL) {
                    // Assign the tile name based on its position
                    sprintf(tiles[i][j]->tileName, "%c%d", 'a' + j, 8 - i);
                } else {
                    printf("Error allocating tile name\n");
                }
                // Initialize the other fields
                tiles[i][j]->row = i;
                tiles[i][j]->col = j;
                tiles[i][j]->piece = NULL;
                tiles[i][j]->eeprom_id = malloc(sizeof(uint64_t));;
                tiles[i][j]->eeprom_id = 0;
            } else {
                printf("Error allocating memory for a tile\n");
            }
        }
    }

    
    initializeBoard(tiles);
    printTiles(tiles);
}

void mapTiles(Tile* tiles[8][8], ChessPiece* board[8][8]){
    for(int i=0; i<8; i++){
        for(int j=0; j<8; j++){
            if(board[i][j] != NULL){
                tiles[i][j]->piece = board[i][j];
                tiles[i][j]->eeprom_id = board[i][j]->eeprom_id;
            }
        }
    }
}

void printBoard(ChessPiece* Board[8][8]) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(Board[i][j] == NULL) {
                printf(" - ");
            } else {
                DisplayChessPiece(*Board[i][j]);
            }
        }
    }
}

void printTiles(Tile* tiles[8][8]) {
    printf("\n");
    for(int i = 0; i < 8; i++) {
        printf("\n");
        for(int j = 0; j < 8; j++) {
            Tile* tile = tiles[i][j];
            if (tile->eeprom_id != 0) {
                printf("1 ");
            } else {
                printf("- ");

            }           
        }
    }
}

int isMoveMade(Tile* prevTiles[8][8], Tile* currTiles[8][8]) {
    int result = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (prevTiles[i][j]->eeprom_id != currTiles[i][j]->eeprom_id) {
                if (prevTiles[i][j]->eeprom_id != 0 && currTiles[i][j]->eeprom_id == 0) {
                    result = 1;
                } else {
                    result = 0;
                }
            }
        }
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (prevTiles[i][j]->eeprom_id != currTiles[i][j]->eeprom_id) {
                if (prevTiles[i][j]->eeprom_id == 0 && currTiles[i][j]->eeprom_id != 0) {
                    result = 1;
                } else {
                    result = 0;
                }
            }
        }
    }
    return result;
}

int countPieces(Tile* tiles[8][8]) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (tiles[i][j]->eeprom_id != 0) {
                count++;
            }
        }
    }
    return count;
}


char* getMoveMade(Tile* prevTiles[8][8], Tile* currTiles[8][8]) {
    char *startSquare = NULL, *endSquare = NULL;
    char* move = malloc(sizeof(char) * 6); // Allocate memory for move description
    if (!move) return NULL; // Check if memory allocation failed
    unsigned long long movedPieceID = 0;

    // Find the start square
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (prevTiles[i][j]->eeprom_id != 0 && currTiles[i][j]->eeprom_id == 0) {
                printf("from: %s\n", prevTiles[i][j]->tileName);
                movedPieceID = prevTiles[i][j]->eeprom_id;
                startSquare = prevTiles[i][j]->tileName;
                break; // Break the inner loop once the moved piece is found
            }
        }
        if (movedPieceID != 0) break; // Break the outer loop as well
    }

    // Find the end square, but only if a start square was found
    if (movedPieceID != 0) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (currTiles[i][j]->eeprom_id == movedPieceID) {
                    printf("to: %s\n", currTiles[i][j]->tileName);
                    endSquare = currTiles[i][j]->tileName;
                    break; // Break the inner loop once the end square is found
                }
            }
            if (endSquare != NULL) break; // Break the outer loop as well
        }
    }
    // Check if both start and end squares were found
    if (startSquare != NULL && endSquare != NULL) {
        sprintf(move, "%s-%s", startSquare, endSquare);
        printf("here4\n");
        return move;
    } else {
        printf("here5\n");
        free(move); // Free allocated memory if move is not completed
        return NULL;
    }
}


void assignEEPROMIDToPiece(uint64_t id, int pieceIndex, ChessPiece chessPieces[16]) {
    if (pieceIndex >= 0 && pieceIndex < 32) { // Validate index
        chessPieces[pieceIndex].eeprom_id = id;
    } else {
        // Handle invalid index error
        printf("Invalid piece index: %d\n", pieceIndex);
    }
}

bool validateBoardState(Tile* board[8][8]) {
    PieceType backRow[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    
    // Validate black pieces in the first two rows
    for (int i = 0; i < 8; i++) {
        if (!board[0][i] || !board[0][i]->piece || board[0][i]->piece->type != backRow[i] || board[0][i]->piece->color != BLACK) {
            printf("Mismatch in black back row at position %d\n", i);
            return false;
        }
        if (!board[1][i] || !board[1][i]->piece || board[1][i]->piece->type != PAWN || board[1][i]->piece->color != BLACK) {
            printf("Mismatch in black pawn row at position %d\n", i);
            return false;
        }
    }

    // Validate white pieces in the last two rows
    for (int i = 0; i < 8; i++) {
        if (!board[7][i] || !board[7][i]->piece || board[7][i]->piece->type != backRow[i] || board[7][i]->piece->color != WHITE) {
            printf("Mismatch in white back row at position %d\n", i);
            return false;
        }
        if (!board[6][i] || !board[6][i]->piece || board[6][i]->piece->type != PAWN || board[6][i]->piece->color != WHITE) {
            printf("Mismatch in white pawn row at position %d\n", i);
            return false;
        }
    }

    // Optionally check that rows 2 to 5 are empty
    for (int row = 2; row < 6; row++) {
        for (int col = 0; col < 8; col++) {
            if (board[row][col] && board[row][col]->piece && board[row][col]->piece->type != EMPTY) {
                printf("Unexpected piece at row %d, column %d\n", row, col);
                return false;
            }
        }
    }

    printf("All initial positions are correct.\n");
    return true;  // All checks passed
}

PieceType getPieceType(const char* typeStr) {
    if (strcmp(typeStr, "p") == 0) return PAWN;
    if (strcmp(typeStr, "n") == 0) return KNIGHT;
    if (strcmp(typeStr, "b") == 0) return BISHOP;
    if (strcmp(typeStr, "r") == 0) return ROOK;
    if (strcmp(typeStr, "q") == 0) return QUEEN;
    if (strcmp(typeStr, "k") == 0) return KING;
    return EMPTY;
}

PieceColor getPieceColor(const char* colorStr) {
    if (strcmp(colorStr, "b") == 0) return BLACK;
    if (strcmp(colorStr, "w") == 0) return WHITE;
    return NONE;
}


bool deepCopyTiles(Tile* copy[8][8], Tile* original[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (original[i][j] == NULL) {
                // Handle original tile being NULL
                printf("Original tile at [%d][%d] is NULL\n", i, j);
                return false; // Return an error status
            }

            // Allocate memory for a new Tile object
            copy[i][j] = malloc(sizeof(Tile));
            if (copy[i][j] == NULL) {
                printf("Error allocating memory for a Tile at [%d][%d]\n", i, j);
                // Cleanup logic here (free previously allocated Tiles)
                return false; // Return an error status
            }

            // Copy simple fields
            copy[i][j]->eeprom_id = original[i][j]->eeprom_id;
            copy[i][j]->piece = original[i][j]->piece; // Shallow copy; deep copy if necessary
            copy[i][j]->row = original[i][j]->row;
            copy[i][j]->col = original[i][j]->col;
            
            // Deep copy for tileName, if it's dynamically allocated
            if (original[i][j]->tileName != NULL) {
                copy[i][j]->tileName = malloc(strlen(original[i][j]->tileName) + 1);
                if (copy[i][j]->tileName == NULL) {
                    printf("Error allocating memory for tileName at [%d][%d]\n", i, j);
                    // Cleanup logic here (free previously allocated Tiles and tileNames)
                    return false;
                }
                strcpy(copy[i][j]->tileName, original[i][j]->tileName);
            } else {
                copy[i][j]->tileName = NULL;
            }
        }
    }
    return true; // Success
}

void freeTiles(Tile* tiles[8][8]) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if (tiles[i][j]->tileName != NULL) {
                free(tiles[i][j]->tileName); // Free dynamically allocated tileName
            }
            free(tiles[i][j]); // Free the Tile structure itself
            tiles[i][j] = NULL; // Prevent dangling pointer by setting it to NULL
        }
    }
}

char* getSourceTileName(Tile* prevTiles[8][8], Tile* currTiles[8][8]) {
    char* startSquare = NULL;
    unsigned long long movedPieceID = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (prevTiles[i][j]->eeprom_id != 0 && currTiles[i][j]->eeprom_id == 0) {
                printf("from: %s\n", prevTiles[i][j]->tileName);
                movedPieceID = prevTiles[i][j]->eeprom_id;
                startSquare = prevTiles[i][j]->tileName;
                break; // Break the inner loop once the moved piece is found
            }
        }
        if (movedPieceID != 0) break; // Break the outer loop as well
    }

    return startSquare;
}


ChessPiece* findPieceByEEPROMID(uint64_t id, ChessPiece chessPieces[32]) {
    for (int i = 0; i < 32; i++) {
        if (chessPieces[i].eeprom_id == id) {
            return &chessPieces[i];
        }
    }
    return NULL; // Not found
}

ChessPiece* findPieceInBoardByEEPROMID(uint64_t id, ChessPiece* board[8][8]) {
    for(int i=0; i<8; i++) {
        for(int j=0; j<8; j++) {
            if(board[i][j] != NULL && board[i][j]->eeprom_id == id) {
                return board[i][j];
            }
        }
    }

    return NULL;
}

void DisplayChessPiece(ChessPiece piece){
    printf("\nPiece Type: %s", PieceTypeToString(piece.type));
    printf("\nPiece Color: %s", PieceColorToString(piece.color));
    printf("\nEEPROM ID: %llu\n", piece.eeprom_id); // Use %llu for printing uint64_t
}

// Function to convert PieceType to string
const char* PieceTypeToString(PieceType type) {
    switch (type) {
        case PAWN: return "Pawn";
        case KNIGHT: return "Knight";
        case BISHOP: return "Bishop";
        case ROOK: return "Rook";
        case QUEEN: return "Queen";
        case KING: return "King";
        case EMPTY: return "Empty";
        default: return "Unknown";
    }
}

// Function to convert PieceColor to string
const char* PieceColorToString(PieceColor color) {
    switch (color) {
        case BLACK: return "Black";
        case WHITE: return "White";
        case NONE: return "None";
        default: return "Unknown";
    }
}

int validateIDs(ChessPiece chessPieces[16]) {
    int valid = 1;
    for (int i = 0; i < 16; i++) {
        for (int j = i + 1; j < 16; j++) {
            if (chessPieces[i].eeprom_id == chessPieces[j].eeprom_id) {
                printf("Error: EEPROM ID %llu is duplicated.\n", chessPieces[i].eeprom_id);
                valid = 0;
            }
        }
        if (chessPieces[i].eeprom_id == 0) {
            valid = 0;
        }
    }
    return valid;
}

Tile* findTileByPiece(ChessPiece* piece, Tile* tiles[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (tiles[i][j]->piece->eeprom_id == piece->eeprom_id) {
                return tiles[i][j];
            }
        }
    }

  return NULL;
}

void sendChessPiece(const ChessPiece* piece, char* buffer, size_t bufferSize) {
    if (buffer != NULL) {
        snprintf(buffer, bufferSize, "\n{\"data\": \"%s\", \"type\": \"%d\", \"color\": \"%d\", \"eeprom_id\": \"%llu\"}\n", 
                "piece", piece->type, piece->color, piece->eeprom_id);
    }
}

Tile* findTileByName(char* tileName, Tile* tiles[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (strcmp(tiles[i][j]->tileName, tileName) == 0) {
                return tiles[i][j];
            }
        }
    }

    return NULL;
}

void set_led_matrix(char* data, Tile* tiles[8][8], int* led_matrix[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            led_matrix[i][j] = 0;
        }
    }

    char delimiters[] = ", ";
    char tokens[MAX_TOKENS][TOKEN_SIZE];
    int tokenCount = 0;
    char* token = strtok(data, delimiters);
    while (token != NULL && tokenCount < MAX_TOKENS) {
        strncpy(tokens[tokenCount], token, TOKEN_SIZE - 1);
        tokens[tokenCount][TOKEN_SIZE - 1] = '\0';  // Ensure null-termination
        tokenCount++;
        token = strtok(NULL, delimiters);
    }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < tokenCount; k++) {
                if(tiles[i][j] != NULL) {
                    if (strcmp(tiles[i][j]->tileName, tokens[k]) == 0) {
                        led_matrix[i][j] = 1;
                    }
                }
            }
        }
    }

}

void print_led_matrix(int* led_matrix[8][8]) {
    for (int i = 0; i < 8; i++) {
        printf("\n");
        for (int j = 0; j < 8; j++) {
            if (led_matrix[i][j] == 1) {
                printf("1 ");
            } else {
                printf("0 ");
            }
        }
    }
}

