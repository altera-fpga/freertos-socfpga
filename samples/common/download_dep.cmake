cmake_minimum_required(VERSION 3.5...3.28)

# Set SOF Path
if(NOT DEFINED SOF_PATH)
    set(SOF_PATH "${CMAKE_SOURCE_DIR}/ghrd_a5ed065bb32ae6sr0.sof" CACHE STRING "SOF File path" FORCE)
    if(SOC STREQUAL "AGILEX3")
        set(SOF_PATH "${CMAKE_SOURCE_DIR}/ghrd_a3cw135bm16ae6s.sof" CACHE STRING "SOF File path" FORCE)
    endif()
endif()

# Default values for when -DSOC parameter is not used
set(SOF_FILE_SEARCH_WORD "a3cw135bm16ae6s")
set(SOF_FILE_REPLACE_WORD "a5ed065bb32ae6sr0")

if(SOC STREQUAL "AGILEX3")
    set(SOF_FILE_SEARCH_WORD "a5ed065bb32ae6sr0")
    set(SOF_FILE_REPLACE_WORD "a3cw135bm16ae6s")
endif()

set(QSPI_PFG_SRC "${CMAKE_CURRENT_LIST_DIR}/../../tools/qspi_flash_image_agilex5_boot.pfg")
set(SDMMC_PFG_SRC "${CMAKE_CURRENT_LIST_DIR}/../../tools/qspi_flash_image_agilex5_sdmmc.pfg")
set(DST_DIR "${CMAKE_BINARY_DIR}")

if(NOT DEFINED PFG_QSPI)
    set(PFG_QSPI
        "${CMAKE_BINARY_DIR}/qspi_flash_image_agilex5_boot.pfg"
        CACHE STRING "PFG File path" FORCE
    )
    if(NOT EXISTS "${PFG_QSPI}")
        message(STATUS "Copying QSPI PFG from Tools")
        if(NOT EXISTS "${QSPI_PFG_SRC}")
            message(FATAL_ERROR "QSPI PFG source not found: ${QSPI_PFG_SRC}")
        endif()
        file(COPY "${QSPI_PFG_SRC}" DESTINATION "${DST_DIR}")
    endif()
    file(READ "${PFG_QSPI}" pfg_content)
    string(REPLACE
        ${SOF_FILE_SEARCH_WORD}
        ${SOF_FILE_REPLACE_WORD}
        pfg_content
        "${pfg_content}"
    )
    file(WRITE "${PFG_QSPI}" "${pfg_content}")
else()
    if(NOT EXISTS "${PFG_QSPI}")
        message(FATAL_ERROR "PFG_QSPI not found: ${PFG_QSPI}")
    endif()
endif()

if(NOT DEFINED PFG_SDMMC)
    set(PFG_SDMMC
        "${CMAKE_BINARY_DIR}/qspi_flash_image_agilex5_sdmmc.pfg"
        CACHE STRING "SD/MMC PFG FIle path" FORCE
    )
    if(NOT EXISTS "${PFG_SDMMC}")
        message(STATUS "Copying SDMMC PFG from Tools")
        if(NOT EXISTS "${SDMMC_PFG_SRC}")
            message(FATAL_ERROR "SDMMC PFG source not found: ${SDMMC_PFG_SRC}")
        endif()
        file(COPY "${SDMMC_PFG_SRC}" DESTINATION "${DST_DIR}")
    endif()
    file(READ "${PFG_SDMMC}" pfg_content)
    string(REPLACE
        ${SOF_FILE_SEARCH_WORD}
        ${SOF_FILE_REPLACE_WORD}
        pfg_content
        "${pfg_content}"
    )
    file(WRITE "${PFG_SDMMC}" "${pfg_content}")
else()
    if(NOT EXISTS "${PFG_SDMMC}")
        message(FATAL_ERROR "PFG_SDMMC not found: ${PFG_SDMMC}")
    endif()
endif()
