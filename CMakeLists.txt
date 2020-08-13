cmake_minimum_required (VERSION 3.10)
project(incppect)

set(CMAKE_EXPORT_COMPILE_COMMANDS "on")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif ()

option(INCPPECT_DEBUG   "Enable debug messages in the incppect service" ON)
option(INCPPECT_NO_SSL  "Disable SSL support" OFF)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

if (APPLE)
    find_package(LibUV REQUIRED)
endif ()

add_subdirectory(third_party)
add_subdirectory(src)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory(examples)
endif ()
