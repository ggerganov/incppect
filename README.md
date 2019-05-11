# incppect

Inspect C++ memory in the browser

## Description

This is a small library that allows a C++ native application to stream memory bits to one or more websocket clients. This functionality can be used to conveniently inspect the internal state of the native application from a browser.

Incppect starts a simple HTTP/WebSocket server in your application that accepts external connections. When a client connects, incppect serves the static content (HTML/JS) from a user-specified location, as well as the built-in incppect JS client [incppect.js](https://github.com/ggerganov/incppect/blob/master/src/incppect.js). The client opens a websocket connection back to the application and starts requesting custom data. The data is streamed back to the client over the websocket. The usage/visualization of the received data is entirely up to the client.

The HTTP/WebSocket server is implemented via the (uWebSocket)[https://github.com/uNetworking/uWebSockets) library (included in the project as a submodule).

## Examples:

<a href="https://i.imgur.com/8hJSbzQ.gif" target="_blank">![incppect-balls2d](https://i.imgur.com/8hJSbzQ.gif)</a>

Checkout the [examples] folder for sample applications.

## Build instructions

**Linux and Mac OS**

    git clone https://github.com/ggerganov/incppect
    cd incppect
    git submodule update --init
    mkdir build && cd build
    cmake ..
    make

**Windows**

    ??
    
