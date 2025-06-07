target_sources(tinyusb_lib 
    PRIVATE 
    src/usb_main.c
    src/class/msc/msc_app.c
    src/class/msc/msc_host.c
    src/tusb.c
    src/host/usbh.c
    src/portable/socfpga/hcd_socfpga.c
    src/common/tusb_fifo.c
    src/host/hub.c
    src/portable/socfpga/socfpga_common.c
    )
include_directories(src)

target_include_directories(tinyusb_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${esw_bare_SOURCE_DIR}/tinyusb/src/class/msc/
    ${esw_bare_SOURCE_DIR}/tinyusb/src/common/
    ${esw_bare_SOURCE_DIR}/drivers/reset_mngr/
    ${esw_bare_SOURCE_DIR}/drivers/usb3/
    ${esw_bare_SOURCE_DIR}/drivers/common/
    ${esw_bare_SOURCE_DIR}/drivers/usb3/dwc3/
    ${esw_bare_SOURCE_DIR}/drivers/usb3/host/
    ${esw_bare_SOURCE_DIR}/osal/freertos/
    ${esw_bare_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/Utilities/logging/
    )

target_link_libraries(tinyusb_lib -lm)
target_link_libraries(tinyusb_lib PUBLIC socfpga_drivers)
