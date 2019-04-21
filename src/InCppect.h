/*! \file InCppect.h
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#pragma once

#include <memory>
#include <functional>
#include <string_view>

class InCppect {
    public:
        using TPath = std::string;
        using TIdxs = std::vector<int>;
        using TGetter = std::function<std::string_view(const TIdxs & idxs)>;

        InCppect();
        ~InCppect();

        bool init(int port);
        void run();

        bool var(const TPath & path, TGetter && getter);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
};

