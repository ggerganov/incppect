/*! \file main.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "App.h" // uWebSockets

int main() {
    /* Overly simple hello world app */
    uWS::App().get("/*", [](auto *res, auto *req) {
        res->end("Hello world!");
    }).listen(3000, [](auto *token) {
        if (token) {
            std::cout << "Listening on port " << 3000 << std::endl;
        }
    }).run();

    std::cout << "Failed to listen on port 3000" << std::endl;

    return 0;
}
