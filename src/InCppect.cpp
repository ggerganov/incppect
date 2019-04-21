/*! \file InCppect.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "InCppect.h"

#include "App.h" // uWebSockets

#include <map>
#include <string>
#include <fstream>

struct InCppect::Impl {
    struct PerSocketData {
        int clientId = 0;

        uWS::Loop * mainLoop = nullptr;
        uWS::WebSocket<false, true> * ws = nullptr;
    };

    bool init() {
        return true;
    }

    void run() {
        mainLoop = uWS::Loop::get();

        uWS::App().ws<PerSocketData>("/data/test", {
            .compression = uWS::SHARED_COMPRESSOR,
                .maxPayloadLength = 16*1024,
                .open = [&](auto *ws, auto *req) {
                    static int uniqueId = 1;
                    ++uniqueId;

                    auto perSocketData = (PerSocketData *) ws->getUserData();
                    perSocketData->clientId = uniqueId;
                    perSocketData->ws = ws;
                    perSocketData->mainLoop = uWS::Loop::get();
                    wsClients.insert({ uniqueId, perSocketData });

                    std::cout << "[+] Client with Id = " << perSocketData->clientId  << " connected\n";
                },
                .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    ws->send(message, opCode);
                },
                .drain = [](auto *ws) {
                    /* Check getBufferedAmount here */
                },
                .ping = [](auto *ws) {

                },
                .pong = [](auto *ws) {

                },
                .close = [&](auto *ws, int code, std::string_view message) {
                    auto perSocketData = (PerSocketData *) ws->getUserData();
                    std::cout << "[+] Client with Id = " << perSocketData->clientId  << " disconnected\n";

                    wsClients.erase(perSocketData->clientId);
                }
        }).get("/*", [](auto *res, auto *req) {
            std::ifstream t("../data/index.html");
            std::string str((std::istreambuf_iterator<char>(t)),
                            std::istreambuf_iterator<char>());
            res->end(str);
        }).listen(port, [this](auto *token) {
            if (token) {
                std::cout << "Listening on port " << port << std::endl;
            }
        }).run();
    }

    int port = 3000;

    uWS::Loop * mainLoop = nullptr;
    std::map<int, PerSocketData *> wsClients;
};

InCppect::InCppect() : m_impl(new Impl()) {}

InCppect::~InCppect() {}

bool InCppect::init(int port) {
    m_impl->port = port;

    return m_impl->init();
}

void InCppect::run() {
    m_impl->run();
}
