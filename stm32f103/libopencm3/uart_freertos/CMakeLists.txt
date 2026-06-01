cmake_minimum_required(VERSION 3.20)

include(${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake)

add_libopencm3_dependency()

include(FetchContent)

set(FREERTOS_KERNEL_GIT_REPOSITORY
    "https://github.com/FreeRTOS/FreeRTOS-Kernel.git"
    CACHE STRING "FreeRTOS kernel git repository")
set(FREERTOS_KERNEL_GIT_TAG
    "main"
    CACHE STRING "FreeRTOS kernel git tag, branch, or commit")

FetchContent_Declare(
    freertos_kernel
    GIT_REPOSITORY "${FREERTOS_KERNEL_GIT_REPOSITORY}"
    GIT_TAG "${FREERTOS_KERNEL_GIT_TAG}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "")
FetchContent_GetProperties(freertos_kernel)
if(NOT freertos_kernel_POPULATED)
    FetchContent_Populate(freertos_kernel)
endif()

add_library(freertos_kernel_stm32f103 STATIC
    ${freertos_kernel_SOURCE_DIR}/croutine.c
    ${freertos_kernel_SOURCE_DIR}/event_groups.c
    ${freertos_kernel_SOURCE_DIR}/list.c
    ${freertos_kernel_SOURCE_DIR}/queue.c
    ${freertos_kernel_SOURCE_DIR}/stream_buffer.c
    ${freertos_kernel_SOURCE_DIR}/tasks.c
    ${freertos_kernel_SOURCE_DIR}/timers.c
    ${freertos_kernel_SOURCE_DIR}/portable/GCC/ARM_CM3/port.c
    ${freertos_kernel_SOURCE_DIR}/portable/MemMang/heap_4.c)

target_include_directories(freertos_kernel_stm32f103 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${freertos_kernel_SOURCE_DIR}/include
    ${freertos_kernel_SOURCE_DIR}/portable/GCC/ARM_CM3)

target_compile_options(freertos_kernel_stm32f103 PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra)

target_compile_definitions(freertos_kernel_stm32f103 PUBLIC
    STM32F1)

add_executable(uart_freertos
    src/main.c)

set_target_properties(uart_freertos PROPERTIES
    SUFFIX .elf)

target_compile_options(uart_freertos PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Werror)

target_compile_definitions(uart_freertos PRIVATE
    STM32F1)

target_link_options(uart_freertos PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -nostartfiles
    -Wl,--gc-sections
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f103c8t6.ld)

target_link_libraries(uart_freertos PRIVATE
    freertos_kernel_stm32f103
    opencm3_stm32f1)

add_custom_command(
    TARGET uart_freertos
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:uart_freertos>
            ${CMAKE_CURRENT_BINARY_DIR}/uart_freertos.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:uart_freertos>
            ${CMAKE_CURRENT_BINARY_DIR}/uart_freertos.hex
    COMMENT "Generating uart_freertos.bin and uart_freertos.hex")
