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
    configure_file(${PROJECT_SOURCE_DIR}/cmake/quantum.h.in ${QUANTUM_HEADER} @ONLY)
endfunction()

# Run file generation function
make_quantum_header()

set(PKG_DIR             "${CMAKE_BINARY_DIR}/package")
set(PKGCONFIG_FILE      "${PKG_DIR}/${PROJECT_TARGET_NAME}.pc")
set(TARGET_CONFIG_FILE  "${PKG_DIR}/${PROJECT_NAME}Config.cmake")
set(TARGET_VERSION_FILE "${PKG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(TARGET_EXPORT_NAME   ${PROJECT_NAME}Targets)
set(NAMESPACE           "${PROJECT_NAME}::")

add_library(${PROJECT_TARGET_NAME} INTERFACE)
set(QUANTUM_DEPENDENCIES Boost::context pthread)
set_target_properties(${PROJECT_TARGET_NAME} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:include>
        INTERFACE_LINK_LIBRARIES "${QUANTUM_DEPENDENCIES}"
        INTERFACE_POSITION_INDEPENDENT_CODE ON
        INTERFACE_SOURCES "$<INSTALL_INTERFACE:include>/${PROJECT_TARGET_NAME}/quantum.h"
)

# Install target
install(
    TARGETS ${PROJECT_TARGET_NAME}
    EXPORT ${TARGET_EXPORT_NAME}
    COMPONENT headers
)

# Install headers
install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h")

if (QUANTUM_EXPORT_PKGCONFIG)
    # Generate package configuration file
    configure_file("${PROJECT_SOURCE_DIR}/cmake/quantum.pc.in" ${PKGCONFIG_FILE} @ONLY)

    # Install the pkgconfig file
    install(
        FILES ${PKGCONFIG_FILE}
        DESTINATION ${QUANTUM_PKGCONFIG_DIR}
        COMPONENT pkgconfig
    )
endif()

if (QUANTUM_EXPORT_CMAKE_CONFIG)
    # Install the exported file
    install(
        EXPORT ${TARGET_EXPORT_NAME}
        NAMESPACE "${NAMESPACE}"
        DESTINATION "${QUANTUM_CMAKE_CONFIG_DIR}"
    )

    # Generate CMAKE configuration file and exported targets
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${TARGET_CONFIG_FILE}"
        INSTALL_DESTINATION "${QUANTUM_CMAKE_CONFIG_DIR}"
        PATH_VARS CMAKE_INSTALL_PREFIX CMAKE_INSTALL_INCLUDEDIR
    )

    # Generate version file
    write_basic_package_version_file(
        "${TARGET_VERSION_FILE}"
        VERSION ${QUANTUM_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    install(
        FILES "${TARGET_CONFIG_FILE}" "${TARGET_VERSION_FILE}"
        DESTINATION "${QUANTUM_CMAKE_CONFIG_DIR}"
    )
endif()
