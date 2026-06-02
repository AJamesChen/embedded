# STM32F103 libopencm3 USB CDC ACM FreeRTOS echo

FreeRTOS USB serial echo example for an STM32F103C8 target using libopencm3.

The firmware:

- configures the board for 72 MHz SYSCLK and 48 MHz USB
- enumerates as a USB CDC ACM serial port
- runs a USB service task that owns all `usbd_*` calls
- moves received bytes through FreeRTOS stream buffers
- echoes bytes from a worker task
- toggles `PC13` from a separate LED task

The example pulses `PA12` low during boot to force a USB disconnect/reconnect on
Blue Pill style boards where the USB pull-up is tied to D+.

## Build

```sh
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build-arm --target usb_cdc_acm_freertos
```

The build emits:

- `usb_cdc_acm_freertos.elf`
- `usb_cdc_acm_freertos.bin`
- `usb_cdc_acm_freertos.hex`

## Test

After flashing, reconnect USB and open the new serial port at any baud rate.
Typed bytes should echo back through the CDC ACM port.

```sh
sudo st-flash write build-arm/stm32f103/libopencm3/usb_cdc_acm_freertos/usb_cdc_acm_freertos.bin 0x08000000
sudo st-flash reset
ls -l /dev/ttyACM*
screen /dev/ttyACM0 115200
```

The active device may be `/dev/ttyACM0`, `/dev/ttyACM1`, or another ACM number.
Check `dmesg` after reconnecting the STM32 USB port:

```sh
sudo dmesg | tail -40
```

Expected enumeration looks like:

```text
Product: STM32F103 CDC ACM FreeRTOS
cdc_acm ... ttyACM1: USB ACM device
```

Use the device named by `dmesg`. In this example, that is `/dev/ttyACM1`.

To test the firmware echo path with two terminals, run this in terminal 1:

```sh
sudo cat /dev/ttyACM1
```

Then run this once in terminal 2:

```sh
printf 'hello usb\r\n' | sudo tee /dev/ttyACM1 > /dev/null
```

Terminal 1 should print:

```text
hello usb
```

This proves the full path works:

```text
PC -> USB CDC OUT -> cdc_data_rx -> usb_rx_stream -> echo_task
   -> usb_tx_stream -> usb_task -> USB CDC IN -> PC
```

You can also test with minicom:

```sh
sudo minicom -D /dev/ttyACM1 -b 115200
```

If text appears twice, disable minicom local echo with `Ctrl-A` then `E`. With
local echo off, visible typed text is coming back from the STM32 firmware.

Exit minicom with `Ctrl-A`, then `X`, then confirm.

If a `/dev/ttyACM0` entry starts with `-` in `ls -l`, it is a regular stale
file, not a USB serial character device. A real ACM device starts with `c`:

```text
crw-rw---- 1 root dialout ... /dev/ttyACM1
```

Remove stale regular files before testing:

```sh
sudo rm -f /dev/ttyACM0
```

## Bring-up notes

The issue encountered during initial testing had two parts:

- The image was not being flashed at first. `st-flash` could not open ST-LINK
  without `sudo`, one command used `0x0800000` instead of `0x08000000`, and one
  flash attempt used a relative path from the wrong directory.
- After the image was flashed, Linux saw a full-speed USB device but reported
  `error -71` while reading descriptors. The RTOS USB service task was sleeping
  for 1 ms between `usbd_poll()` calls, which was too slow during enumeration on
  the tested host.

The fix was to poll aggressively until configuration completes, then give the
worker tasks regular CPU time:

```c
if (usb_configured) {
    vTaskDelay(pdMS_TO_TICKS(1));
} else {
    taskYIELD();
}
```

The example also pulses `PA12` low and then releases it to input floating before
USB starts. Leaving `PA12` driven as GPIO is invalid because `PA12` is USB D+.
