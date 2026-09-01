cmake_minimum_required(VERSION 3.5...3.28)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../samples/common/download_dep.cmake)

include(FetchContent)

set(FREERTOS_FATFS y)
set(FREERTOS_USB y)
set(FREERTOS_TCPIP y)
set(FREERTOS_LIBRSU n)
set(FREERTOS_LIBFCS n)

FetchContent_Declare(
    freertos
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../../../../../../
)

# Get the freertos repo
FetchContent_MakeAvailable(freertos)
FetchContent_GetProperties(freertos)

# project
project(heterogeneous_multi_block_test C CXX ASM)

set(LINKER_SCRIPT "${freertos_SOURCE_DIR}/lscript.ld")
include(${freertos_SOURCE_DIR}/tools/target_socfpga.cmake)

# target
add_executable(${PROJECT_NAME}.elf)

# Add image generation
generate_bin_file(${PROJECT_NAME}.elf)
add_sd_image(${PROJECT_NAME}.elf)
add_qspi_image(${PROJECT_NAME}.elf)

# link to the baremetal library
target_link_libraries(${PROJECT_NAME}.elf
    PRIVATE freertos_socfpga
)

target_sources(${PROJECT_NAME}.elf
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heterogeneous_multi_block_mbox.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heterogeneous_multi_block_ethernet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heterogeneous_multi_block_qspi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heterogeneous_multi_block_rtos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heterogeneous_multi_block_sd.c
)

# specify linker script
target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${LINKER_SCRIPT}
)
