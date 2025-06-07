set(EXECUTABLE_NAME "timer.elf")
set(DEBUGGER_SCRIPT "debugger_script.ds")

set(PFG_FILENAME "qspi_flash_image_agilex5_boot.pfg")
set(QUARTUS_PATH "<path to quartus>")

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

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

target_include_directories(${EXECUTABLE_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# sources
target_sources(${EXECUTABLE_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/samples_main.c ${SRC_FILES}
)

# specify linker script
target_link_options(${EXECUTABLE_NAME} PRIVATE
    -T${LINKER_SCRIPT}
)

# Add a custom command to replace <APPNAME> with the EXECUTABLE_NAME
add_custom_command(
    TARGET ${EXECUTABLE_NAME}
    COMMAND ${CMAKE_COMMAND} -E echo "Creating debug script for ${EXECUTABLE_NAME}"
    COMMAND sed 's|<APPNAME>|timer/${EXECUTABLE_NAME}|g' ${CMAKE_SOURCE_DIR}/${DEBUGGER_SCRIPT} > ${CMAKE_BINARY_DIR}/${DEBUGGER_SCRIPT}
    DEPENDS ${DEBUGGER_SCRIPT_INPUT}
    COMMENT "Generating ${DEBUGGER_SCRIPT} with executable name"
)

