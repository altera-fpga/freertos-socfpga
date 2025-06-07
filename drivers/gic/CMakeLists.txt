target_sources(socfpga_drivers PRIVATE
    socfpga_GIC.c
    socfpga_gic_reg.c
    socfpga_interrupt.c
    socfpga_interrupt.h
    )

target_include_directories(socfpga_drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
