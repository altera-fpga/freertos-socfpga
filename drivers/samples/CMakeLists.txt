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

add_subdirectory(wdt)
add_subdirectory(usb3)
add_subdirectory(uart)
add_subdirectory(timer)
add_subdirectory(spi)
add_subdirectory(seu)
add_subdirectory(sdmmc)
add_subdirectory(sdm_mailbox)
add_subdirectory(reboot_mngr)
add_subdirectory(qspi)
add_subdirectory(iossm)
add_subdirectory(i3c)
add_subdirectory(i2c)
add_subdirectory(gpio)
add_subdirectory(fatfs)
add_subdirectory(dma)
add_subdirectory(fcs)
add_subdirectory(bridge)
add_subdirectory(fpga_manager)
add_subdirectory(rsu)
