cmake_minimum_required(VERSION 3.20)

include(${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake)

add_libopencm3_dependency()

add_executable(gpio_blink
    src/main.c)

set_target_properties(gpio_blink PROPERTIES
    SUFFIX .elf)

target_compile_options(gpio_blink PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Werror)

target_compile_definitions(gpio_blink PRIVATE
    STM32F1)

target_link_options(gpio_blink PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -nostartfiles
    -Wl,--gc-sections
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f103c8t6.ld)

target_link_libraries(gpio_blink PRIVATE opencm3_stm32f1)

add_custom_command(
    TARGET gpio_blink
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:gpio_blink>
            ${CMAKE_CURRENT_BINARY_DIR}/gpio_blink.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:gpio_blink>
            ${CMAKE_CURRENT_BINARY_DIR}/gpio_blink.hex
    COMMENT "Generating gpio_blink.bin and gpio_blink.hex")
