/*! \file main.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "Common.h"
#include "InCppect.h"

#include <thread>

// root.
//      name = "some string"
//      nrooms = 10
//      room[0].
//              nparticles = 10
//              buffer (size = 1024)
//              particle[0].
//                          x
//                          y
//                          z
//              particle[1].
//                          x
//                          y
//                          z
//              ...
//              particle[9].
//                          x
//                          y
//                          z
//      ...
//      room[9].
//              nparticles = 10
//              particle[0].
//                          x
//                          y
//                          z
//              particle[1].
//                          x
//                          y
//                          z
//              ...
//              particle[9].
//                          x
//                          y
//                          z
//
// using TBuffer = std::pair<const char *, size_t>;
//
// incppect_define("root.name",                     [&]()->TBuffer { return { someStr.data(), someStr.size() } );
// incppect_define("root.nrooms",                   [&]()->TBuffer { return { &nrooms, sizeof(nrooms) } );
// incppect_define("root.nroom[%d].nparticles",     [&](int i0)->TBuffer { return { &room[i0].nparticles, sizeof(room[i0].nparticles) } );
// incppect_define("root.nroom[%d].particle[%d].x", [&](int i0, int i1)->TBuffer { return { &room[i0].particle[i1].x, sizeof(room[i0].particles[i1].x) } );
// incppect_define("root.nroom[%d].buffer",         [&](int i0)->TBuffer { return { &room[i0].buffer.data(), sizeof(room[i0].buffer[0])*room[i0].buffer.size() } );
//
//
//
// incppect("root", &someData);
//
//
//

int main() {
    InCppect inCppect;
    inCppect.init(3000);

    std::thread worker([&](){ inCppect.run(); });

    while (true) {
        int a = rand()%10000;
        //for (auto & [_, wsData] : wsClients) {
        //    if (wsData->mainLoop && wsData->ws) {
        //        wsData->mainLoop->defer([&, ws = wsData->ws]() {
        //            ws->send("Connected clients: " + std::to_string(wsClients.size()), uWS::OpCode::TEXT);
        //            ws->send("Rand " + std::to_string(a), uWS::OpCode::TEXT);
        //        });
        //    }
        //}

        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    worker.join();

    return 0;
}
