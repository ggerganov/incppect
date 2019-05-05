add_library(incppect SHARED incppect.cpp)
target_include_directories(incppect PUBLIC ../include)
target_include_directories(incppect PRIVATE .)
target_link_libraries(incppect PRIVATE uWS)
