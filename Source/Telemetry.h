#pragma once
#include <array>
#include <atomic>
#include <cstddef>
namespace xiffects {
struct TelemetryFrame { float detected=0, corrected=0, minimum=0, maximum=0; };
// Single producer (audio), single consumer (editor). Full queue drops new frames.
// No GUI reads audio-owned buffers; acquire/release publishes complete frames.
class TelemetryQueue {
public:
    bool push(TelemetryFrame frame) noexcept {
        const auto h=head.load(std::memory_order_relaxed), next=(h+1)%capacity;
        if(next==tail.load(std::memory_order_acquire)) return false;
        frames[h]=frame; head.store(next,std::memory_order_release); return true;
    }
    bool pop(TelemetryFrame& frame) noexcept {
        const auto t=tail.load(std::memory_order_relaxed);
        if(t==head.load(std::memory_order_acquire)) return false;
        frame=frames[t]; tail.store((t+1)%capacity,std::memory_order_release); return true;
    }
private:
    static constexpr size_t capacity=1024;
    std::array<TelemetryFrame,capacity> frames{};
    std::atomic<size_t> head{0},tail{0};
};
}
