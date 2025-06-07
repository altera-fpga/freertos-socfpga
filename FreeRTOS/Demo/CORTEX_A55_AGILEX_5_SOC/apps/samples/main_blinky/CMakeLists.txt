
target_include_directories(RTOSBlinkyApp.elf
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

set(MAIN_SOURCE_SRCS_BLINKY
    ${MAIN_SOURCE_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/main_blinky.c
    PARENT_SCOPE
)
