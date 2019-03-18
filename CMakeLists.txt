cmake_minimum_required(VERSION 3.10.3)
project(quantum LANGUAGES CXX)

# CMake options
option(QUANTUM_BUILD_DOC "Build documentation" OFF)
option(QUANTUM_ENABLE_DOT "Enable generation of DOT viewer files" OFF)
option(QUANTUM_VERBOSE_MAKEFILE "Enable verbose cmake output" ON)
option(QUANTUM_ENABLE_TESTS "Generate 'tests' target" OFF)
option(QUANTUM_BOOST_STATIC_LIBS "Link with Boost static libraries." ON)
option(QUANTUM_BOOST_USE_MULTITHREADED "Use Boost multithreaded libraries." ON)
option(QUANTUM_BOOST_USE_VALGRIND "Use valgrind headers for Boost." OFF)

if (QUANTUM_INSTALL_ROOT)
    set(CMAKE_INSTALL_PREFIX ${QUANTUM_INSTALL_ROOT})
endif()

#Global options
set(MODE 64)
set(LINKER_LANGUAGE CXX)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_BUILD_TYPE Debug)

#Set the compiler if the CXX environment variable is not set
if (NOT DEFINED ENV{CXX})
    set(CMAKE_CXX_COMPILER g++)
endif()
#Set the compiler if the CXX_STANDARD environment variable is not set
if (NOT DEFINED ENV{CXXSTANDARD})
    set(CMAKE_CXX_STANDARD 14)
else()
    set(CMAKE_CXX_STANDARD $ENV{CXXSTANDARD})
endif()
#Set compile flags if CXXFLAGS environment variable is not set
if (NOT DEFINED ENV{CXXFLAGS})
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wabi -O0 -m${MODE} -std=c++${CMAKE_CXX_STANDARD} -ftemplate-backtrace-limit=0")
endif()
if (QUANTUM_VERBOSE_MAKEFILE)
    message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
endif()

add_definitions(
        -D_REENTRANT
        -D_THREAD_SAFE
        -D_POSIX_PTHREAD_SEMANTICS
        -D__FUNCTION__=__FILE__
)

if (QUANTUM_BUILD_DOC)
    message(STATUS "Generating Doxygen configuration files")
    # Add a target to generate API documentation using Doxygen
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # Configure Doxygen parameters
        set(DOXYGEN_PROJECT_NAME "Quantum Library")
        set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
        set(DOXYGEN_INPUT ${PROJECT_SOURCE_DIR}/src)
        set(DOXYGEN_HTML_OUTPUT ${PROJECT_SOURCE_DIR}/docs)
        set(DOXYGEN_CREATE_SUBDIRS YES)
        if (DOXYGEN_DOT_FOUND AND QUANTUM_ENABLE_DOT)
            set(DOXYGEN_HAVE_DOT YES)
        else()
            set(DOXYGEN_HAVE_DOT NO)
        endif()
        # set input and output files
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    endif()
else()
    message(STATUS "Doxygen configuration files have not been generated. Use QUANTUM_BUILD_DOC=ON to generate them.")
endif()

# Look for Boost
find_package(Boost 1.61 REQUIRED COMPONENTS context)
if (Boost_FOUND)
    if (QUANTUM_BOOST_USE_VALGRIND)
        add_definitions(-DBOOST_USE_VALGRIND)
    endif()
    set(Boost_USE_STATIC_LIBS ${QUANTUM_BOOST_STATIC_LIBS})
    set(Boost_USE_MULTITHREADED ${QUANTUM_BOOST_USE_MULTITHREADED})
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
    if (QUANTUM_VERBOSE_MAKEFILE)
        message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
        message(STATUS "Boost library dir: ${Boost_LIBRARY_DIRS}")
        message(STATUS "Boost use static libs: ${Boost_USE_STATIC_LIBS}")
        message(STATUS "Boost is multi-threaded: ${Boost_USE_MULTITHREADED}")
        message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
    endif()
else()
    message(FATAL_ERROR "Boost not found")
endif()

if (QUANTUM_ENABLE_TESTS)
    find_package(GTest REQUIRED)
    if (GTEST_FOUND)
        message(STATUS "Adding target 'quantum_tests' to build output")
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "Package GTEST not found. Skipping testing.")
    endif()
else()
    set(BUILD_TESTING OFF)
    message(STATUS "Skipping target 'tests'")
endif()

add_subdirectory(src)

# Debug info
if (QUANTUM_VERBOSE_MAKEFILE)
    message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}/")
    message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "BOOST_ROOT = ${BOOST_ROOT}")
    message(STATUS "REQUIRED BOOST_VERSION = 1.61")
    message(STATUS "GTEST_ROOT = ${GTEST_ROOT}")
endif()
