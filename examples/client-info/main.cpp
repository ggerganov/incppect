/*! \file main.cpp
 *  \brief incppect basics
 *  \author Georgi Gerganov
 */

#include "incppect/incppect.h"

int main(int argc, char ** argv) {
	printf("Usage: %s [port] [httpRoot]\n", argv[0]);

    int port = argc > 1 ? atoi(argv[1]) : 3000;
    std::string httpRoot = argc > 2 ? argv[2] : "../examples/static";

    Incppect::getInstance().runAsync(Incppect::Parameters {
        .portListen = port,
        .maxPayloadLength_bytes = 256*1024,
        .httpRoot = httpRoot + "/client-info",
    }).detach();

    int8_t  var_int8  = 1;
    int16_t var_int16 = 2;
    int32_t var_int32 = 3;
    int32_t var_arr32[4] = { 4, 5, 6, 7 };
    float   var_float = 8.0f;

    const char * var_str = "hello browser";

    Incppect::getInstance().var("var_int8", [&](auto) { return Incppect::view(var_int8); });
    Incppect::getInstance().var("var_int16", [&](auto) { return Incppect::view(var_int16); });
    Incppect::getInstance().var("var_int32", [&](auto) { return Incppect::view(var_int32); });
    Incppect::getInstance().var("var_int32_arr", [&](auto) { return Incppect::view(var_arr32); });
    Incppect::getInstance().var("var_int32_arr[%d]", [&](auto idxs) { return Incppect::view(var_arr32[idxs[0]]); });
    Incppect::getInstance().var("var_float", [&](auto idxs) { return Incppect::view(var_float); });
    Incppect::getInstance().var("var_str", [&](auto idxs) { return var_str; });

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
