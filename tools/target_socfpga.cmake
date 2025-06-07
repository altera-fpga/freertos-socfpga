
set(CC_PREFIX aarch64-none-elf-)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR "arm64")
set(CMAKE_CROSSCOMPILING True)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_AR ${CC_PREFIX}ar CACHE STRING "AR" FORCE)
set(CMAKE_C_COMPILER  ${CC_PREFIX}gcc CACHE STRING "CC" FORCE)
set(CMAKE_ASM_COMPILER ${CC_PREFIX}gcc CACHE STRING "AS" FORCE)
set(CMAKE_CXX_COMPILER ${CC_PREFIX}g++ CACHE STRING "CXX" FORCE)

set(ToolchainPrefix ${CC_PREFIX} CACHE STRING "Toolchain prefix." FORCE)
set(ToolchainObjdump ${CC_PREFIX}objdump CACHE STRING "Objdump executable." FORCE)
set(ToolchainObjdumpFlags -SCdtx CACHE STRING "Objdump flags." FORCE)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(OBJECT_GEN_FLAGS "-ffunction-sections -fdata-sections")

set(CMAKE_OBJCOPY ${CC_PREFIX}objcopy)
set(CMAKE_OBJDUMP ${CC_PREFIX}objdump)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld PARENT_SCOPE)
set(LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/lib)

set(FREERTOS_CONFIG_HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Demo/CORTEX_A55_AGILEX_5_SOC)

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

message("${BoldWhite}Host System     :  ${CMAKE_SYSTEM_NAME}${ColourReset}")
message("${BoldWhite}C COMPILER      :  ${CMAKE_C_COMPILER}${ColourReset}")

add_compile_options(
    -Wall
    -Wextra
    -Wformat
    -Wformat-security
    -D_FORTIFY_SOURCE=2
    -march=armv8-a
    -mtune=cortex-a55
    -mstrict-align
    -g
    -DGUEST
)

add_link_options(
    -Wl,-gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
    -static
    --specs=nosys.specs
    -z noexecstack
)

add_link_options(-T ${LINKER_SCRIPT})
