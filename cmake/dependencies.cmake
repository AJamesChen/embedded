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
