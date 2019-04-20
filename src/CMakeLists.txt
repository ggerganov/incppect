file(READ "incppect.js" src_incppect_js)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/common.h.in ${CMAKE_CURRENT_SOURCE_DIR}/common.h @ONLY)

add_library(incppect SHARED incppect.cpp)
target_include_directories(incppect PUBLIC ../include)
target_include_directories(incppect PRIVATE .)
target_link_libraries(incppect PRIVATE uWS)

if (INCPPECT_DEBUG)
    target_compile_definitions(incppect PRIVATE INCPPECT_DEBUG=1)
endif ()
