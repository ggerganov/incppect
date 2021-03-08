if (NOT INCPPECT_NO_SSL)
    find_package(OpenSSL REQUIRED)
else ()
    unset(OPENSSL_LIBRARIES)
    unset(OPENSSL_INCLUDE_DIR)
endif ()

if (APPLE)
    add_library(uWS SHARED
        uWebSockets/uSockets/src/context.c
        uWebSockets/uSockets/src/loop.c
        uWebSockets/uSockets/src/socket.c
        uWebSockets/uSockets/src/crypto/openssl.c
        uWebSockets/uSockets/src/eventing/libuv.c
        )

    target_include_directories(uWS PRIVATE
        uWebSockets/uSockets/src
        ${LIBUV_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        )

    target_include_directories(uWS INTERFACE
        uWebSockets/uSockets/src
        uWebSockets/src
        )

    target_link_libraries(uWS PRIVATE
        ${LIBUV_LIBRARIES}
        )

    target_link_libraries(uWS PUBLIC
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

    target_compile_definitions(uWS PRIVATE LIBUS_USE_LIBUV=1)
elseif (WIN32)
    message(FATAL_ERROR "Windows build not yet supported")
else ()
    add_library(uWS SHARED
        uWebSockets/uSockets/src/context.c
        uWebSockets/uSockets/src/loop.c
        uWebSockets/uSockets/src/socket.c
        uWebSockets/uSockets/src/crypto/openssl.c
        uWebSockets/uSockets/src/eventing/epoll_kqueue.c
        uWebSockets/uSockets/src/eventing/gcd.c
        )

    target_include_directories(uWS PRIVATE
        uWebSockets/uSockets/src
        ${OPENSSL_INCLUDE_DIR}
        )

    target_include_directories(uWS INTERFACE
        uWebSockets/uSockets/src
        uWebSockets/src
        )

    target_link_libraries(uWS PUBLIC
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )
endif ()

if (INCPPECT_NO_SSL)
    target_compile_options(uWS PRIVATE -DLIBUS_NO_SSL=1)
else()
    target_compile_options(uWS PRIVATE -DLIBUS_USE_OPENSSL=1)
endif ()
