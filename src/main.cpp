/*! \file main.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "App.h" // uWebSockets
#include "Common.h"

#include <thread>
#include <map>

const int kPort = 3000;

int main() {
    /* ws->getUserData returns one of these */
    struct PerSocketData {
        int clientId = 0;

        uWS::Loop * mainLoop = nullptr;
        uWS::WebSocket<false, true> * ws = nullptr;
    };

    std::map<int, PerSocketData *> wsClients;

    auto server = [&]() {
        uWS::App().ws<PerSocketData>("/data/test", {
            .compression = uWS::SHARED_COMPRESSOR,
                .maxPayloadLength = 16 * 1024,
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
            res->end(kPageIndex);
        }).listen(kPort, [](auto *token) {
            if (token) {
                std::cout << "Listening on port " << kPort << std::endl;
            }
        }).run();
    };

    std::thread worker(server);

    while (true) {
        int a = rand()%10000;
        for (auto & [_, wsData] : wsClients) {
            if (wsData->mainLoop && wsData->ws) {
                wsData->mainLoop->defer([&, ws = wsData->ws]() {
                    ws->send("Connected clients: " + std::to_string(wsClients.size()), uWS::OpCode::TEXT);
                });
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    worker.join();

    return 0;
}
