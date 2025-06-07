cmake_minimum_required(VERSION 3.5...3.28)

#define these for qspi image generation
set(PFG_FILENAME "qspi_flash_image_agilex5_boot.pfg")
set(QUARTUS_PATH "path to quartus")

include(FetchContent)

set(FREERTOS_FATFS y)
set(FREERTOS_USB y)
set(FREERTOS_TCPIP y)
set(FREERTOS_LIBRSU y)
set(FREERTOS_LIBFCS y)

FetchContent_Declare(
    freertos
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/../..
)

# Get the freertos repo
FetchContent_MakeAvailable(freertos)
FetchContent_GetProperties(freertos)

# project
project(freertos_driver_samples C CXX ASM)

include(${CMAKE_CURRENT_SOURCE_DIR}/target_socfpga.cmake)


set(EXECUTABLE_NAME "drivertests.elf")

# target
add_executable(${EXECUTABLE_NAME})

generate_bin_file(${EXECUTABLE_NAME})
add_sd_image(${EXECUTABLE_NAME})
add_qspi_image(${EXECUTABLE_NAME} ${PFG_FILENAME} ${QUARTUS_PATH})

#Add image generation
# link to the baremetal library
target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE freertos_socfpga
)
target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE FCS)

get_property(rsu_include_dirs TARGET socfpga_rsu PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(${EXECUTABLE_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${rsu_include_dirs}
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/unity)
# sources
target_sources(${EXECUTABLE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dma/dma_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio/gpio_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fatfs/fat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/i2c/i2c_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/i3c/i3c_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/qspi/qspi_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/timer_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/seu/seu_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fcs/fcs_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdmmc/sdmmc_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/spi/spi_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usb3/usb3_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/wdt/wdt_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/reboot_mngr/reboot_mngr_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdm_mailbox/mbox_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/uart/uart_test_print.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fpga_manager/fpga_mngr_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fpga_manager/socfpga_mmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bridge/bridge_driver_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fcs/fcs_test.c
    $<TARGET_OBJECTS:socfpga_rsu>
    ${CMAKE_CURRENT_SOURCE_DIR}/rsu/rsu_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/iossm/iossm_test.c
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/timer
    ${CMAKE_CURRENT_SOURCE_DIR}/dma
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio
    ${CMAKE_CURRENT_SOURCE_DIR}/fatfs
    ${CMAKE_CURRENT_SOURCE_DIR}/i2c
    ${CMAKE_CURRENT_SOURCE_DIR}/i3c
    ${CMAKE_CURRENT_SOURCE_DIR}/qspi
    ${CMAKE_CURRENT_SOURCE_DIR}/seu
    ${CMAKE_CURRENT_SOURCE_DIR}/sdmmc
    ${CMAKE_CURRENT_SOURCE_DIR}/spi
    ${CMAKE_CURRENT_SOURCE_DIR}/usb3
    ${CMAKE_CURRENT_SOURCE_DIR}/wdt
    ${CMAKE_CURRENT_SOURCE_DIR}/uart
    ${CMAKE_CURRENT_SOURCE_DIR}/reboot_mngr
    ${CMAKE_CURRENT_SOURCE_DIR}/fpga_manager
    ${CMAKE_CURRENT_SOURCE_DIR}/bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libFCS/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libFCS/lib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libFCS/toolchain/freertos_aarch64/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../librsu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/iossm
    )

# specify linker script
target_link_options(${EXECUTABLE_NAME} PRIVATE
    -T${LINKER_SCRIPT}
)

