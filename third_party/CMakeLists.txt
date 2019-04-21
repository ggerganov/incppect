if (APPLE)
    add_library(uWS SHARED
        uSockets/src/context.c
        uSockets/src/loop.c
        uSockets/src/socket.c
        uSockets/src/ssl.c
        uSockets/src/eventing/libuv.c
        )

    target_include_directories(uWS PRIVATE
        uSockets/src
        ${LIBUV_INCLUDE_DIR}
        )

    target_include_directories(uWS INTERFACE
        uSockets/src
        uWebSockets/src
        )

    target_link_libraries(uWS PRIVATE
        ${LIBUV_LIBRARIES}
        )

    target_link_libraries(uWS PUBLIC
        ${ZLIB_LIBRARIES}
        )

    target_compile_definitions(uWS PRIVATE LIBUS_NO_SSL=1 LIBUS_USE_LIBUV=1)
else ()
endif ()
