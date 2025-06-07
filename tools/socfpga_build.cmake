add_custom_target(atf-build)
add_custom_target(atf-build-qspi)
add_custom_target(atf-build-sd)

set(CMAKE_OBJCOPY aarch64-none-elf-objcopy PARENT_SCOPE)
set(CMAKE_OBJDUMP aarch64-none-elf-objdump PARENT_SCOPE)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
set(ASM_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.asm)

add_dependencies(atf-build-qspi atf-bl2-bl31-qspi)
add_dependencies(atf-build-sd atf-bl2-bl31-sd)

if(NOT ATF_GIT_REPO)
    set(ATF_GIT_REPO https://github.com/altera-opensource/arm-trusted-firmware)
endif()

if(NOT ATF_GIT_TAG)
    set(ATF_GIT_TAG QPDS24.3_REL_GSRD_PR)
endif()

FetchContent_Declare(
    atf
    GIT_REPOSITORY ${ATF_GIT_REPO}
    GIT_TAG ${ATF_GIT_TAG}
    )

# Fetch the ATF code
FetchContent_MakeAvailable(atf)
set(atf_SOURCE_DIR "${atf_SOURCE_DIR}" CACHE INTERNAL "Path to ATF source")
set(TOOL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools" CACHE INTERNAL "Path to tools folder")

# Add custom target to build ATF
add_custom_target(
    atf-bl2-bl31-qspi
    COMMAND make -C ${atf_SOURCE_DIR} PLAT=agilex5 clean
    COMMAND make -C ${atf_SOURCE_DIR} fiptool
    COMMAND make -C ${atf_SOURCE_DIR} CROSS_COMPILE=aarch64-none-elf- PLAT=agilex5 SOCFPGA_BOOT_SOURCE_QSPI=1 bl2 bl31 PRELOADED_BL33_BASE=0x80100000
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    BYPRODUCTS ${atf_SOURCE_DIR}/build/agilex5/release/bl31.bin
    BYPRODUCTS ${atf_SOURCE_DIR}/build/agilex5/release/bl2.bin
)

add_custom_target(
    atf-bl2-bl31-sd
    COMMAND make -C ${atf_SOURCE_DIR} PLAT=agilex5 clean
    COMMAND make -C ${atf_SOURCE_DIR} fiptool
    COMMAND make -C ${atf_SOURCE_DIR} CROSS_COMPILE=aarch64-none-elf- PLAT=agilex5 bl2 bl31 PRELOADED_BL33_BASE=0x80100000
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    BYPRODUCTS ${atf_SOURCE_DIR}/build/agilex5/release/bl31.bin
    BYPRODUCTS ${atf_SOURCE_DIR}/build/agilex5/release/bl2.bin
)

# Add custom target to build fip tool
add_custom_target(
    atf-fiptool
    COMMAND make -C ${atf_SOURCE_DIR} fiptool
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    BYPRODUCTS ${atf_SOURCE_DIR}/tools/fiptool/fiptool
)

function(generate_bin_file target_name)
    if(target_name MATCHES "\\.elf$")
        string(REGEX REPLACE "\\.elf$" ".bin" BIN_FILE "${target_name}")
    else()
        set(BIN_FILE "${target_name}.bin")
    endif()
    string(REGEX REPLACE ".bin" ".asm" ASM_FILE ${BIN_FILE})
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${target_name} ${BIN_FILE}
        COMMAND ${CMAKE_OBJDUMP} -D ${target_name} >  ${ASM_FILE}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Generating ${BIN_FILE} and ${ASM_FILE} from ${target_name}"
    )
    add_custom_target(${BIN_FILE} DEPENDS ${target_name})
endfunction()

function(add_sd_image executable_name)
    if(executable_name MATCHES "\\.elf$")
        string(REGEX REPLACE "\\.elf$" ".bin" BIN_FILE "${executable_name}")
    else()
        set(BIN_FILE "${executable_name}.bin")
    endif()
    string(REGEX REPLACE ".bin" "" TARG_NAME ${BIN_FILE})
    set(FIP_FILE "fip.bin")
    set(FIPTOOL_BIN ${atf_SOURCE_DIR}/tools/fiptool/fiptool)
    set(BL31_BIN ${atf_SOURCE_DIR}/build/agilex5/release/bl31.bin)
    add_custom_target(
        sd-image-${TARG_NAME}
        COMMAND ${FIPTOOL_BIN} create --soc-fw ${BL31_BIN} --nt-fw ${CMAKE_CURRENT_BINARY_DIR}/${BIN_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${FIP_FILE}
        COMMAND rm -f sd.img
        COMMAND ${TOOL_DIR}/make_sdimage.sh -f ${FIP_FILE} -o sd.img -s 2048
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        BYPRODUCTS sd.img
    )
    add_dependencies(sd-image-${TARG_NAME} atf-build-sd)
endfunction()

function(add_qspi_image executable_name pfg_file quartus_path)
    if(executable_name MATCHES "\\.elf$")
        string(REGEX REPLACE "\\.elf$" ".bin" BIN_FILE "${executable_name}")
    else()
        set(BIN_FILE "${executable_name}.bin")
    endif()
    string(REGEX REPLACE ".bin" "" TARG_NAME ${BIN_FILE})
    set(FIP_FILE "fip.bin")
    set(FIPTOOL_BIN ${atf_SOURCE_DIR}/tools/fiptool/fiptool)
    set(BL31_BIN ${atf_SOURCE_DIR}/build/agilex5/release/bl31.bin)
    set(BL2_BIN ${atf_SOURCE_DIR}/build/agilex5/release/bl2.bin)
    add_custom_target(
        qspi-image-${TARG_NAME}
        COMMAND ${FIPTOOL_BIN} create --soc-fw ${BL31_BIN} --nt-fw ${CMAKE_CURRENT_BINARY_DIR}/${BIN_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${FIP_FILE}
        COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex --change-addresses 0x0 ${BL2_BIN} bl2.hex
        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/ghrd_a5ed065bb32ae6sr0.sof ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${quartus_path}/quartus/bin/quartus_pfg -c ${CMAKE_CURRENT_SOURCE_DIR}/${pfg_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        BYPRODUCTS sd.img
    )
    add_dependencies(qspi-image-${TARG_NAME} atf-build-qspi)
endfunction()

