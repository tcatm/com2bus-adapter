// A C parser for com2bus that is fed with bytes by each call.
// There are two functions:
// - parse_start(uint8_t type): starts a new message
// - parse_byte(uint8_t byte): adds a byte to the current message

// The wire format is as follows:
// - 1 byte type
// - 1 byte address
// - 1 byte length
// - length bytes data
// - 2 byte crc (xmodem)

// The parser will return a message struct when a message is complete.
// The message struct contains the following fields:
// - type: the type byte of the message
// - address: the address byte of the message
// - length: the length byte of the message
// - data: the data bytes of the message
// - crc: the crc of the message (16bit)

// The parser will return NULL if no message is complete yet.
// The parser will return NULL if a message is complete but the crc is invalid.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "com2bus.h"

void parse_start(Parser *parser, uint8_t byte) {
    memset(&parser->msg, 0, sizeof(Message)); // Clear the message structure.
    parser->msg.type = byte; // Set the type byte.
    parser->bytes_parsed = 1;
}

Message *parse_byte(Parser *parser, uint8_t byte) {
    if (parser->bytes_parsed == 1) { // Second byte is the address.
        parser->msg.address = byte;
    } else if (parser->bytes_parsed == 2) { // Third byte is the length.
        parser->msg.length = byte;
    } else if (parser->bytes_parsed >= 3 && parser->bytes_parsed < (3 + parser->msg.length)) {
        // Subsequent bytes are data bytes.
        parser->msg.data[parser->bytes_parsed - 3] = byte;
    } else if (parser->bytes_parsed >= (3 + parser->msg.length) && parser->bytes_parsed < (5 + parser->msg.length)) {
        // Last two bytes after the data are CRC in big endian.
        if (parser->bytes_parsed == (3 + parser->msg.length))
                parser->msg.crc |= byte << 8;
        else if (parser->bytes_parsed == (4 + parser->msg.length))
                parser->msg.crc |= byte;
    } else {
        return NULL; // We have parsed the entire message, abort.
    }
    
    // Increment the parsed byte count.
    parser->bytes_parsed++;
    
    // If we have parsed the entire message, return 1.
    if (parser->bytes_parsed == (5 + parser->msg.length)) {
        parser->bytes_parsed = 0;
        return &parser->msg;
    }

    return NULL;
}

// XMODEM CRC-16 based on CRC_POLY
uint16_t crc16(uint8_t *data, size_t length) {
    uint16_t crc = 0;
    for (int i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC_POLY;
            else
                crc <<= 1;
        }
    }
    return crc;
}

uint16_t msg_crc(Message *msg) {
    uint8_t buffer[MAX_DATA_LENGTH + 5];
    serialize_message(msg, buffer);

    return crc16(buffer, 3 + msg->length);
}

int check_crc(Message *msg) {
    uint16_t crc = msg_crc(msg);
    return crc == msg->crc;
}

Message *deserialize_message(uint8_t *buffer) {
    Message *msg = malloc(sizeof(Message));
    
    msg->type = buffer[0];
    msg->address = buffer[1];
    msg->length = buffer[2];
    memcpy(msg->data, buffer + 3, msg->length);

    // deserialize CRC as big endian
    msg->crc = buffer[3 + msg->length] << 8;
    msg->crc |= buffer[4 + msg->length];

    return msg;
}

size_t serialize_message(Message *msg, uint8_t *buffer) {
    buffer[0] = msg->type;
    buffer[1] = msg->address;
    buffer[2] = msg->length;
    memcpy(buffer + 3, msg->data, msg->length);

    // serialize CRC as big endian
    buffer[3 + msg->length] = msg->crc >> 8;
    buffer[4 + msg->length] = msg->crc & 0xff;

    return 5 + msg->length;
}

void print_message(Message *msg) {
    printf("Message: type=%02x, address=%02x, length=%02x, data=[", msg->type, msg->address, msg->length);
    for (int i = 0; i < msg->length; i++) {
        printf("%02x", msg->data[i]);
        if (i < msg->length - 1)
            printf(", ");
    }
    printf("], crc=%04x ", msg->crc);

    // Valid in green, invalid in red.
    uint16_t crc = msg_crc(msg);
    if (crc == msg->crc)
        printf("\033[32mvalid\033[0m\r\n");
    else
        printf("\033[31minvalid (should be %04x)\033[0m\r\n", crc);
}