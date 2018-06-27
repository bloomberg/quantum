file(GLOB SOURCE_FILES
    *.cpp
    *.c
)

include_directories(AFTER
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests
    ${DPKG_REFROOT_INCLUDE}
)

link_directories(
    ${DPKG_REFROOT_LIB}
)

add_executable(tests ${SOURCE_FILES})

target_link_libraries(tests
    boost_context
    pthread
    gtest
    gmock
    gtest_main
    gmock_main
)

set_target_properties(tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/tests"
    RUNTIME_OUTPUT_NAME "${PROJECT_NAME}.${CMAKE_SYSTEM_NAME}${MODE}.tests.tsk"
)

if (QUANTUM_VERBOSE_MAKEFILE)
    message(STATUS "SOURCE_FILES = ${SOURCE_FILES}")
    get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
    get_property(link_dirs DIRECTORY PROPERTY LINK_DIRECTORIES)
    message(STATUS "INCLUDE_DIRECTORIES = ${inc_dirs}")
    message(STATUS "LINK_DIRECTORIES = ${link_dirs}")
endif()
