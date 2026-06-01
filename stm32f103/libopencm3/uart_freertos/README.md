# STM32F103 libopencm3 FreeRTOS UART example

FreeRTOS UART example for an STM32F103C8 target using libopencm3.

The firmware:

- runs FreeRTOS on the Cortex-M3 SysTick
- toggles `PC13` every 500 ms
- writes a message on `USART1` every second

USART1 uses the common Blue Pill pins:

- `PA9` as TX
- `PA10` as RX
- `115200 8N1`

## Build

Install an ARM embedded GCC toolchain that provides `arm-none-eabi-gcc`,
`arm-none-eabi-objcopy`, and related binutils.

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build --target uart_freertos
```

CMake fetches libopencm3 and the FreeRTOS kernel with `FetchContent`.

The build emits:

- `uart_freertos.elf`
- `uart_freertos.bin`
- `uart_freertos.hex`


## Test
```bash
$ minicom 
Welcome to minicom 2.9

OPTIONS: I18n 
Port /dev/ttyUSB0, 20:31:27

Press CTRL-A Z for help on special keys

[00:00:40.160] FreeRTOS UART example running on USART1
[00:00:41.164] FreeRTOS UART example running on USART1
[00:00:42.168] FreeRTOS UART example running on USART1
[00:00:43.172] FreeRTOS UART example running on USART1
[00:00:44.176] FreeRTOS UART example running on USART1

```