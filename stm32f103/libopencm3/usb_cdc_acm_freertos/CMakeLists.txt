cmake_minimum_required(VERSION 3.20)

include(${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake)

add_libopencm3_dependency()
add_freertos_kernel_stm32f103(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(usb_cdc_acm_freertos
    src/main.c)

set_target_properties(usb_cdc_acm_freertos PROPERTIES
    SUFFIX .elf)

target_compile_options(usb_cdc_acm_freertos PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Werror)

target_compile_definitions(usb_cdc_acm_freertos PRIVATE
    STM32F1)

target_link_options(usb_cdc_acm_freertos PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -nostartfiles
    -Wl,--gc-sections
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f103c8t6.ld)

target_link_libraries(usb_cdc_acm_freertos PRIVATE
    freertos_kernel_stm32f103
    opencm3_stm32f1)

add_custom_command(
    TARGET usb_cdc_acm_freertos
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:usb_cdc_acm_freertos>
            ${CMAKE_CURRENT_BINARY_DIR}/usb_cdc_acm_freertos.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:usb_cdc_acm_freertos>
            ${CMAKE_CURRENT_BINARY_DIR}/usb_cdc_acm_freertos.hex
    COMMENT "Generating usb_cdc_acm_freertos.bin and usb_cdc_acm_freertos.hex")
