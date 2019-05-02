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
#include <algorithm>

namespace {
    inline int64_t timestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
}

struct InCppect::Impl {
    struct Request {
        int64_t tLastUpdated_ms = -1;
        int64_t tLastRequested_ms = -1;
        int64_t tMinUpdate_ms = 16;
        int64_t tLastRequestTimeout_ms = 3000;

        TIdxs idxs;
        int getterId = -1;

        std::vector<char> prevData;
        std::string_view curData;
    };

    struct ClientData {
        int64_t tConnected_ms = -1;

        uint8_t ipAddress[4];

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

        uWS::App().ws<PerSocketData>("/incppect", {
            .compression = uWS::SHARED_COMPRESSOR,
                .maxPayloadLength = 256*1024,
                .open = [&](auto *ws, auto *req) {
                    std::cout << "XXXXXX: " << ws->getRemoteAddress().size() << std::endl;
                    for (int i = 0; i < ws->getRemoteAddress().size(); ++i) {
                        printf("%d - %d\n", i, ws->getRemoteAddress()[i]);
                    }

                    static int uniqueId = 1;
                    ++uniqueId;

                    auto & cd = clientData[uniqueId];
                    cd.tConnected_ms = ::timestamp();

                    auto addressBytes = ws->getRemoteAddress();
                    cd.ipAddress[0] = addressBytes[12];
                    cd.ipAddress[1] = addressBytes[13];
                    cd.ipAddress[2] = addressBytes[14];
                    cd.ipAddress[3] = addressBytes[15];

                    auto sd = (PerSocketData *) ws->getUserData();
                    sd->clientId = uniqueId;
                    sd->ws = ws;
                    sd->mainLoop = uWS::Loop::get();

                    socketData.insert({ uniqueId, sd });

                    std::cout << "[+] Client with Id = " << sd->clientId  << " connected\n";
                },
                .message = [this](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    rxTotal_bytes += message.size();
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
            std::vector<char> buffer;
            for (auto & [requestId, req] : cd.requests) {
                auto & getter = getters[req.getterId];
                auto tCur = ::timestamp();
                if (tCur - req.tLastRequested_ms < req.tLastRequestTimeout_ms &&
                    tCur - req.tLastUpdated_ms > req.tMinUpdate_ms) {
                    req.curData = getter(req.idxs);
                    req.tLastUpdated_ms = tCur;

                    const int kPadding = 4;

                    int dataSize_bytes = req.curData.size();
                    int padding_bytes = 0;
                    {
                        int r = dataSize_bytes%kPadding;
                        while (r > 0 && r < kPadding) {
                            ++dataSize_bytes;
                            ++padding_bytes;
                            ++r;
                        }
                    }

                    std::copy((char *)(&requestId), (char *)(&requestId) + sizeof(requestId), std::back_inserter(buffer));
                    std::copy((char *)(&dataSize_bytes), (char *)(&dataSize_bytes) + sizeof(dataSize_bytes), std::back_inserter(buffer));
                    std::copy(req.curData.begin(), req.curData.end(), std::back_inserter(buffer));
                    {
                        char v = 0;
                        for (int i = 0; i < padding_bytes; ++i) {
                            std::copy((char *)(&v), (char *)(&v) + sizeof(v), std::back_inserter(buffer));
                        }
                    }
                }
            }
            if (socketData[clientId]->ws->getBufferedAmount()) {
                std::cout << "  [update] Buffered amount = " << socketData[clientId]->ws->getBufferedAmount() << std::endl;
            }
            if (buffer.size() > 0) {
                socketData[clientId]->ws->send({ buffer.data(), buffer.size() }, uWS::OpCode::BINARY);
                txTotal_bytes += buffer.size();
            }
        }
    }

    int port = 3000;

    int32_t txTotal_bytes = 0;
    int32_t rxTotal_bytes = 0;

    std::map<TPath, int> pathToGetter;
    std::vector<TGetter> getters;

    uWS::Loop * mainLoop = nullptr;
    std::map<int, PerSocketData *> socketData;
    std::map<int, ClientData> clientData;
};

InCppect::InCppect() : m_impl(new Impl()) {
    var("incppect.nclients", [this](const TIdxs & idxs) {
        static int n = 0;
        n = m_impl->socketData.size();
        return std::string_view((char *)&n, sizeof(n));
    });

    var("incppect.tx_total", [this](const TIdxs & idxs) {
        return std::string_view((char *)&m_impl->txTotal_bytes, sizeof(m_impl->txTotal_bytes));
    });

    var("incppect.rx_total", [this](const TIdxs & idxs) {
        return std::string_view((char *)&m_impl->rxTotal_bytes, sizeof(m_impl->rxTotal_bytes));
    });

    var("incppect.ip_address[%d]", [this](const TIdxs & idxs) {
        auto it = m_impl->clientData.cbegin();
        std::advance(it, idxs[0]);
        return std::string_view((char *)it->second.ipAddress, sizeof(it->second.ipAddress));
    });
}

InCppect::~InCppect() {}

bool InCppect::init(int port) {
    m_impl->port = port;

    return m_impl->init();
}

void InCppect::run() {
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
