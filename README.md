# Com2Bus Interface for Raspberry Pi Pico

This project provides a comprehensive solution for interfacing the proprietary com2bus protocol of Telenot's systems to a USB UART, specifically tailored for the Raspberry Pi Pico. It utilizes an UART-RS485 converter to facilitate the communication between Telenot's devices and a computer, simplifying the integration process.

## Hardware Requirements

- Raspberry Pi Pico
- UART-RS485 converter module

## Software Requirements

- Raspberry Pi Pico C/C++ SDK

## Installation

1. Clone this repository to your local machine.
2. Ensure you have the required hardware set up as per the schematics provided.
3. Build the project using the provided Makefile.
4. Flash the generated `.uf2` file to your Raspberry Pi Pico.

## Usage

1. Connect the Raspberry Pi Pico to the Telenot device using the UART-RS485 converter.
2. Run the provided software.
3. Interface from your computer through the USB port to send and receive com2bus messages.

## API Reference

The project consists of a parser for com2bus messages, message serialization/deserialization functions, and a simple queuing system for managing incoming and outgoing messages.

- `parse_start(Parser *parser, uint8_t byte)`: Initializes a new message parsing sequence.
- `parse_byte(Parser *parser, uint8_t byte)`: Parses a byte and updates the message state.
- `serialize_message(Message *msg, uint8_t *buffer)`: Serializes a message into a byte buffer.
- `check_crc(Message *msg)`: Validates the CRC of a message.

# Wire Format

The communication protocol utilizes a specific packet structure, known as the wire format, for sending and receiving messages over the com2bus network. Each packet is structured as follows:

- **Type (1 byte):** Indicates the type of the message being sent or received.
- **Address (1 byte):** The address byte is used to identify the target device or origin of the message.
- **Length (1 byte):** Specifies the length of the data field in the packet.
- **Data (variable length):** The actual data being transmitted. The length of this field is determined by the Length byte.
- **CRC (2 bytes):** A cyclic redundancy check value used for error-checking the integrity of the packet.

## Detailed Breakdown

1. **Type:** The first byte of the packet that defines the type of message.
2. **Address:** The second byte that represents the unique address of the device in the network.
3. **Length:** The third byte that specifies how many bytes of data follow the Length byte itself.
4. **Data:** The data bytes follow immediately after the Length byte. The number of data bytes is equal to the value specified by the Length byte.
5. **CRC:** The final two bytes are the CRC checksum, which is calculated using the XMODEM CRC-16 algorithm. The CRC is transmitted in big-endian format.

## Example

For a message with a type of `0x6C`, an address of `0xFF`, a data length of `0x02`, and data bytes of `0x00` and `0xFF`, the packet would be structured as follows:

| Type | Address | Length | Data       | CRC          |
|------|---------|--------|------------|--------------|
| 6C   | FF      | 02     | 00 FF      | (CRC value)  |

The CRC value is computed over the type, address, length, and data fields and appended at the end of the packet.

## Parsing and Serialization

The provided `parse_start` and `parse_byte` functions in the `com2bus.c` file are used to parse incoming bytes into this packet structure, and the `serialize_message` function is used to serialize a `Message` structure back into a byte stream for transmission.

When implementing or interfacing with this protocol, it is crucial to adhere to this wire format to ensure correct communication between devices on the com2bus network.


## Contributing

Contributions to this project are welcome. Please follow the standard GitHub pull request process to propose any changes.

## License

This project is licensed under the [MIT License](LICENSE).

## Disclaimer

This project is not affiliated with, authorized by, endorsed by, or in any way officially connected with Telenot, or any of its subsidiaries or its affiliates. The official Telenot website can be found at [https://www.telenot.com](https://www.telenot.com).


