#pragma once
#include "Modulation.h"
namespace logisth {
struct EnvelopePlotPoint {
    float x, level;
};
inline std::array<EnvelopePlotPoint, 5> envelopePlot(const EnvelopeParams &p) {
    const float attack = std::max(0.f, p.attack), decay = std::max(0.f, p.decay),
                release = std::max(0.f, p.release), heldSeconds = .5f;
    const float seconds = attack + decay + heldSeconds + release;
    return {{{0, 0},
             {attack / seconds, 1},
             {(attack + decay) / seconds, clamp(p.sustain, 0, 1)},
             {(attack + decay + heldSeconds) / seconds, clamp(p.sustain, 0, 1)},
             {1, 0}}};
}
} // namespace logisth
