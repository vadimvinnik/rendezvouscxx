
include_guard(GLOBAL)

cmake_policy(SET CMP0077 NEW) # see https://cmake.org/cmake/help/latest/policy/CMP0077.html

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Turn off shared library target")

block()
    if(NOT catch2_REQUIRED_VERSION)
        set(catch2_REQUIRED_VERSION v3.4.0) # By default
    endif()

    FetchContent_Declare(
        catch2
        GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
        GIT_TAG         ${catch2_REQUIRED_VERSION}
        GIT_SHALLOW     ON
        OVERRIDE_FIND_PACKAGE
    )
endblock()
