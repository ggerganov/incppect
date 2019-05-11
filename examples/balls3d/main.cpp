/*! \file main.cpp
 *  \brief Elastic 3d collisions
 *  \author Georgi Gerganov
 */

#include "incppect/incppect.h"

#include <cmath>
#include <vector>
#include <cstdlib>

inline float frand() { return (float)(rand())/RAND_MAX; }

struct Ball {
    float r = 0.0f;
    float m = 0.0f;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
};

float dist2(const float r0, const float r1) {
    return (r0 + r1)*(r0 + r1);
}

float dist2(const Ball & c0, const Ball & c1) {
    return (c0.x - c1.x)*(c0.x - c1.x) + (c0.y - c1.y)*(c0.y - c1.y) + (c0.z - c1.z)*(c0.z - c1.z);
}

struct State {
    State() {
        Incppect::getInstance().var("state.dt", [this](const auto & idxs) { return Incppect::view(dt); });

        Incppect::getInstance().var("state.nballs", [this](const auto & idxs) {
            static int n = 0;
            n = balls.size();
            return Incppect::view(n);
        });

        Incppect::getInstance().var("state.dt", [this](const auto & idxs) { return Incppect::view(dt); });
        Incppect::getInstance().var("state.energy", [this](const auto & idxs) { return Incppect::view(energy); });

        Incppect::getInstance().var("state.ball[%d].r", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].r); });
        Incppect::getInstance().var("state.ball[%d].m", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].m); });
        Incppect::getInstance().var("state.ball[%d].x", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].x); });
        Incppect::getInstance().var("state.ball[%d].y", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].y); });
        Incppect::getInstance().var("state.ball[%d].z", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].z); });
        Incppect::getInstance().var("state.ball[%d].vx", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].vx); });
        Incppect::getInstance().var("state.ball[%d].vy", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].vy); });
        Incppect::getInstance().var("state.ball[%d].vz", [this](const auto & idxs) { return Incppect::view(balls[idxs[0]].vz); });
    }

    void init(int nBalls) {
        balls.resize(nBalls);
        for (int i = 0; i < nBalls; ++i) {
            auto & ball = balls[i];
            ball.r = 0.05f*frand() + 0.02f;
            ball.m = ball.r*ball.r*ball.r;

            ball.x = 2.0f*frand() - 1.0f;
            ball.y = 2.0f*frand() - 1.0f;
            ball.z = 2.0f*frand() - 1.0f;

            ball.vx = 2.0f*frand() - 1.0f;
            ball.vy = 2.0f*frand() - 1.0f;
            ball.vz = 2.0f*frand() - 1.0f;
        }
    }

    void update() {
        float energy = 0.0;

        for (auto & ball : balls) {
            ball.x += ball.vx*dt;
            ball.y += ball.vy*dt;
            ball.z += ball.vz*dt;

            if (ball.x - ball.r < -1.0f) ball.vx =  std::abs(ball.vx);
            if (ball.y - ball.r < -1.0f) ball.vy =  std::abs(ball.vy);
            if (ball.z - ball.r < -1.0f) ball.vz =  std::abs(ball.vz);
            if (ball.x + ball.r >  1.0f) ball.vx = -std::abs(ball.vx);
            if (ball.y + ball.r >  1.0f) ball.vy = -std::abs(ball.vy);
            if (ball.z + ball.r >  1.0f) ball.vz = -std::abs(ball.vz);

            if (ball.x < -2.0f || ball.y < -2.0f || ball.z < 2.0f || ball.x > 2.0f || ball.y > 2.0f || ball.z > 2.0f) {
                ball.x = std::max(ball.x, -1.0f);
                ball.x = std::min(ball.x,  1.0f);
                ball.y = std::max(ball.y, -1.0f);
                ball.y = std::min(ball.y,  1.0f);
                ball.z = std::max(ball.z, -1.0f);
                ball.z = std::min(ball.z,  1.0f);
            }

            energy += ball.m*(ball.vx*ball.vx + ball.vy*ball.vy + ball.vz*ball.vz);
        }

        this->energy = energy;

        for (int i = 0; i < balls.size(); ++i) {
            auto & c0 = balls[i];
            for (int j = i + 1; j < balls.size(); ++j) {
                auto & c1 = balls[j];

                float d2 = ::dist2(c0, c1);
                if (d2 > ::dist2(c0.r, c1.r)) continue;

                float nx = c0.x - c1.x;
                float ny = c0.y - c1.y;
                float nz = c0.z - c1.z;
                float d = sqrt(d2);
                nx /= d;
                ny /= d;
                nz /= d;

                float dvx = c0.vx - c1.vx;
                float dvy = c0.vy - c1.vy;
                float dvz = c0.vz - c1.vz;
                float norm = 1.0f/(c0.m + c1.m);

                float p = 2.0f*norm*(dvx*nx + dvy*ny + dvz*nz);
                float vx0 = c0.vx - c1.m*p*nx;
                float vy0 = c0.vy - c1.m*p*ny;
                float vz0 = c0.vz - c1.m*p*nz;
                float vx1 = c1.vx + c0.m*p*nx;
                float vy1 = c1.vy + c0.m*p*ny;
                float vz1 = c1.vz + c0.m*p*nz;

                c0.vx = vx0;
                c0.vy = vy0;
                c0.vz = vz0;
                c1.vx = vx1;
                c1.vy = vy1;
                c1.vz = vz1;

                float l = 1.01f*(c0.r + c1.r) - d;

                float nv0 = c0.vx*nx + c0.vy*ny + c0.vz*nz;
                float nv1 = c1.vx*nx + c1.vy*ny + c1.vz*nz;
                if (std::abs(nv0 - nv1) > 1e-5) {
                    float dt = l/(nv0 - nv1);

                    c0.x += c0.vx*dt;
                    c0.y += c0.vy*dt;
                    c0.z += c0.vz*dt;
                    c1.x += c1.vx*dt;
                    c1.y += c1.vy*dt;
                    c1.z += c1.vz*dt;
                }
            }
        }

        this->t += dt;
    }

    float t = 0.000f;
    float dt = 0.001f;
    float energy = 0.0f;

    std::vector<Ball> balls;
};

int main(int argc, char ** argv) {
	printf("Usage: %s [port] [httpRoot] [nBalls]\n", argv[0]);

    int port = argc > 1 ? atoi(argv[1]) : 3000;
    std::string httpRoot = argc > 2 ? argv[2] : "../examples/static";
    int nBalls = argc > 3 ? atoi(argv[3]) : 64;

    nBalls = std::max(1, std::min(128, nBalls));

    State state;
    state.init(nBalls);

    Incppect::getInstance().runAsync(Incppect::Parameters {
        .portListen = port,
        .maxPayloadLength_bytes = 256*1024,
        .httpRoot = httpRoot + "/balls3d",
    }).detach();

    float checkpoint = 0.0f;
    while (true) {
        if (state.t > checkpoint) {
            printf(" - Simulating 3D elastic collisions at t = %4.2f ...\n", state.t);
            checkpoint += 1.0f;
        }

        state.update();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
