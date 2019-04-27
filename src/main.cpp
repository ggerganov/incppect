/*! \file main.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "InCppect.h"

#include <vector>
#include <cstdlib>

inline float frand() { return (float)(rand())/RAND_MAX; }

struct Circle {
    float r = 0.0f;

    float x = 0.0f;
    float y = 0.0f;

    float vx = 0.0f;
    float vy = 0.0f;
};

struct State {
    State() {
        InCppect::getInstance().var("state.dt", [this](const auto & idxs) { return InCppect::View(dt); });

        InCppect::getInstance().var("state.nCircles", [this](const auto & idxs) {
            static int n = 0;
            n = circles.size();
            return InCppect::View(n);
        });

        InCppect::getInstance().var("state.circle[%d].r", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].r); });
        InCppect::getInstance().var("state.circle[%d].x", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].x); });
        InCppect::getInstance().var("state.circle[%d].y", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].y); });
        InCppect::getInstance().var("state.circle[%d].vx", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].vx); });
        InCppect::getInstance().var("state.circle[%d].vy", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].vy); });
    }

    void init(int nCircles) {
        circles.resize(nCircles);
        for (auto & circle : circles) {
            circle.r = 0.01f*frand() + 0.01f;

            circle.x = 2.0f*frand() - 1.0f;
            circle.y = 2.0f*frand() - 1.0f;

            circle.vx = 2.0f*frand() - 1.0f;
            circle.vy = 2.0f*frand() - 1.0f;
        }
    }

    float dt = 0.001f;

    std::vector<Circle> circles;
};

int main(int argc, char ** argv) {
	printf("Usage: %s [nCircles]\n", argv[0]);
    int nCircles = argc > 0 ? atoi(argv[1]) : 256;

    State state;
    state.init(nCircles);

    InCppect::getInstance().init(3000);
    InCppect::getInstance().runAsync().detach();

    while (true) {
        for (auto & circle : state.circles) {
            circle.x += circle.vx*state.dt;
            circle.y += circle.vy*state.dt;

            if (circle.x < -1.0f || circle.x > 1.0f) circle.vx = -circle.vx;
            if (circle.y < -1.0f || circle.y > 1.0f) circle.vy = -circle.vy;
        }

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
