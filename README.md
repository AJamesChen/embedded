# Embedded STM32 Examples

This repository contains small embedded firmware examples. The current project is
an STM32F103/libopencm3 GPIO blink firmware for an STM32F1 medium-density target
such as an STM32F103C8/CB board.

## Hardware

The verified target was detected as:

```text
Chip ID: 0x0410
Device:  STM32F1xx_MD
Flash:   0x20000 = 128 KiB
SRAM:    0x5000 = 20 KiB
```

Typical ST-LINK SWD wiring:

```text
ST-LINK GND   -> STM32 GND
ST-LINK SWDIO -> STM32 PA13 / SWDIO
ST-LINK SWCLK -> STM32 PA14 / SWCLK
ST-LINK 3.3V  -> STM32 3.3V, if powering the board from ST-LINK
ST-LINK NRST  -> STM32 NRST / RST, optional but useful for recovery
```

If the board is powered separately, still connect ST-LINK GND to the board GND.

## Tools

Install the ARM toolchain, CMake, make, and ST-LINK tools. On Debian/Ubuntu:

```bash
sudo apt install cmake make gcc-arm-none-eabi stlink-tools
```

Check the tools:

```bash
which arm-none-eabi-gcc
which cmake
which st-info
which st-flash
```

## Build

Configure a fresh build directory with the ARM toolchain file:

```bash
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
```

Build the blink firmware:

```bash
cmake --build build-arm --target gpio_blink
```

Generated firmware files:

```text
build-arm/stm32f103/libopencm3/gpio_blink/gpio_blink.elf
build-arm/stm32f103/libopencm3/gpio_blink/gpio_blink.bin
build-arm/stm32f103/libopencm3/gpio_blink/gpio_blink.hex
```

If you change `src/main.c`, rebuild before flashing:

```bash
cmake --build build-arm --target gpio_blink
```

## GitHub Actions

The firmware build runs in GitHub Actions on pushes, pull requests, and manual
workflow dispatches. The workflow is defined in:

```text
.github/workflows/firmware.yml
```

It installs the ARM embedded toolchain, configures CMake, builds `gpio_blink`,
prints the firmware size, and uploads these files as the `gpio_blink-stm32f103`
artifact:

```text
gpio_blink.elf
gpio_blink.bin
gpio_blink.hex
```

## Flash

First check that ST-LINK can see the target:

```bash
st-info --chipid
```

Expected result for this board:

```text
0x0410
```

Flash the binary:

```bash
st-flash write build-arm/stm32f103/libopencm3/gpio_blink/gpio_blink.bin 0x08000000
```

Success looks like:

```text
Flash written and verified!
```

Press the board reset button if the firmware does not start immediately.

## Backup Existing Flash

Before overwriting a board, you can read back flash. For example, read the first
1 KiB:

```bash
st-flash read backup.bin 0x08000000 1024
```

The output file is written to the current working directory. Use `pwd` to check
where you are.

## Troubleshooting

### `CMAKE_TOOLCHAIN_FILE` was not used

CMake only uses `CMAKE_TOOLCHAIN_FILE` the first time a build directory is
configured. If you see:

```text
Manually-specified variables were not used by the project:
  CMAKE_TOOLCHAIN_FILE
```

use a fresh build directory:

```bash
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build-arm --target gpio_blink
```

### `open(...gpio_blink.bin) == -1`

The binary path is wrong for your current directory. Either `cd` to the repository
root or use an absolute path:

```bash
cd /home/jamesc/git/skills/embedded
st-flash write build-arm/stm32f103/libopencm3/gpio_blink/gpio_blink.bin 0x08000000
```

### `st-info --chipid` returns `0x0000`

ST-LINK is connected over USB, but it cannot see the STM32 over SWD. Check:

```text
Target board is powered
ST-LINK GND is connected to STM32 GND
SWDIO is connected to PA13 / SWDIO
SWCLK is connected to PA14 / SWCLK
Wires are not loose or swapped
```

Then power-cycle the board and retry:

```bash
st-info --chipid
```

You can also try a slower SWD clock:

```bash
st-info --freq=100 --chipid
```

### `Can not connect to target`

If normal flashing fails:

```bash
st-flash write build-arm/stm32f103/libopencm3/gpio_blink/gpio_blink.bin 0x08000000
```

connect ST-LINK `NRST` to the STM32 `NRST/RST` pin, then use connect-under-reset:

```bash
st-flash --connect-under-reset write build-arm/stm32f103/libopencm3/gpio_blink/gpio_blink.bin 0x08000000
```

If `NRST` is not connected, `st-flash` may print:

```text
NRST is not connected
Soft reset failed
```

In that case, wire `NRST` or try manual reset timing: hold the board reset
button, start the `st-flash write` command, then release reset just after the
command starts.

### Firmware flashed but LED does not blink

Check these first:

```text
The flashed `.bin` is newer than `src/main.c`
The LED is actually connected to PC13
The LED polarity may be inverted on the board
The board was reset after flashing
```

The current blink code toggles `GPIOC, GPIO13`. With `delay_ms(2000)`, the LED
changes state every 2 seconds, so a full on/off cycle takes about 4 seconds.
