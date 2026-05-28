# STM32F103 libopencm3 GPIO blink

Minimal GPIO blink example for an STM32F103C8 target using libopencm3.

The example toggles `PC13`, which is the onboard LED pin on many STM32F103
Blue Pill boards.

## Build

Install an ARM embedded GCC toolchain that provides `arm-none-eabi-gcc`,
`arm-none-eabi-objcopy`, and related binutils.

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build --target gpio_blink
```

CMake fetches libopencm3 with `FetchContent` during configure.

The build emits:

- `gpio_blink.elf`
- `gpio_blink.bin`
- `gpio_blink.hex`
