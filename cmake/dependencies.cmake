include(FetchContent)

set(LIBOPENCM3_GIT_REPOSITORY
    "https://github.com/libopencm3/libopencm3.git"
    CACHE STRING "libopencm3 git repository")
set(LIBOPENCM3_GIT_TAG
    "master"
    CACHE STRING "libopencm3 git tag, branch, or commit")

function(add_libopencm3_dependency)
    if(TARGET opencm3_stm32f1)
        return()
    endif()

    find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

    FetchContent_Declare(
        libopencm3
        GIT_REPOSITORY "${LIBOPENCM3_GIT_REPOSITORY}"
        GIT_TAG "${LIBOPENCM3_GIT_TAG}"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")
    FetchContent_MakeAvailable(libopencm3)

    set(libopencm3_library
        "${libopencm3_SOURCE_DIR}/lib/libopencm3_stm32f1.a")

    add_custom_command(
        OUTPUT "${libopencm3_library}"
        COMMAND "${MAKE_EXECUTABLE}" TARGETS=stm32/f1 PREFIX="${TOOLCHAIN_PREFIX}"
        WORKING_DIRECTORY "${libopencm3_SOURCE_DIR}"
        COMMENT "Building libopencm3"
        VERBATIM)

    add_custom_target(libopencm3_build DEPENDS "${libopencm3_library}")

    add_library(opencm3_stm32f1 STATIC IMPORTED GLOBAL)
    add_dependencies(opencm3_stm32f1 libopencm3_build)
    set_target_properties(
        opencm3_stm32f1 PROPERTIES
        IMPORTED_LOCATION "${libopencm3_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${libopencm3_SOURCE_DIR}/include"
        INTERFACE_LINK_DIRECTORIES "${libopencm3_SOURCE_DIR}/lib")
endfunction()

set(FREERTOS_KERNEL_GIT_REPOSITORY
    "https://github.com/FreeRTOS/FreeRTOS-Kernel.git"
    CACHE STRING "FreeRTOS kernel git repository")
set(FREERTOS_KERNEL_GIT_TAG
    "main"
    CACHE STRING "FreeRTOS kernel git tag, branch, or commit")

function(add_freertos_kernel_stm32f103 config_include_dir)
    if(TARGET freertos_kernel_stm32f103)
        return()
    endif()

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
        ${config_include_dir}
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
endfunction()
