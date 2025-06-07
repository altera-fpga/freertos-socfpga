cmake_minimum_required(VERSION 3.5...3.28)

#define these for qspi image generation
set(PFG_FILENAME "qspi_flash_image_agilex5_boot.pfg")
set(QUARTUS_PATH "<path to quartus>")

include(FetchContent)

set(FREERTOS_CLI y)
set(FREERTOS_FATFS y)
set(FREERTOS_USB y)
set(FREERTOS_TCPIP y)
set(FREERTOS_LIBRSU y)
set(FREERTOS_LIBFCS y)

FetchContent_Declare(
    freertos
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../../../../..
)

# Get the freertos repo
FetchContent_MakeAvailable(freertos)
FetchContent_GetProperties(freertos)

# project
project(freertos_sample C CXX ASM)

include(${CMAKE_CURRENT_SOURCE_DIR}/../target_socfpga.cmake)

# target
add_executable(RTOSFullApp.elf)
add_executable(RTOSCliApp.elf)
add_executable(RTOSEnetApp.elf)
add_executable(RTOSBlinkyApp.elf)
generate_bin_file(RTOSFullApp.elf)
generate_bin_file(RTOSCliApp.elf)
generate_bin_file(RTOSEnetApp.elf)
generate_bin_file(RTOSBlinkyApp.elf)

add_sd_image(RTOSFullApp.elf)
add_qspi_image(RTOSFullApp.elf ${PFG_FILENAME} ${QUARTUS_PATH})
add_sd_image(RTOSCliApp.elf)
add_qspi_image(RTOSCliApp.elf ${PFG_FILENAME} ${QUARTUS_PATH})
add_sd_image(RTOSEnetApp.elf)
add_qspi_image(RTOSEnetApp.elf ${PFG_FILENAME} ${QUARTUS_PATH})
add_sd_image(RTOSBlinkyApp.elf)
add_qspi_image(RTOSBlinkyApp.elf ${PFG_FILENAME} ${QUARTUS_PATH})

add_subdirectory(cli_app)
add_subdirectory(main_freertosplus_basic)
add_subdirectory(main_full)
add_subdirectory(Common)
add_subdirectory(main_blinky)

# link to the baremetal library
target_link_libraries(RTOSFullApp.elf
    PRIVATE freertos_socfpga
)
target_link_libraries(RTOSCliApp.elf
    PRIVATE freertos_socfpga
)
target_link_libraries(RTOSEnetApp.elf
    PRIVATE freertos_socfpga
)
target_link_libraries(RTOSBlinkyApp.elf
    PRIVATE freertos_socfpga
)

target_include_directories(RTOSCliApp.elf PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libFCS/include
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libFCS/lib/include
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libFCS/toolchain/freertos_aarch64/include
)

# sources
target_sources(RTOSFullApp.elf
    PRIVATE main.c
    ${MAIN_SOURCE_SRCS_FULL}
)
target_sources(RTOSCliApp.elf
    PRIVATE main.c
    ${MAIN_SOURCE_SRCS_CLI}
)
target_sources(RTOSEnetApp.elf
    PRIVATE main.c
    ${MAIN_SOURCE_SRCS_PLUS}
)
target_sources(RTOSBlinkyApp.elf
    PRIVATE main.c
    ${MAIN_SOURCE_SRCS_BLINKY}
)

target_compile_options(RTOSFullApp.elf PRIVATE -DconfigACTIVE_DEMO=4)
target_compile_options(RTOSCliApp.elf PRIVATE -DconfigACTIVE_DEMO=0)
target_compile_options(RTOSEnetApp.elf PRIVATE -DconfigACTIVE_DEMO=5)
target_compile_options(RTOSBlinkyApp.elf PRIVATE -DconfigACTIVE_DEMO=3)

# specify linker script
target_link_options(RTOSFullApp.elf PRIVATE
    -T${LINKER_SCRIPT}
)
target_link_options(RTOSCliApp.elf PRIVATE
    -T${LINKER_SCRIPT}
)
target_link_options(RTOSEnetApp.elf PRIVATE
    -T${LINKER_SCRIPT}
)
target_link_options(RTOSBlinkyApp.elf PRIVATE
    -T${LINKER_SCRIPT}
)
