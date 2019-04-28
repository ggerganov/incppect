/*! \file main.cpp
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#include "InCppect.h"

#include <cmath>
#include <vector>
#include <cstdlib>

inline float frand() { return (float)(rand())/RAND_MAX; }

struct Circle {
    float r = 0.0f;
    float m = 0.0f;

    float x = 0.0f;
    float y = 0.0f;

    float vx = 0.0f;
    float vy = 0.0f;
};

float dist2(const float r0, const float r1) {
    return (r0 + r1)*(r0 + r1);
}

float dist2(const Circle & c0, const Circle & c1) {
    return (c0.x - c1.x)*(c0.x - c1.x) + (c0.y - c1.y)*(c0.y - c1.y);
}

struct State {
    State() {
        InCppect::getInstance().var("state.dt", [this](const auto & idxs) { return InCppect::View(dt); });

        InCppect::getInstance().var("state.ncircles", [this](const auto & idxs) {
            static int n = 0;
            n = circles.size();
            return InCppect::View(n);
        });

        InCppect::getInstance().var("state.dt", [this](const auto & idxs) { return InCppect::View(dt); });
        InCppect::getInstance().var("state.energy", [this](const auto & idxs) { return InCppect::View(energy); });

        InCppect::getInstance().var("state.circle[%d].r", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].r); });
        InCppect::getInstance().var("state.circle[%d].m", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].m); });
        InCppect::getInstance().var("state.circle[%d].x", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].x); });
        InCppect::getInstance().var("state.circle[%d].y", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].y); });
        InCppect::getInstance().var("state.circle[%d].vx", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].vx); });
        InCppect::getInstance().var("state.circle[%d].vy", [this](const auto & idxs) { return InCppect::View(circles[idxs[0]].vy); });
    }

    void init(int nCircles) {
        circles.resize(nCircles);
        for (int i = 0; i < nCircles; ++i) {
            auto & circle = circles[i];
            circle.r = 0.05f*frand() + 0.02f;
            circle.m = circle.r*circle.r;

            circle.x = 2.0f*frand() - 1.0f;
            circle.y = 2.0f*frand() - 1.0f;

            circle.vx = 2.0f*frand() - 1.0f;
            circle.vy = 2.0f*frand() - 1.0f;
        }
    }

    float dt = 0.001f;
    float energy = 0.0f;

    std::vector<Circle> circles;
};

int main(int argc, char ** argv) {
	printf("Usage: %s [nCircles]\n", argv[0]);
    int nCircles = argc > 1 ? atoi(argv[1]) : 64;

    State state;
    state.init(nCircles);

    InCppect::getInstance().init(3000);
    InCppect::getInstance().runAsync().detach();

    while (true) {
        float energy = 0.0;

        for (auto & circle : state.circles) {
            circle.x += circle.vx*state.dt;
            circle.y += circle.vy*state.dt;

            if (circle.x - circle.r < -1.0f) circle.vx =  std::abs(circle.vx);
            if (circle.y - circle.r < -1.0f) circle.vy =  std::abs(circle.vy);
            if (circle.x + circle.r >  1.0f) circle.vx = -std::abs(circle.vx);
            if (circle.y + circle.r >  1.0f) circle.vy = -std::abs(circle.vy);

            if (circle.x < -2.0f || circle.y < -2.0f || circle.x >  2.0f || circle.y >  2.0f) {
                circle.x = std::max(circle.x, -1.0f);
                circle.x = std::min(circle.x,  1.0f);
                circle.y = std::max(circle.y, -1.0f);
                circle.y = std::min(circle.y,  1.0f);
            }

            energy += circle.m*(circle.vx*circle.vx + circle.vy*circle.vy);
        }

        state.energy = energy;

        for (int i = 0; i < state.circles.size(); ++i) {
            auto & c0 = state.circles[i];
            for (int j = i + 1; j < state.circles.size(); ++j) {
                auto & c1 = state.circles[j];

                float d2 = ::dist2(c0, c1);
                if (d2 > ::dist2(c0.r, c1.r)) continue;

                float nx = c0.x - c1.x;
                float ny = c0.y - c1.y;
                float d = sqrt(d2);
                nx /= d;
                ny /= d;

                float dvx = c0.vx - c1.vx;
                float dvy = c0.vy - c1.vy;
                float norm = 1.0f/(c0.m + c1.m);

                float p = 2.0f*norm*(dvx*nx + dvy*ny);
                float vx0 = c0.vx - c1.m*p*nx;
                float vy0 = c0.vy - c1.m*p*ny;
                float vx1 = c1.vx + c0.m*p*nx;
                float vy1 = c1.vy + c0.m*p*ny;

                c0.vx = vx0;
                c0.vy = vy0;
                c1.vx = vx1;
                c1.vy = vy1;

                float l = 1.01f*(c0.r + c1.r) - d;

                float nv0 = c0.vx*nx + c0.vy*ny;
                float nv1 = c1.vx*nx + c1.vy*ny;
                if (std::abs(nv0 - nv1) > 1e-5) {
                    float dt = l/(nv0 - nv1);

                    c0.x += c0.vx*dt;
                    c0.y += c0.vy*dt;
                    c1.x += c1.vx*dt;
                    c1.y += c1.vy*dt;
                }
            }
        }

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
