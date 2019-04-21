/*! \file InCppect.h
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#pragma once

#include <memory>
#include <functional>

class InCppect {
    public:
        InCppect();
        ~InCppect();

        bool init(int port);
        void run();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
};

