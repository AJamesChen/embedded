# STM32F103 libopencm3 FreeRTOS blinky

Minimal FreeRTOS bring-up for an STM32F103C8 target using libopencm3.

The firmware:

- configures the board for a 72 MHz system clock
- lets FreeRTOS own SysTick
- runs two simple tasks
- toggles the common Blue Pill LED on `PC13`

## Build

```sh
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build-arm --target rtos_blinky
```

The build emits:

- `rtos_blinky.elf`
- `rtos_blinky.bin`
- `rtos_blinky.hex`
