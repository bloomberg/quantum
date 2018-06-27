cmake_minimum_required(VERSION 3.9)
project(quantum LANGUAGES CXX)

# CMake options
option(QUANTUM_BUILD_DOC "Build documentation" ON)
option(QUANTUM_ENABLE_DOT "Enable generation of DOT files" OFF)
option(QUANTUM_VERBOSE_MAKEFILE "Enable verbose cmake output" ON)
option(QUANTUM_PRINT_DEBUG "Enable debug printing at runtime" OFF)

#Global options
set(MODE 64)
set(GCC_VER 8.1)
#set(CMAKE_CXX_COMPILER ${DPKG_REFROOT}/opt/bb/lib/gcc-${GCC_VER}/bin/g++)
set(CMAKE_CXX_COMPILER /opt/bb/bin/g++)
set(CMAKE_CXX_STANDARD 14)
set(LINKER_LANGUAGE CXX)
set(CMAKE_SYSTEM_NAME linux)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_BUILD_TYPE Debug)

if (QUANTUM_VERBOSE_MAKEFILE)
    message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}/")
    message(STATUS "DPKG_REFROOT = ${DPKG_REFROOT}/")
endif()
set(DPKG_REFROOT_INCLUDE ${DPKG_REFROOT}/opt/bb/include)
set(DPKG_REFROOT_LIB ${DPKG_REFROOT}/opt/bb/lib${MODE})

if (QUANTUM_BUILD_DOC)
    message(STATUS "Generating Doxygen configuration files")
    # Add a target to generate API documentation using Doxygen
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # Configure Doxygen parameters
        set(DOXYGEN_PROJECT_NAME "Quantum Library")
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)
        set(DOXYGEN_INPUT ${CMAKE_CURRENT_BINARY_DIR}/../${PROJECT_NAME})
        set(DOXYGEN_HTML_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../docs)
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
    message(STATUS "Doxygen configuration files have not been generated. "
            "Use BUILD_DOC=ON to generate them.")
endif()

add_definitions(
        -D_REENTRANT
        -D_THREAD_SAFE
        -D_POSIX_PTHREAD_SEMANTICS
        -D__FUNCTION__=__FILE__
        -DBOOST_THREAD_PROVIDES_INTERRUPTIONS
)

if (QUANTUM_PRINT_DEBUG)
    add_definitions(-D_QUANTUM_PRINT_DEBUG)
endif()

add_compile_options(-Wall -Wextra -Werror -Wabi -O0 -m${MODE} -std=gnu++${CMAKE_CXX_STANDARD} -ftemplate-backtrace-limit=0)

add_subdirectory(tests)
