# Ticket: Harden STM32F103 FreeRTOS USB CDC example

Status: closed

## Summary

Improve and validate the `usb_cdc_acm_freertos` example so it is reliable as a
reference for STM32F103/libopencm3 USB CDC with FreeRTOS.

## Background

Initial bring-up exposed several practical failure modes:

- `st-flash` could not access ST-LINK without elevated permissions.
- One flash command used the invalid address `0x0800000` instead of
  `0x08000000`.
- One flash command used a relative `.bin` path from the wrong working
  directory.
- USB enumeration initially failed with Linux `error -71` while reading
  descriptors.
- Continuous USB polling fixed enumeration, but then the USB task could starve
  the lower-priority echo task after configuration.
- A stale regular file at `/dev/ttyACM0` can be created accidentally by writing
  to the path when no CDC ACM device exists.

The current firmware enumerates as:

```text
Product: STM32F103 CDC ACM FreeRTOS
Manufacturer: Example
SerialNumber: DEMO002
cdc_acm ... ttyACM1: USB ACM device
```

## Current Implementation

The RTOS example uses:

- `cdc_data_rx()` to receive USB OUT bytes into `usb_rx_stream`.
- `echo_task()` to move bytes from `usb_rx_stream` to `usb_tx_stream`.
- `usb_task()` to own `usbd_poll()` and `usbd_ep_write_packet()`.
- Aggressive `taskYIELD()` polling before USB configuration.
- A 1 ms task delay after configuration so worker tasks can run.
- A pending TX packet retry path when the USB IN endpoint is busy.
- A `PA12` disconnect pulse that releases D+ back to input floating before USB
  starts.

## Acceptance Criteria

- Firmware builds with:

```sh
cmake --build build-arm --target usb_cdc_acm_freertos
```

- Firmware flashes and verifies with:

```sh
sudo st-flash write build-arm/stm32f103/libopencm3/usb_cdc_acm_freertos/usb_cdc_acm_freertos.bin 0x08000000
sudo st-flash reset
```

- Linux enumerates the STM32 native USB port as CDC ACM without `error -71`:

```text
Product: STM32F103 CDC ACM FreeRTOS
cdc_acm ... ttyACM*: USB ACM device
```

- Two-terminal echo test passes:

```sh
sudo cat /dev/ttyACM1
```

```sh
printf 'hello usb\r\n' | sudo tee /dev/ttyACM1 > /dev/null
```

Expected output in the `cat` terminal:

```text
hello usb
```

- Minicom test passes with local echo disabled:

```sh
sudo minicom -D /dev/ttyACM1 -b 115200
```

Typed characters are displayed because they are echoed by the STM32 firmware.

## Completed Work

- Replaced per-byte queues with stream buffers to reduce queue overhead.
- Added configured/reset state handling for USB task scheduling.
- Added a pending TX packet retry path when the USB IN endpoint is busy.

## Remaining Follow-up Work

- Add an optional command-shell example on top of the CDC transport.
- Add documentation for installing ST-LINK udev rules so flashing does not
  require `sudo`.
- Consider a test build variant that emits debug status over `USART1`.
