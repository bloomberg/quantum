include(GoogleTest)
set(TEST_TARGET quantum_tests)
file(GLOB SOURCE_FILES *.cpp)
include_directories(AFTER
    ${CMAKE_INSTALL_PREFIX}/include
    ${PROJECT_SOURCE_DIR}/tests
    ${BOOST_ROOT}
    ${GTEST_ROOT}
)
link_directories(
    ${BOOST_ROOT}
    ${GTEST_ROOT}
)
add_executable(${TEST_TARGET} ${SOURCE_FILES})
add_dependencies(${TEST_TARGET} headers-install)
gtest_discover_tests(${TEST_TARGET}
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${TEST_TARGET}
    ${Boost_LIBRARIES}
    GTest::GTest
    GTest::Main
    pthread
)
set_target_properties(${TEST_TARGET}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    RUNTIME_OUTPUT_NAME "${TEST_TARGET}.${CMAKE_SYSTEM_NAME}${MODE}"
)
if (QUANTUM_VERBOSE_MAKEFILE)
    message(STATUS "SOURCE_FILES = ${SOURCE_FILES}")
    get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
    get_property(link_dirs DIRECTORY PROPERTY LINK_DIRECTORIES)
    message(STATUS "INCLUDE_DIRECTORIES = ${inc_dirs}")
    message(STATUS "LINK_DIRECTORIES = ${link_dirs}")
endif()
add_custom_target(tests)
add_dependencies(tests ${TEST_TARGET})
add_test(${PROJECT_NAME} ${TEST_TARGET})
