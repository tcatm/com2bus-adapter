#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <hardware/gpio.h>

#include "com2bus.h"

#define UART_ID uart1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// Global message buffer
Parser com2bus_parser = {.bytes_parsed = 0};
Parser frontend_parser = {.bytes_parsed = 0};

Message no_response = {
    .type = 0x6c,
    .address = 0xff,
    .length = 0x02,
    .data = {0x00, 0xff},
    .crc = 0xffff
};

// A queue of up to 100 messages
Message message_queue[100];
int message_queue_length = 0;

// A list of up to 10 addresses that have been seen
uint8_t seen_addresses[10];
int seen_addresses_length = 0;

int is_seen_address(uint8_t address) {
    for (int i = 0; i < seen_addresses_length; i++) {
        if (seen_addresses[i] == address) {
            return 1;
        }
    }

    return 0;
}

// Add an address to the seen list, but only if it's not already there
void add_seen_address(uint8_t address) {
    // Check limit
    if (seen_addresses_length == 10) {
        return;
    }

    if (!is_seen_address(address)) {
        seen_addresses[seen_addresses_length++] = address;
    }
}

// RX interrupt handler
void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);

        // check parity
        io_rw_32 dr = uart_get_hw(UART_ID)->dr;

        // check bit 9, invert
        int start = (dr & (1 << 9)) == 0;

        if (start) {
            parse_start(&com2bus_parser, ch);
        } else {
            Message *msg = parse_byte(&com2bus_parser, ch);

            if (msg && check_crc(msg)) {
                uint8_t buffer[sizeof(Message) + 1];

                // if message type is 0x4c and payload is 0x00, respond with message from queue or 0x00, 0xff

                // first check if we've seen this address before

                if (msg->type == 0x4c) {
                    Message *response = NULL;

                    // check if we've seen this address before
                    if (is_seen_address(msg->address)) {
                        // find first message in queue with matching address
                        for (int i = 0; i < message_queue_length; i++) {
                            if (message_queue[i].address == msg->address) {
                                response = &message_queue[i];

                                // shift queue
                                for (int j = i; j < message_queue_length - 1; j++) {
                                    message_queue[j] = message_queue[j + 1];
                                }

                                message_queue_length--;
                                break;
                            }
                        }

                        if (response == NULL) {
                            // respond with 0x00, 0xff
                            response = &no_response;
                            response->address = msg->address;
                            response->crc = crc16((uint8_t *)response, 3 + response->length);
                        }

                        // Send response
                        size_t length = serialize_message(response, buffer);

                        uart_tx_wait_blocking(UART_ID);

                        uart_putc_raw(UART_ID, buffer[0]);

                        uart_tx_wait_blocking(UART_ID);

                        hw_set_bits(&uart_get_hw(UART_ID)->lcr_h, UART_UARTLCR_H_EPS_BITS);

                        for (int i = 1; i < length; i++) {
                            uart_putc_raw(UART_ID, buffer[i]);
                        }
                        
                        uart_tx_wait_blocking(UART_ID);

                        hw_clear_bits(&uart_get_hw(UART_ID)->lcr_h, UART_UARTLCR_H_EPS_BITS);
                    }
                }

                size_t length = serialize_message(msg, buffer);

                for (int i = 0; i < length; i++) {
                    putchar(buffer[i]);
                }
            }
        }
    }
}

int setup_uart() {
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 9600);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    hw_write_masked(&uart_get_hw(UART_ID)->lcr_h,
                   (1 << UART_UARTLCR_H_SPS_LSB) | (1 << UART_UARTLCR_H_PEN_LSB),
                   UART_UARTLCR_H_SPS_BITS | UART_UARTLCR_H_PEN_BITS);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}


int main(void)
{
    stdio_init_all();

    setup_uart();

    while (1) {
        // Read from stdin
        int ch = getchar();
        if (frontend_parser.bytes_parsed == 0) {
            parse_start(&frontend_parser, ch);
        } else {
            Message *msg = parse_byte(&frontend_parser, ch);

            if (msg && check_crc(msg) && msg->type == 0x6c) {
                // Copy to queue
                memcpy(&message_queue[message_queue_length++], msg, sizeof(Message));

                // Add address to seen list
                add_seen_address(msg->address);
            }
        }
    }

    return 0;
}
