#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
namespace logisth {
// Single audio writer, GUI readers. Atomic cells also make ring wrap safe in C++.
class ScopeBuffer {
    static constexpr int capacity = 8192;
    using Cells = std::array<std::array<std::atomic<float>, capacity>, 3>;
    std::unique_ptr<Cells> cells = std::make_unique<Cells>();
    uint64_t writer = 0;
    std::atomic<uint64_t> published{0};

  public:
    void push(float a, float b, float sum) {
        size_t i = size_t(writer++ % capacity);
        (*cells)[0][i].store(a, std::memory_order_relaxed);
        (*cells)[1][i].store(b, std::memory_order_relaxed);
        (*cells)[2][i].store(sum, std::memory_order_relaxed);
    }
    void commit() {
        published.store(writer, std::memory_order_release);
    }
    void clear() {
        writer = 0;
        published.store(0, std::memory_order_release);
    }
    uint64_t count() const {
        return published.load(std::memory_order_acquire);
    }
    void read(int source, float *output, int n) const {
        uint64_t end = published.load(std::memory_order_acquire),
                 available = std::min<uint64_t>(end, capacity);
        for (int i = 0; i < n; ++i) {
            int64_t at = int64_t(end) - n + i;
            output[i] =
                at < 0 || uint64_t(int64_t(end) - at) > available
                    ? 0
                    : (*cells)[size_t(std::clamp(source, 0, 2))][size_t(at % capacity)].load(
                          std::memory_order_relaxed);
        }
    }
};
struct PlaybackPicture {
    float position = 0;
    bool active = false;
    uint64_t revision = 0;
    std::array<float, 16> grains{};
    int grainCount = 0;
};
struct PlaybackTelemetry {
    std::atomic<float> position{0};
    std::atomic<bool> active{false};
    std::atomic<uint64_t> revision{0};
    std::array<std::atomic<float>, 16> grains{};
    std::atomic<int> grainCount{0};
    void write(const PlaybackPicture &p) {
        position.store(p.position, std::memory_order_relaxed);
        for (int i = 0; i < 16; ++i)
            grains[size_t(i)].store(p.grains[size_t(i)], std::memory_order_relaxed);
        grainCount.store(p.grainCount, std::memory_order_relaxed);
        revision.store(p.revision, std::memory_order_relaxed);
        active.store(p.active, std::memory_order_release);
    }
    PlaybackPicture read() const {
        PlaybackPicture p;
        p.active = active.load(std::memory_order_acquire);
        p.position = position.load(std::memory_order_relaxed);
        p.revision = revision.load(std::memory_order_relaxed);
        p.grainCount = grainCount.load(std::memory_order_relaxed);
        for (int i = 0; i < 16; ++i)
            p.grains[size_t(i)] = grains[size_t(i)].load(std::memory_order_relaxed);
        return p;
    }
};
} // namespace logisth
