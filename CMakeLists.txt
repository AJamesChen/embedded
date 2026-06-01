cmake_minimum_required(VERSION 3.20)

project(embedded C ASM)

add_subdirectory(stm32f103/libopencm3/gpio_blink)
add_subdirectory(stm32f103/libopencm3/uart_freertos)
