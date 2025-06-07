file(GLOB CLI_APP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

file(GLOB TCP_IP_SRCS ${CMAKE_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/*.c)

get_property(rsu_include_dirs TARGET socfpga_rsu PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(RTOSCliApp.elf
    PUBLIC
	${rsu_include_dirs}
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(RTOSCliApp.elf
    PUBLIC
    FCS)

target_sources(RTOSCliApp.elf
	PUBLIC
    $<TARGET_OBJECTS:socfpga_rsu>
)

set(MAIN_SOURCE_SRCS_CLI 
    ${CLI_APP_SRCS} 
    ${TCP_IP_SRCS}
    PARENT_SCOPE
    )
