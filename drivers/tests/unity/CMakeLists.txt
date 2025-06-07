file(GLOB UNITY_TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

target_sources(${EXECUTABLE_NAME} PRIVATE
    ${UNITY_TEST_SRCS}
    )

target_include_directories(${EXECUTABLE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
