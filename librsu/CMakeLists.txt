cmake_minimum_required(VERSION 3.15.0)
project(socfpga_rsu_library C CXX ASM)

if(NOT DEFINED RSU_DEP_INCLUDES)
    message(FATAL_ERROR "define RSU_DEP_INCLUDES with include driectories of RTOS, drivers and fatfs")
endif()

add_library(socfpga_rsu OBJECT)
target_sources(socfpga_rsu PRIVATE)

#extra compilation option
#To suppress warnings from librsu
target_compile_options(socfpga_rsu PRIVATE
	-march=armv8-a+crc
)

set(RSU_OSAL_INC ${CMAKE_CURRENT_SOURCE_DIR}/platform/freeRTOS/dependency/freeRTOS-common/include)
set(RSU_PLAT_INC_BASE ${CMAKE_CURRENT_SOURCE_DIR}/platform/platform_include)

#Private includes
target_include_directories(socfpga_rsu PRIVATE ${RSU_DEP_INCLUDES})
target_include_directories(socfpga_rsu PRIVATE ${RSU_OSAL_INC})
target_include_directories(socfpga_rsu PRIVATE ${RSU_PLAT_INC_BASE})
target_include_directories(socfpga_rsu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/priv_include)

#public interface includes
target_include_directories(socfpga_rsu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB RSU_PLAT_SRCS 
	${CMAKE_CURRENT_SOURCE_DIR}/platform/freeRTOS/dependency/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/platform/freeRTOS/dependency/freeRTOS-common/*.c
)
file(GLOB RSU_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

#suppressing compiler warnings
set_source_files_properties(
  ${RSU_SRCS}
  PROPERTIES  COMPILE_FLAGS 
  "-Wno-format -Wno-format-security"
  )

set_source_files_properties(
	${CMAKE_CURRENT_SOURCE_DIR}/platform/freeRTOS/dependency/rsu_crc32_def.c
  	PROPERTIES  COMPILE_FLAGS 
	"-Wno-int-conversion"
	)

target_sources(socfpga_rsu PRIVATE
	${RSU_PLAT_SRCS}
	${RSU_SRCS}
)

