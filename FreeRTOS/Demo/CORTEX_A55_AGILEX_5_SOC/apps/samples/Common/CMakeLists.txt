file(GLOB COMMON_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/Minimal/*.c)

target_include_directories(RTOSFullApp.elf
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# As per this link, we have to exclude the following files.
# https://github.com/FreeRTOS/FreeRTOS/blob/main/FreeRTOS/Demo/ThirdParty/Template/README.md#project-configuration
list(REMOVE_ITEM COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Minimal/comtest_strings.c")
list(REMOVE_ITEM COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Minimal/crhook.c")
list(REMOVE_ITEM COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Minimal/comtest.c")
list(REMOVE_ITEM COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Minimal/crflash.c")
list(REMOVE_ITEM COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Minimal/flash.c")
list(REMOVE_ITEM COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Minimal/flash_timer.c")
list(REMOVE_ITEM COMMON_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Minimal/sp_flop.c")

set(MAIN_SOURCE_SRCS_FULL 
    ${MAIN_SOURCE_SRCS_FULL}
    ${COMMON_SRCS} 
    PARENT_SCOPE
    )
