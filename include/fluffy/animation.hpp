// include/fluffy/animation.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// SmoothAnim — exponentially-decaying animation channel (hover / press).

#pragma once
#include <cmath>
namespace fluffy {
struct SmoothAnim {
    float current = 0.0f;
    float target_ = 0.0f;
    void  setTarget(float t) noexcept { target_ = t; }
    float target()    const noexcept { return target_; }
    float value()     const noexcept { return current; }
    void  tick(float dt, float tau) noexcept {
        if (dt <= 0.0f || tau <= 0.0f) return;
        const float alpha = 1.0f - std::exp(-dt / tau);
        current += (target_ - current) * alpha;
    }
    void  snapTo(float v) noexcept { current = target_ = v; }
};
}  

