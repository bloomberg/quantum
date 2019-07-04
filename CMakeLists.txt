cmake_minimum_required(VERSION 3.12.4)
cmake_policy(SET CMP0074 NEW) #Prior to 3.12 this allows using '<Package>_ROOT' to point to an package location
project(Quantum LANGUAGES CXX)
set(PROJECT_TARGET_NAME quantum)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# CMake options
option(QUANTUM_BUILD_DOC "Build documentation" OFF)
option(QUANTUM_ENABLE_DOT "Enable generation of DOT viewer files" OFF)
option(QUANTUM_VERBOSE_MAKEFILE "Enable verbose cmake output" ON)
option(QUANTUM_ENABLE_TESTS "Generate 'tests' target" OFF)
option(QUANTUM_BOOST_STATIC_LIBS "Link with Boost static libraries." ON)
option(QUANTUM_BOOST_USE_MULTITHREADED "Use Boost multithreaded libraries." ON)
option(QUANTUM_BOOST_USE_VALGRIND "Use valgrind headers for Boost." OFF)
option(QUANTUM_USE_DEFAULT_ALLOCATOR "Use default system supplied allocator instead of Quantum's." OFF)
option(QUANTUM_ALLOCATE_POOL_FROM_HEAP "Pre-allocates object pools from heap instead of the application stack." OFF)
option(QUANTUM_BOOST_USE_SEGMENTED_STACKS "Use Boost segmented stacks for coroutines." OFF)
option(QUANTUM_BOOST_USE_PROTECTED_STACKS "Use Boost protected stacks for coroutines." OFF)
option(QUANTUM_BOOST_USE_FIXEDSIZE_STACKS "Use Boost fixed size stacks for coroutines." OFF)
option(QUANTUM_EXPORT_PKGCONFIG "Generate 'quantum.pc' file" ON)
option(QUANTUM_EXPORT_CMAKE_CONFIG "Generate CMake config, target and version files." ON)

set(QUANTUM_VERSION_MAJOR 2)
set(QUANTUM_VERSION_MINOR 0)
set(QUANTUM_VERSION_REVISION 1)
set(QUANTUM_VERSION "${QUANTUM_VERSION_MAJOR}.${QUANTUM_VERSION_MINOR}.${QUANTUM_VERSION_REVISION}")

if (QUANTUM_INSTALL_ROOT)
    set(CMAKE_INSTALL_PREFIX ${QUANTUM_INSTALL_ROOT})
endif()
if (NOT QUANTUM_PKGCONFIG_DIR)
    set(QUANTUM_PKGCONFIG_DIR share/pkgconfig)
endif()
if (NOT QUANTUM_CMAKE_CONFIG_DIR)
    set(QUANTUM_CMAKE_CONFIG_DIR share/cmake/${PROJECT_NAME})
endif()

#Global options
set(MODE 64)
set(LINKER_LANGUAGE CXX)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_BUILD_TYPE Debug)

#Set the compiler if the CXX environment variable is not set
if (NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER g++)
endif()
#Set the compiler if the CXX_STANDARD environment variable is not set
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()
#Set compile flags if CXXFLAGS environment variable is not set
if (NOT CMAKE_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -O0 -m${MODE} -std=c++${CMAKE_CXX_STANDARD} -ftemplate-backtrace-limit=0")
endif()
if (QUANTUM_VERBOSE_MAKEFILE)
    message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
endif()

# Determine if this is a 32 or 64 bit build
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")

# Properly set the output directory
if (${BITS} EQUAL 64)
    set(LIBDIR "lib64")
else()
    set(LIBDIR "lib")
endif()

add_definitions(
        -D_REENTRANT
        -D_THREAD_SAFE
        -D_POSIX_PTHREAD_SEMANTICS
        -D__FUNCTION__=__FILE__
        -D_GLIBCXX_USE_CXX11_ABI=1
)
if (QUANTUM_USE_DEFAULT_ALLOCATOR)
    add_definitions(-D__QUANTUM_USE_DEFAULT_ALLOCATOR)
endif()
if (QUANTUM_ALLOCATE_POOL_FROM_HEAP)
    add_definitions(-D__QUANTUM_ALLOCATE_POOL_FROM_HEAP)
endif()

if (QUANTUM_BUILD_DOC)
    message(STATUS "Generating Doxygen configuration files")
    # Add a target to generate API documentation using Doxygen
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # Configure Doxygen parameters
        set(DOXYGEN_PROJECT_NAME "Quantum Library")
        set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")
        set(DOXYGEN_INPUT "${PROJECT_SOURCE_DIR}/${PROJECT_TARGET_NAME}")
        set(DOXYGEN_HTML_OUTPUT "${PROJECT_SOURCE_DIR}/docs")
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
    if (QUANTUM_BOOST_USE_SEGMENTED_STACKS)
        add_definitions(-D__QUANTUM_BOOST_USE_SEGMENTED_STACKS)
    elseif (QUANTUM_BOOST_USE_PROTECTED_STACKS)
        add_definitions(-D__QUANTUM_BOOST_USE_PROTECTED_STACKS)
    elseif (QUANTUM_BOOST_USE_FIXEDSIZE_STACKS)
        add_definitions(-D__QUANTUM_BOOST_USE_FIXEDSIZE_STACKS)
    endif()
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
    message(FATAL_ERROR "Boost not found, please define BOOST_ROOT.")
endif()

add_subdirectory(${PROJECT_TARGET_NAME})

if (QUANTUM_ENABLE_TESTS)
    find_package(GTest REQUIRED)
    if (GTEST_FOUND)
        message(STATUS "Adding target '${PROJECT_TARGET_NAME}_tests' to build output")
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "Package GTEST not found. Skipping testing.")
    endif()
else()
    set(BUILD_TESTING OFF)
    message(STATUS "Skipping target 'tests'")
endif()

# Debug info
if (QUANTUM_VERBOSE_MAKEFILE)
    message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}/")
    message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "QUANTUM_PKGCONFIG_DIR = ${QUANTUM_PKGCONFIG_DIR}")
    message(STATUS "BOOST_ROOT = ${BOOST_ROOT}")
    message(STATUS "REQUIRED BOOST_VERSION = 1.61")
    message(STATUS "GTEST_ROOT = ${GTEST_ROOT}")
endif()
