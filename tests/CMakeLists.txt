include(GoogleTest)
set(TEST_TARGET ${PROJECT_NAME}Tests)
file(GLOB SOURCE_FILES *.cpp)
include_directories(AFTER
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${GTEST_ROOT}
)
link_directories(
    ${Boost_LIBRARY_DIRS}
    ${GTEST_ROOT}
)
add_executable(${TEST_TARGET} ${SOURCE_FILES})
gtest_discover_tests(${TEST_TARGET}
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${TEST_TARGET}
    Boost::context
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
add_test(NAME ${TEST_TARGET}
         COMMAND ${TEST_TARGET})
