cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(Pico_Telenot_Adapter)
pico_sdk_init()

add_executable(main main.c com2bus)

target_compile_definitions(main PRIVATE
	PICO_DEFAULT_UART_TX_PIN=16
	PICO_DEFAULT_UART_RX_PIN=17
)

# enable usb output, disable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(main)

target_link_libraries(main pico_stdlib hardware_uart hardware_irq hardware_gpio)