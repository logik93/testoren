#pragma once
#include "Modulation.h"
namespace logisth {
class Envelope {
    enum Stage { off, attack, decay, sustain, release } stage = off;
    float value = 0, releaseStep = 0;

  public:
    void reset() {
        stage = off;
        value = 0;
    }
    void on() {
        stage = attack;
        value = 0;
    }
    void releaseNote(float seconds, double sr) {
        releaseStep = value / float(std::max(1., seconds * sr));
        stage = release;
    }
    bool active() const {
        return stage != off;
    }
    float tick(const EnvelopeParams &p, double sr) {
        // Zero-duration stages consume no sample. In particular, A=D=S=0 must
        // enter silent sustain directly, never emit a one-sample peak first.
        for (;;) {
            switch (stage) {
            case off:
                return 0;
            case attack:
                if (p.attack <= 0) {
                    value = 1;
                    stage = decay;
                    continue;
                }
                value += float(1 / std::max(1., p.attack * sr));
                if (value >= 1) {
                    value = 1;
                    stage = decay;
                }
                return value;
            case decay:
                if (p.decay <= 0) {
                    value = p.sustain;
                    stage = sustain;
                    continue;
                }
                value -= float((1 - p.sustain) / std::max(1., p.decay * sr));
                if (value <= p.sustain) {
                    value = p.sustain;
                    stage = sustain;
                }
                return value;
            case sustain:
                return value = p.sustain;
            case release:
                value -= releaseStep;
                if (value <= 0)
                    reset();
                return value;
            }
        }
    }
};

// Amplitude stages get a short, causal finite smoothing window. Control/modulation
// envelopes above retain their exact stage values. Two half-ms moving averages
// form a positive triangular kernel: unity DC gain, no overshoot or ringing,
// constant work per sample, and exact silence at the end of the bounded tail.
class AudioEnvelope {
    static constexpr int capacity = 512; // 1 ms total through 1.024 MHz host rates.
    Envelope logical;
    std::array<float, capacity> first{}, second{};
    double sum1 = 0, sum2 = 0;
    float delayed = 0;
    int window = 0, cursor = 0, pending = 0;
    void clearFilter() {
        std::fill_n(first.begin(), window, 0.f);
        std::fill_n(second.begin(), window, 0.f);
        sum1 = sum2 = 0;
        delayed = 0;
        cursor = pending = 0;
    }

  public:
    static constexpr double edgeSeconds = .001;
    void reset() {
        logical.reset();
        clearFilter();
    }
    void on() {
        clearFilter();
        logical.on();
    }
    void releaseNote(float seconds, double sr) {
        logical.releaseNote(seconds, sr);
    }
    bool active() const {
        return logical.active() || pending > 0;
    }
    float tick(const EnvelopeParams &p, double sr) {
        if (window == 0)
            window = std::clamp(int(std::lround(sr * edgeSeconds * .5)), 1, capacity);
        float target = logical.tick(p, sr);
        if (target != 0)
            pending = window * 2;
        else if (pending == 0)
            return 0;
        else if (--pending == 0) {
            clearFilter();
            return 0;
        }
        // The single-sample delay anchors a new note's first sample at zero and
        // preserves the previous amplitude at the exact note-off sample.
        sum1 += double(delayed) - first[size_t(cursor)];
        first[size_t(cursor)] = delayed;
        delayed = target;
        float average = float(sum1 / window);
        sum2 += double(average) - second[size_t(cursor)];
        second[size_t(cursor)] = average;
        if (++cursor == window)
            cursor = 0;
        return clamp(float(sum2 / window), 0, 1);
    }
};
} // namespace logisth
