# STM32F103 libopencm3 USB CDC ACM bare-metal echo

Bare-metal USB serial echo example for an STM32F103C8 target using libopencm3.

The firmware:

- configures the board for 72 MHz SYSCLK and 48 MHz USB
- enumerates as a USB CDC ACM serial port
- echoes bytes received on the virtual serial port
- toggles `PC13` while polling USB

The example pulses `PA12` low during boot to force a USB disconnect/reconnect on
Blue Pill style boards where the USB pull-up is tied to D+.

## Build

```sh
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build-arm --target usb_cdc_acm_baremetal
```

The build emits:

- `usb_cdc_acm_baremetal.elf`
- `usb_cdc_acm_baremetal.bin`
- `usb_cdc_acm_baremetal.hex`

## Test

After flashing, reconnect USB and open the new serial port at any baud rate.
CDC ACM line coding requests are accepted, but the baud rate does not affect USB
traffic.

```sh
sudo st-flash write build-arm/stm32f103/libopencm3/usb_cdc_acm_baremetal/usb_cdc_acm_baremetal.bin 0x08000000
sudo st-flash reset
ls -l /dev/ttyACM*
screen /dev/ttyACM0 115200
```

The active device may not be `/dev/ttyACM0`. Check `dmesg` after reconnecting
the STM32 USB port and use the ACM number reported by `cdc_acm`:

```sh
sudo dmesg | tail -40
```

Two-terminal echo test:

```sh
sudo cat /dev/ttyACM1
```

In another terminal:

```sh
printf 'hello usb\r\n' | sudo tee /dev/ttyACM1 > /dev/null
```

The first terminal should print `hello usb`.

Minicom test:

```sh
sudo minicom -D /dev/ttyACM1 -b 115200
```

If text appears twice, toggle minicom local echo with `Ctrl-A` then `E`.

If Linux reports `error -71` while enumerating, use this bare-metal example as a
control for the FreeRTOS version. If both examples fail, check the USB cable,
native USB connector, `PA11`/`PA12`, D+ pull-up, and 8 MHz crystal.
