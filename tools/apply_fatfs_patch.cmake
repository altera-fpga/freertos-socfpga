# Reset FreeRTOS/Fat-Fs to a pinned base commit, then apply tools/fatfs.patch.
#
# Invoked from root CMakeLists.txt (configure time):
#   cmake -DROOT_DIR=<repo-root> -DFATFS_BASE_COMMIT=<sha> -P tools/apply_fatfs_patch.cmake

if(NOT DEFINED ROOT_DIR)
    message(FATAL_ERROR "ROOT_DIR is not set")
endif()

if(NOT DEFINED FATFS_BASE_COMMIT OR FATFS_BASE_COMMIT STREQUAL "")
    message(FATAL_ERROR "FATFS_BASE_COMMIT is not set")
endif()

set(PATCH_WORKDIR "${ROOT_DIR}/FreeRTOS/Fat-Fs")
set(PATCH_FILE    "${CMAKE_CURRENT_LIST_DIR}/fatfs.patch")

if(NOT EXISTS "${PATCH_WORKDIR}/.git")
    message(FATAL_ERROR
        "FatFs submodule not found at ${PATCH_WORKDIR}. "
        "Run: git submodule update --init FreeRTOS/Fat-Fs")
endif()

if(NOT EXISTS "${PATCH_FILE}")
    message(FATAL_ERROR "FatFs patch not found at ${PATCH_FILE}")
endif()

message(STATUS "FatFs: checking out base commit ${FATFS_BASE_COMMIT} in ${PATCH_WORKDIR}")

execute_process(
    COMMAND git -C "${PATCH_WORKDIR}" checkout --force ${FATFS_BASE_COMMIT}
    RESULT_VARIABLE _checkout_result
    ERROR_VARIABLE  _checkout_err
)

if(NOT _checkout_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to checkout FatFs base commit ${FATFS_BASE_COMMIT}:\n${_checkout_err}")
endif()

execute_process(
    COMMAND git -C "${PATCH_WORKDIR}" clean -fd
    RESULT_VARIABLE _clean_result
    ERROR_VARIABLE  _clean_err
)

if(NOT _clean_result EQUAL 0)
    message(FATAL_ERROR "Failed to clean FatFs submodule:\n${_clean_err}")
endif()

message(STATUS "Applying FatFs patch: ${PATCH_FILE}")

execute_process(
    COMMAND git -C "${PATCH_WORKDIR}" am --3way "${PATCH_FILE}"
    RESULT_VARIABLE _apply_result
    ERROR_VARIABLE  _apply_err
)

if(NOT _apply_result EQUAL 0)
    message(FATAL_ERROR "Failed to apply FatFs patch:\n${_apply_err}")
endif()

message(STATUS "FatFs patch applied successfully")