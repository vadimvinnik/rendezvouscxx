project(rendevouscxx_test
    VERSION 0.1.0
    LANGUAGES CXX
)

find_package(Catch2 2 REQUIRED)

add_executable(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} 
    PROPERTIES
        CXX_STANDARD ${RENDEZVOUSCXX_CXX_STANDARD}
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)

list(APPEND SOURCES
    single_connection_test.cpp
    multiple_connection_test.cpp
    closing_gate_test.cpp
)

target_sources(${PROJECT_NAME} 
    PRIVATE 
        ${SOURCES}
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        rendezvouscxx::rendezvouscxx
        Catch2::Catch2WithMain
)

# CTest
add_test(NAME ${PROJECT_NAME} 
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)

#add_custom_command(
#     TARGET ${PROJECT_NAME}
#     COMMENT "Run tests"
#     POST_BUILD
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#     COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "^${PROJECT_NAME}$" --output-on-failures
#)
