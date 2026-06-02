cmake_minimum_required(VERSION 3.20)

include(${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake)

add_libopencm3_dependency()
add_freertos_kernel_stm32f103(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(rtos_blinky
    src/main.c)

set_target_properties(rtos_blinky PROPERTIES
    SUFFIX .elf)

target_compile_options(rtos_blinky PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Werror)

target_compile_definitions(rtos_blinky PRIVATE
    STM32F1)

target_link_options(rtos_blinky PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -nostartfiles
    -Wl,--gc-sections
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f103c8t6.ld)

target_link_libraries(rtos_blinky PRIVATE
    freertos_kernel_stm32f103
    opencm3_stm32f1)

add_custom_command(
    TARGET rtos_blinky
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:rtos_blinky>
            ${CMAKE_CURRENT_BINARY_DIR}/rtos_blinky.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:rtos_blinky>
            ${CMAKE_CURRENT_BINARY_DIR}/rtos_blinky.hex
    COMMENT "Generating rtos_blinky.bin and rtos_blinky.hex")
