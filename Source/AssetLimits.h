#pragma once
#include <cstdint>
namespace logisth {
// Decoded stereo PCM across the four imported sources; factory data is shared.
// Duration is unrestricted. Memory and host-state sizes are explicitly bounded.
inline constexpr uint64_t sampleMemoryBudget = 512ull * 1024 * 1024;
inline constexpr int stateMemoryLimit = 1024 * 1024 * 1024;
inline constexpr uint64_t sourceBlobLimit = sampleMemoryBudget + 2 * 1024 * 1024;
inline constexpr bool fitsSampleMemory(uint64_t frames, uint64_t otherBytes = 0) {
    return frames >= 2 && otherBytes <= sampleMemoryBudget &&
           frames <= (sampleMemoryBudget - otherBytes) / 8;
}
} // namespace logisth
