/*! \file incppect.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "incppect/incppect.h"

#include "common.h"

#include "App.h" // uWebSockets

#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#ifdef INCPPECT_DEBUG
#define my_printf printf
#else
#define my_printf(...)
#endif

namespace {
    inline int64_t timestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
}

struct Incppect::Impl {
    using IpAddress = uint8_t[4];

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

        IpAddress ipAddress;

        std::vector<int> lastRequests;
        std::map<int, Request> requests;
    };

    struct PerSocketData {
        int clientId = 0;

        uWS::Loop * mainLoop = nullptr;
        uWS::WebSocket<false, true> * ws = nullptr;
    };

    void run() {
        mainLoop = uWS::Loop::get();

        uWS::App().ws<PerSocketData>("/incppect", uWS::TemplatedApp<false>::WebSocketBehavior {
            .compression = uWS::SHARED_COMPRESSOR,
            .maxPayloadLength = parameters.maxPayloadLength_bytes,
            .idleTimeout = 120,
            .open = [&](auto *ws, auto *req) {
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

                my_printf("[incppect] client with id = %d connectd\n", sd->clientId);
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
                                    my_printf("[incppect] requestId = %d, path = '%s', nidxs = %d\n", requestId, path.c_str(), nidxs);
                                    request.getterId = pathToGetter[path];

                                    cd.requests[requestId] = std::move(request);
                                } else {
                                    my_printf("[incppect] missing path '%s'\n", path.c_str());
                                }
                            }
                        }
                        break;
                    case 2:
                        {
                            int nRequests = (message.size() - sizeof(int))/sizeof(int);
                            my_printf("[incppect] received requests: %d\n", nRequests);

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
                        my_printf("[incppect] unknown message type: %d\n", type);
                };
                sd->mainLoop->defer([this]() { this->update(); });
            },
            .drain = [](auto *ws) {
                /* Check getBufferedAmount here */
                if (ws->getBufferedAmount() > 0) {
                    my_printf("[incppect] drain: buffered amount = %d\n", ws->getBufferedAmount());
                }
            },
            .ping = [](auto *ws) {

            },
            .pong = [](auto *ws) {

            },
            .close = [&](auto *ws, int code, std::string_view message) {
                auto sd = (PerSocketData *) ws->getUserData();
                my_printf("[incppect] client with id = %d disconnected\n", sd->clientId);

                clientData.erase(sd->clientId);
                socketData.erase(sd->clientId);
            }
        }).get("/incppect.js", [this](auto *res, auto *req) {
            res->end(kIncppect_js);
        }).get("/*", [this](auto *res, auto *req) {
            std::string url = std::string(req->getUrl());

            if (url.size() == 0) {
                res->end("Resource not found");
                return;
            }

            if (url[url.size() - 1] == '/') {
                url += "index.html";
            }

            std::ifstream fin(parameters.httpRoot + url);

            if (fin.is_open() == false || fin.good() == false) {
                res->end("Resource not found");
                return;
            }

            std::string str((std::istreambuf_iterator<char>(fin)),
                            std::istreambuf_iterator<char>());

            if (str.size() == 0) {
                res->end("Resource not found");
                return;
            }

            res->end(str);
        }).listen(parameters.portListen, [this](auto *token) {
            if (token) {
                my_printf("[incppect] listening on port %d\n", parameters.portListen);
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
                my_printf("[incppect] update: buffered amount = %d\n", socketData[clientId]->ws->getBufferedAmount());
            }
            if (buffer.size() > 0) {
                socketData[clientId]->ws->send({ buffer.data(), buffer.size() }, uWS::OpCode::BINARY);
                txTotal_bytes += buffer.size();
            }
        }
    }

    Parameters parameters;

    int32_t txTotal_bytes = 0;
    int32_t rxTotal_bytes = 0;

    std::map<TPath, int> pathToGetter;
    std::vector<TGetter> getters;

    uWS::Loop * mainLoop = nullptr;
    std::map<int, PerSocketData *> socketData;
    std::map<int, ClientData> clientData;
};

Incppect::Incppect() : m_impl(new Impl()) {
    var("incppect.nclients", [this](const TIdxs & idxs) {
        static int n = 0;
        n = m_impl->socketData.size();
        return view(n);
    });

    var("incppect.tx_total", [this](const TIdxs & idxs) { return view(m_impl->txTotal_bytes); });
    var("incppect.rx_total", [this](const TIdxs & idxs) { return view(m_impl->rxTotal_bytes); });

    var("incppect.ip_address[%d]", [this](const TIdxs & idxs) {
        auto it = m_impl->clientData.cbegin();
        std::advance(it, idxs[0]);
        return view(it->second.ipAddress);
    });
}

Incppect::~Incppect() {}

void Incppect::run(Parameters parameters) {
    m_impl->parameters = parameters;
    m_impl->run();
}

std::thread Incppect::runAsync(Parameters parameters) {
    std::thread worker([this, parameters]() { this->run(parameters); });
    return worker;
}

bool Incppect::var(const TPath & path, TGetter && getter) {
    m_impl->pathToGetter[path] = m_impl->getters.size();
    m_impl->getters.emplace_back(std::move(getter));

    return true;
}
