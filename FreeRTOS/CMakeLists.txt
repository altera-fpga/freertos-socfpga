cmake_minimum_required(VERSION 3.15.0)

#Use the heap provided by the User
#By default heap_3.c is used 
set(HEAP 3)
SET(HEAP "3" CACHE STRING "Some user-specified option")
#message("Heap entered  is : " ${HEAP})
if( ${HEAP} GREATER "5" )
    message(FATAL_ERROR "Entered value for heap file should be less than 6")
endif()

set(FREERTOS_HEAP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/MemMang/heap_${HEAP}.c)
message("${BoldWhite}Heap used       :  ${FREERTOS_HEAP_SRC}${ColourReset}")

#FreeRTOS kernel Source files
set(FREERTOS_KERNEL_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/croutine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/stream_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/timers.c
    )

target_include_directories(freertos_socfpga 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source/include/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/portable/GCC/ARM_CA55_64_BIT/
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/Utilities/logging/
)

if (FREERTOS_CLI )
    file(GLOB CLI_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI/*.c)
    target_include_directories(freertos_socfpga
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI
    )
endif()

if (FREERTOS_FATFS )
    file(GLOB FATFS_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/Fat-Fs/*.c)
    target_include_directories(freertos_socfpga
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Fat-Fs/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Fat-Fs/portable/common
    )
endif()

if (FREERTOS_TCPIP )
    file(GLOB TCP_IP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/*.c)

    list(APPEND TCP_IP_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/portable/BufferManagement/BufferAllocation_1.c
        ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/portable/NetworkInterface/AGILEX5/NetworkInterface.c
    )


    target_include_directories(freertos_socfpga
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/portable/Compiler/GCC
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/portable/NetworkInterface/AGILEX5/
    )

    #TODO: Supress compiler warning in TCP stack (Cannot edit stack files)
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/FreeRTOS_ICMP.c
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../ PROPERTY COMPILE_OPTIONS "-Wno-unused-parameter")
    set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/FreeRTOS_DNS.c
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../ PROPERTY COMPILE_OPTIONS "-w")

endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Demo/CORTEX_A55_AGILEX_5_SOC)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/portable)

set(MAIN_SOURCE_SRCS
    ${FREERTOS_KERNEL_SRCS}
    ${FREERTOS_HEAP_SRC}
    ${MAIN_SOURCE_SRCS}
    ${FATFS_SRCS}
    ${TINYUSB_SRCS}
    ${TCP_IP_SRCS}
    ${CLI_SRCS}
    PARENT_SCOPE
    )
