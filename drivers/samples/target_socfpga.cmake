
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR "arm64")
set(CMAKE_CROSSCOMPILING True)

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
)

add_link_options(
    -Wl,-gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
    -static
    --specs=nosys.specs
    -z noexecstack
)
