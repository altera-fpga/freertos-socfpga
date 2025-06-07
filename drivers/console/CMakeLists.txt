target_sources(socfpga_drivers PRIVATE
    socfpga_console.c
    )

target_include_directories(socfpga_drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
