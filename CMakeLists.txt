cmake_minimum_required(VERSION 3.15.0)

set(ARCH freertos_aarch64)
include(FetchContent)

include(${CMAKE_CURRENT_SOURCE_DIR}/tools/target_socfpga.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/tools/socfpga_build.cmake)

project(RTOSApp C CXX ASM)
add_library(freertos_socfpga STATIC)

target_include_directories(freertos_socfpga
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/osal/freertos/
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/portable/GCC/ARM_CA55_64_BIT/
    ${FREERTOS_CONFIG_HEADER_PATH}
)

set(RTOS_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/osal/freertos/
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Source/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/portable/GCC/ARM_CA55_64_BIT/
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/Utilities/logging/
    ${FREERTOS_CONFIG_HEADER_PATH}
)

if (FREERTOS_USB )
    file(GLOB_RECURSE TINYUSB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/*.c)
    target_include_directories(freertos_socfpga
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/class/msc
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/common
    )
    set(RTOS_INCLUDES
        ${RTOS_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/class/msc
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/common
    )
endif()

if (FREERTOS_TCPIP )
    set(RTOS_INCLUDES
        ${RTOS_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/include
        ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/portable/Compiler/GCC
        ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/portable/NetworkInterface/include
    )
endif()

set(DRIVER_RTOS_INCLUDES ${RTOS_INCLUDES})

add_subdirectory(drivers)
add_subdirectory(FreeRTOS)

get_property(driver_include_dirs TARGET socfpga_drivers PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(freertos_socfpga PUBLIC ${driver_include_dirs})

if(FREERTOS_LIBRSU)
	#Need the libFCS to be built as static lib
	set(NEED_STATIC_LIB ON)
	add_subdirectory(libFCS)

	#pass required includes to FCS library
	target_include_directories(FCS PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Fat-Fs/portable/common/
		${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Fat-Fs/include/
		${CMAKE_CURRENT_SOURCE_DIR}/osal/freertos/
		${FREERTOS_CONFIG_HEADER_PATH}
		${driver_include_dirs}
		${RTOS_INCLUDES}
	)
endif()

if(FREERTOS_LIBRSU)
	#lib RSU library setup
	set(RSU_DEP_INCLUDES
	${RTOS_INCLUDES}
	${driver_include_dirs}
	${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Fat-Fs/include/
	${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Fat-Fs/portable/common/
)
	add_subdirectory(librsu)
endif()
target_link_libraries(freertos_socfpga m)

target_sources(freertos_socfpga PRIVATE 
    ${MAIN_SOURCE_SRCS}
    $<TARGET_OBJECTS:socfpga_drivers>
    )

