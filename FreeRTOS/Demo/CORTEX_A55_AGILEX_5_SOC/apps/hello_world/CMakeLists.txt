cmake_minimum_required(VERSION 3.5...3.28)

set(EXECUTABLE_NAME "hello_world.elf")

#define these for qspi image generation
set(PFG_FILENAME "qspi_flash_image_agilex5_boot.pfg")
set(QUARTUS_PATH "<path to quartus>")

include(FetchContent)

set(FREERTOS_FATFS n)
set(FREERTOS_USB n)
set(FREERTOS_TCPIP n)
set(FREERTOS_LIBRSU n)
set(FREERTOS_LIBFCS n)

FetchContent_Declare(
    freertos
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../../../../..
)

# Get the freertos repo
FetchContent_MakeAvailable(freertos)
FetchContent_GetProperties(freertos)

# project
project(freertos_hello_world C CXX ASM)

include(${CMAKE_CURRENT_SOURCE_DIR}/../target_socfpga.cmake)

# target
add_executable(${EXECUTABLE_NAME})

#Add image generation
generate_bin_file(${EXECUTABLE_NAME})
add_sd_image(${EXECUTABLE_NAME})
add_qspi_image(${EXECUTABLE_NAME} ${PFG_FILENAME} ${QUARTUS_PATH})

# link to the baremetal library
target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE freertos_socfpga
)

# sources
target_sources(${EXECUTABLE_NAME}
    PRIVATE hello_world.c
)

# specify linker script
target_link_options(${EXECUTABLE_NAME} PRIVATE
    -T${LINKER_SCRIPT}
)
