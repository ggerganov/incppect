/*! \file incppect.h
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

class Incppect {
    public:
        using TPath = std::string;
        using TIdxs = std::vector<int>;
        using TGetter = std::function<std::string_view(const TIdxs & idxs)>;

        // service parameters
        struct Parameters {
            int32_t portListen = 3000;
            int32_t maxPayloadLength_bytes = 256*1024;

            std::string httpRoot = ".";

            // todo:
            // max clients
            // max buffered amount
            // etc.
        };

        Incppect();
        ~Incppect();

        // run the incppect service main loop in the current thread
        // blocking call
        void run(Parameters parameters);

        // run the incppect service main loop in dedicated thread
        // non-blocking call, returns the created std::thread
        std::thread runAsync(Parameters parameters);

        // define variable/memory to inspect
        //
        // examples:
        //
        //   var("path0", [](auto ) { ... });
        //   var("path1[%d]", [](auto idxs) { ... idxs[0] ... });
        //   var("path2[%d].foo[%d]", [](auto idxs) { ... idxs[0], idxs[1] ... });
        //
        bool var(const TPath & path, TGetter && getter);

        // shorthand for string_view from var
        template<typename T>
            static std::string_view view(const T & v) {
                return std::string_view { (char *)(&v), sizeof(v) };
            }

        // get global instance
        static Incppect & getInstance() {
            static Incppect instance;
            return instance;
        }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
};

