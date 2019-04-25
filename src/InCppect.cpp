/*! \file InCppect.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "InCppect.h"

#include "App.h" // uWebSockets

#include <map>
#include <string>
#include <fstream>
#include <chrono>
#include <sstream>

namespace {
    inline int64_t timestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
}

struct InCppect::Impl {
    struct Request {
        int64_t tLastUpdated_ms = -1;
        int64_t tLastRequested_ms = -1;
        int64_t tMinUpdate_ms = 100;
        int64_t tLastRequestTimeout_ms = 3000;

        TIdxs idxs;
        int getterId = -1;

        std::vector<char> prevData;
        std::string_view curData;
    };

    struct ClientData {
        int64_t tConnected_ms = -1;

        std::vector<int> lastRequests;
        std::map<int, Request> requests;
    };

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

                    clientData[uniqueId].tConnected_ms = ::timestamp();

                    auto sd = (PerSocketData *) ws->getUserData();
                    sd->clientId = uniqueId;
                    sd->ws = ws;
                    sd->mainLoop = uWS::Loop::get();
                    socketData.insert({ uniqueId, sd });

                    std::cout << "[+] Client with Id = " << sd->clientId  << " connected\n";
                },
                .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    std::cout << "Received message size = " << message.size() << std::endl;
                    if (message.size() < sizeof(int)) {
                        return;
                    }

                    int * p = (int *) message.data();
                    int type = p[0];
                    auto sd = (PerSocketData *) ws->getUserData();

                    auto & cd = clientData[sd->clientId];

                    switch (type) {
                        case 1:
                            {
                                std::cout << "var_to_id: " << message.data() + 4 << std::endl;
                                std::stringstream ss(message.data() + 4);
                                while (true) {
                                    Request request;

                                    std::string path;
                                    ss >> path;
                                    if (ss.eof()) break;
                                    int requestId = 0;
                                    ss >> requestId;
                                    int nidxs = 0;
                                    ss >> nidxs;
                                    for (int i = 0; i < nidxs; ++i) {
                                        int idx = 0;
                                        ss >> idx;
                                        request.idxs.push_back(idx);
                                    }

                                    if (pathToGetter.find(path) != pathToGetter.end()) {
                                        printf("requestId = %d, path = '%s', nidxs = %d\n", requestId, path.c_str(), nidxs);
                                        request.getterId = pathToGetter[path];

                                        cd.requests[requestId] = std::move(request);
                                    } else {
                                        printf("Missing path '%s'\n", path.c_str());
                                    }
                                }
                            }
                            break;
                        case 2:
                            {
                                int nRequests = (message.size() - sizeof(int))/sizeof(int);
                                std::cout << "Received requests: " << nRequests << std::endl;

                                cd.lastRequests.clear();
                                for (int i = 0; i < nRequests; ++i) {
                                    int curRequest = p[i + 1];
                                    if (cd.requests.find(curRequest) != cd.requests.end()) {
                                        cd.lastRequests.push_back(curRequest);
                                        cd.requests[curRequest].tLastRequested_ms = ::timestamp();
                                    }
                                }
                            }
                            break;
                        case 3:
                            {
                                for (auto curRequest : cd.lastRequests) {
                                    if (cd.requests.find(curRequest) != cd.requests.end()) {
                                        cd.requests[curRequest].tLastRequested_ms = ::timestamp();
                                    }
                                }
                            }
                            break;
                        default:
                            std::cout << "Unknown message type = " << type << std::endl;
                    };
                    sd->mainLoop->defer([this]() { this->update(); });
                },
                .drain = [](auto *ws) {
                    /* Check getBufferedAmount here */
                    if (ws->getBufferedAmount() > 0) {
                        std::cout << "  [drain] Buffered amount = " << ws->getBufferedAmount() << std::endl;
                    }
                },
                .ping = [](auto *ws) {

                },
                .pong = [](auto *ws) {

                },
                .close = [&](auto *ws, int code, std::string_view message) {
                    auto sd = (PerSocketData *) ws->getUserData();
                    std::cout << "[+] Client with Id = " << sd->clientId  << " disconnected\n";

                    clientData.erase(sd->clientId);
                    socketData.erase(sd->clientId);
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

    void update() {
        for (auto & [clientId, cd] : clientData) {
            for (auto & [requestId, req] : cd.requests) {
                auto & getter = getters[req.getterId];
                auto tCur = ::timestamp();
                if (tCur - req.tLastRequested_ms < req.tLastRequestTimeout_ms &&
                    tCur - req.tLastUpdated_ms > req.tMinUpdate_ms) {
                    //printf("Update %d, idxs.size() = %d -> ", req.getterId, (int) req.idxs.size());
                    //for (auto & idx : req.idxs) printf("%d ", idx);
                    //printf("\n");
                    req.curData = getter(req.idxs);
                    req.tLastUpdated_ms = tCur;

                    socketData[clientId]->ws->send({ (char *)(&requestId), sizeof(requestId) }, uWS::OpCode::BINARY);
                    socketData[clientId]->ws->send(req.curData, uWS::OpCode::BINARY);
                }
            }
        }
    }

    int port = 3000;

    std::map<TPath, int> pathToGetter;
    std::vector<TGetter> getters;

    uWS::Loop * mainLoop = nullptr;
    std::map<int, PerSocketData *> socketData;
    std::map<int, ClientData> clientData;
};

InCppect::InCppect() : m_impl(new Impl()) {}

InCppect::~InCppect() {}

bool InCppect::init(int port) {
    m_impl->port = port;

    return m_impl->init();
}

void InCppect::run() {
    var("nClients", [this](const TIdxs & idxs) {
        static int n = 0;
        n = m_impl->socketData.size();
        return std::string_view((char *)&n, sizeof(n));
    });

    m_impl->run();
}

std::thread InCppect::runAsync() {
    std::thread worker([this](){ this->run(); });
    return worker;
}

bool InCppect::var(const TPath & path, TGetter && getter) {
    m_impl->pathToGetter[path] = m_impl->getters.size();
    m_impl->getters.emplace_back(std::move(getter));

    return true;
}
