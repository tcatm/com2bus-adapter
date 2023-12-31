// com2bus.h

#pragma once

#define MAX_DATA_LENGTH 256

#define CRC_POLY 0x1021

// Order matters for CRC computation
typedef struct {
    uint8_t type;
    uint8_t address;
    size_t length;
    uint8_t data[MAX_DATA_LENGTH];
    uint16_t crc;
} Message;

typedef struct {
    size_t bytes_parsed;
    Message msg;
} Parser;

// Function prototypes
void parse_start(Parser *parser, uint8_t type);
Message *parse_byte(Parser *parser, uint8_t byte);
Message *deserialize_message(uint8_t *buffer);
size_t serialize_message(Message *msg, uint8_t *buffer);

// CRC computation function prototype
uint16_t crc16(uint8_t *data, size_t length);
int check_crc(Message *msg);

void print_message(Message *msg);