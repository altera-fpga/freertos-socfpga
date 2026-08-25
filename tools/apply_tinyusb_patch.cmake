# Reset tinyusb to a pinned base commit, then apply tools/tinyusb.patch
# (unified diff) with git apply.
#
# Invoked from root CMakeLists.txt (configure time):
#   cmake -DROOT_DIR=<repo-root> -DTINYUSB_BASE_COMMIT=<sha> -P tools/apply_tinyusb_patch.cmake
#
# Note: tools/tinyusb.patch must be a single unified diff against TINYUSB_BASE_COMMIT
# (e.g. git diff <base>..<tip>), not a format-patch / git-am mailbox. The TinyUSB
# history in this range is merge-heavy; git am does not apply it cleanly.

if(NOT DEFINED ROOT_DIR)
    message(FATAL_ERROR "ROOT_DIR is not set")
endif()

if(NOT DEFINED TINYUSB_BASE_COMMIT OR TINYUSB_BASE_COMMIT STREQUAL "")
    message(FATAL_ERROR "TINYUSB_BASE_COMMIT is not set")
endif()

set(PATCH_WORKDIR "${ROOT_DIR}/tinyusb")
set(PATCH_FILE    "${CMAKE_CURRENT_LIST_DIR}/tinyusb.patch")

if(NOT EXISTS "${PATCH_WORKDIR}/.git")
    message(FATAL_ERROR
        "TinyUSB submodule not found at ${PATCH_WORKDIR}. "
        "Run: git submodule update --init tinyusb")
endif()

if(NOT EXISTS "${PATCH_FILE}")
    message(FATAL_ERROR "TinyUSB patch not found at ${PATCH_FILE}")
endif()

message(STATUS "TinyUSB: checking out base commit ${TINYUSB_BASE_COMMIT} in ${PATCH_WORKDIR}")

# Abort any in-progress git am from a previous failed run before resetting.
execute_process(
    COMMAND git -C "${PATCH_WORKDIR}" am --abort
    OUTPUT_QUIET ERROR_QUIET
)

execute_process(
    COMMAND git -C "${PATCH_WORKDIR}" checkout --force ${TINYUSB_BASE_COMMIT}
    RESULT_VARIABLE _checkout_result
    ERROR_VARIABLE  _checkout_err
)

if(NOT _checkout_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to checkout TinyUSB base commit ${TINYUSB_BASE_COMMIT}:\n${_checkout_err}")
endif()

execute_process(
    COMMAND git -C "${PATCH_WORKDIR}" clean -fd
    RESULT_VARIABLE _clean_result
    ERROR_VARIABLE  _clean_err
    OUTPUT_QUIET
)

if(NOT _clean_result EQUAL 0)
    message(FATAL_ERROR "Failed to clean TinyUSB submodule:\n${_clean_err}")
endif()

message(STATUS "Applying TinyUSB patch: ${PATCH_FILE}")

execute_process(
    COMMAND git -C "${PATCH_WORKDIR}" apply --whitespace=nowarn "${PATCH_FILE}"
    RESULT_VARIABLE _apply_result
    ERROR_VARIABLE  _apply_err
    OUTPUT_VARIABLE _apply_out
)

if(NOT _apply_result EQUAL 0)
    message(FATAL_ERROR "Failed to apply TinyUSB patch:\n${_apply_out}${_apply_err}")
endif()

message(STATUS "TinyUSB patch applied successfully")
