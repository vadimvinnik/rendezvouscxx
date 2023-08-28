cmake_minimum_required(VERSION 3.25)

project(rendezvouscxx
    VERSION 0.1.0
    DESCRIPTION "Rendez-vous synchronisation mechanism for C++"
    HOMEPAGE_URL https://github.com/vadimvinnik/rendezvouscxx
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(GNUInstallDirs)
include(dependencies)

set(RENDEZVOUSCXX_BASE_ALIAS ${PROJECT_NAME} CACHE INTERNAL "Base targets alias" FORCE)
set(RENDEZVOUSCXX_CXX_STANDARD 17 CACHE INTERNAL "Project c++ standard version" FORCE)

option(rendezvouscxx_BUILD_TESTS "Enable build demo target" ON)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${RENDEZVOUSCXX_BASE_ALIAS}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources(${PROJECT_NAME}
    INTERFACE
        FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include
            FILES
                include/rendezvouscxx.hpp
                include/i_server.hpp
                include/simple_server_base.hpp
)

set_target_properties(${PROJECT_NAME} 
    PROPERTIES
        CXX_STANDARD ${RENDEZVOUSCXX_CXX_STANDARD}
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF
)

if (rendezvouscxx_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(CMakePackageConfigHelpers)

    install(TARGETS ${PROJECT_NAME}
        EXPORT ${RENDEZVOUSCXX_BASE_ALIAS}Targets
        FILE_SET HEADERS
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ${RENDEZVOUSCXX_BASE_ALIAS}Targets
        FILE ${RENDEZVOUSCXX_BASE_ALIAS}Targets.cmake
        NAMESPACE ${RENDEZVOUSCXX_BASE_ALIAS}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${RENDEZVOUSCXX_BASE_ALIAS}
    )

    configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${RENDEZVOUSCXX_BASE_ALIAS}Config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${RENDEZVOUSCXX_BASE_ALIAS}"
        NO_SET_AND_CHECK_MACRO
    )

    write_basic_package_version_file("${RENDEZVOUSCXX_BASE_ALIAS}ConfigVersion.cmake"
        COMPATIBILITY AnyNewerVersion
    )

    install(FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/${RENDEZVOUSCXX_BASE_ALIAS}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${RENDEZVOUSCXX_BASE_ALIAS}ConfigVersion.cmake
        DESTINATION 
            ${CMAKE_INSTALL_LIBDIR}/cmake/${RENDEZVOUSCXX_BASE_ALIAS}
    )
endif()
