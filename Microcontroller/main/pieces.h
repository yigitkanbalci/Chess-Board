// pieces.h
#ifndef PIECES_H
#define PIECES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, EMPTY } PieceType;
typedef enum { BLACK, WHITE, NONE } PieceColor;

typedef struct {
    PieceType type;
    PieceColor color;
    uint64_t eeprom_id;
} ChessPiece;

typedef struct {
    char* tileName;
    int row;
    int col;
    uint64_t eeprom_id;
    ChessPiece* piece;
} Tile;

#define MAX_TOKENS 64
#define TOKEN_SIZE 3

extern ChessPiece WhiteChessPieces[16]; // White pieces
extern ChessPiece BlackChessPieces[16]; //Black pieces
extern ChessPiece AllPieces[32]; //All pieces

void initializeChessPieces(void);
void assignEEPROMIDToPiece(uint64_t id, int pieceIndex, ChessPiece chessPieces[16]);
ChessPiece* findPieceByEEPROMID(uint64_t id, ChessPiece chessPieces[32]);
const char* PieceTypeToString(PieceType type);
const char* PieceColorToString(PieceColor color);
void DisplayChessPiece(ChessPiece piece);
int validateIDs(ChessPiece chessPieces[16]);
void sendChessPiece(const ChessPiece* piece, char* buffer, size_t bufferSize);
void initializeBoard(Tile* tiles[8][8]);
void printBoard(ChessPiece* Board[8][8]);
void movePiece(int startX, int startY, int endX, int endY);
void initTiles(Tile* tiles[8][8]);
void printTiles(Tile* tiles[8][8]);
int* getIndexes(char* moveString, Tile* tiles[8][8]);
Tile* findTileByPiece(ChessPiece* piece, Tile* tiles[8][8]);
ChessPiece* findPieceInBoardByEEPROMID(uint64_t id, ChessPiece* board[8][8]);
void mapTiles(Tile* tiles[8][8], ChessPiece* board[8][8]);
Tile* findTileByName(char* tileName, Tile* tiles[8][8]);
char* getMoveMade(Tile* prevTiles[8][8], Tile* currTiles[8][8]);
int isMoveMade(Tile* prevTiles[8][8], Tile* currTiles[8][8]);
bool deepCopyTiles(Tile* prevTiles[8][8], Tile* currTiles[8][8]);
void freeTiles(Tile* tiles[8][8]);
int countPieces(Tile* tiles[8][8]);
char* getSourceTileName(Tile* prevTiles[8][8], Tile* currTiles[8][8]);
bool validateBoardState(Tile* board[8][8]);
void set_led_matrix(char* data, Tile* tiles[8][8], int* led_matrix[8][8]);
void print_led_matrix(int* led_matrix[8][8]);
PieceType getPieceType(const char* typeStr);
PieceColor getPieceColor(const char* colorStr);
// char* getMove(Tile* prevTiles[8][8], Tile* currTiles[8][8]);

#endif
