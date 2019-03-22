# Local function to auto-generate main quantum.h header file
function(make_quantum_header)
    set(QUANTUM_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/quantum.h)
    file(GLOB INCLUDE_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            "*.h"
            "interface/*.h"
            "util/*.h")
    list(SORT INCLUDE_HEADERS)
    foreach(header ${INCLUDE_HEADERS})
        if (NOT ${header} MATCHES "quantum.h")
            SET(QUANTUM_HEADERS "${QUANTUM_HEADERS}#include <quantum/${header}>\n")
        endif()
    endforeach()

    #create file from template
    configure_file(${PROJECT_SOURCE_DIR}/quantum.h.in ${QUANTUM_HEADER} @ONLY)
endfunction()

# Run file generation function
make_quantum_header()

# Install headers including the auto-generated quantum.h
file(GLOB INCLUDE_FILES "*.h")
file(GLOB IMPL_INCLUDE_FILES "impl/*.h")
file(GLOB INTERFACE_INCLUDE_FILES "interface/*.h")
file(GLOB UTIL_INCLUDE_FILES "util/*.h")
file(GLOB UTIL_IMPL_INCLUDE_FILES "util/impl/*.h")

install(
    FILES ${INCLUDE_FILES}
    DESTINATION include/quantum
    COMPONENT headers
)
install(
    FILES ${IMPL_INCLUDE_FILES}
    DESTINATION include/quantum/impl/
    COMPONENT headers
)
install(
    FILES ${INTERFACE_INCLUDE_FILES}
    DESTINATION include/quantum/interface/
    COMPONENT headers
)
install(
    FILES ${UTIL_INCLUDE_FILES}
    DESTINATION include/quantum/util/
    COMPONENT headers
)
install(
    FILES ${UTIL_IMPL_INCLUDE_FILES}
    DESTINATION include/quantum/util/impl
    COMPONENT headers
)
add_custom_target(headers-install
    COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=headers -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
    EXCLUDE_FROM_ALL
)
