/*! \file InCppect.h
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#pragma once

#include <memory>
#include <functional>
#include <string>
#include <string_view>
#include <thread>

class InCppect {
    public:
        using TPath = std::string;
        using TIdxs = std::vector<int>;
        using TGetter = std::function<std::string_view(const TIdxs & idxs)>;

        InCppect();
        ~InCppect();

        bool init(int port);
        void run();
        std::thread runAsync();

        bool var(const TPath & path, TGetter && getter);

        template<typename T>
            static std::string_view View(const T & v) {
                return std::string_view { (char *)(&v), sizeof(v) };
            }

        static InCppect & getInstance() {
            static InCppect instance;
            return instance;
        }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
};

