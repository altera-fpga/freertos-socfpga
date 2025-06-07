#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/apps)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/startup)

set(MAIN_SOURCE_SRCS
    ${MAIN_SOURCE_SRCS}
    PARENT_SCOPE
)
