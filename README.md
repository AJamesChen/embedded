# Embedded STM32 Examples

This repository contains small embedded firmware examples for STM32F103 targets
using libopencm3.

Current examples:

- `gpio_blink`: bare-metal `PC13` LED blink
- `rtos_blinky`: minimal FreeRTOS task scheduling
- `uart_freertos`: FreeRTOS task output on `USART1`
- `usb_cdc_acm_baremetal`: bare-metal USB CDC ACM echo
- `usb_cdc_acm_freertos`: USB CDC ACM echo using FreeRTOS stream buffers

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
sudo apt install cmake make gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-dev libnewlib-arm-none-eabi stlink-tools
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

Build one firmware target:

```bash
cmake --build build-arm --target gpio_blink
```

Other available targets:

```bash
cmake --build build-arm --target rtos_blinky
cmake --build build-arm --target uart_freertos
cmake --build build-arm --target usb_cdc_acm_baremetal
cmake --build build-arm --target usb_cdc_acm_freertos
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

### `Couldn't find any ST-Link devices`

If `st-flash` cannot find ST-LINK but `lsusb` shows `0483:3748`, Linux can see
the USB programmer but the current user may not have permission to open it:

```bash
lsusb | grep -i -E 'stlink|0483'
sudo st-info --probe
sudo st-info --chipid
```

If `sudo st-info --chipid` returns `0x0410`, flash with `sudo` or install udev
rules for ST-LINK access:

```bash
sudo st-flash write build-arm/stm32f103/libopencm3/usb_cdc_acm_freertos/usb_cdc_acm_freertos.bin 0x08000000
```

Use the full flash base address `0x08000000`. A shortened address such as
`0x0800000` is outside the known STM32 flash region and `st-flash` will print
`Unknown memory region`.

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

### USB CDC does not create `/dev/ttyACM0`

`/dev/ttyACM0` appears only after the STM32 native USB port enumerates as CDC
ACM. It is separate from the ST-LINK USB port.

Useful checks:

```bash
sudo dmesg -w
ls -l /dev/ttyACM*
```

Root causes found during bring-up:

- The firmware must actually be flashed. `st-flash` may fail because of missing
  permissions, wrong working directory, or a mistyped address.
- On Blue Pill style boards, `PA12`/USB D+ may need a boot-time disconnect
  pulse. The examples pull `PA12` low briefly and then release it back to input
  floating before enabling USB.
- The FreeRTOS USB service task must poll `usbd_poll()` frequently during
  enumeration. A 1 ms task delay caused Linux `error -71` configuration
  descriptor failures on the tested host. The RTOS USB example now polls USB and
  uses `taskYIELD()` instead of sleeping in the USB task.

If Linux reports `device descriptor read/all, error -71` or `can't read
configurations, error -71`, test the bare-metal USB example as a control:

```bash
sudo st-flash write build-arm/stm32f103/libopencm3/usb_cdc_acm_baremetal/usb_cdc_acm_baremetal.bin 0x08000000
```

If both bare-metal and FreeRTOS USB examples fail, check the USB cable, native
USB connector, `PA11`/`PA12` wiring, D+ pull-up, and 8 MHz crystal.

### Testing USB CDC echo

Find the active ACM device from `dmesg`:

```bash
sudo dmesg | tail -40
```

Example:

```text
Product: STM32F103 CDC ACM FreeRTOS
cdc_acm ... ttyACM1: USB ACM device
```

Use the reported path, such as `/dev/ttyACM1`.

Terminal 1:

```bash
sudo cat /dev/ttyACM1
```

Terminal 2:

```bash
printf 'hello usb\r\n' | sudo tee /dev/ttyACM1 > /dev/null
```

Terminal 1 should print:

```text
hello usb
```

Minicom can also be used:

```bash
sudo minicom -D /dev/ttyACM1 -b 115200
```

`/dev/ttyACM*` is USB CDC virtual serial, not STM32 `USART1`. If minicom shows
characters twice, toggle local echo with `Ctrl-A` then `E`.
