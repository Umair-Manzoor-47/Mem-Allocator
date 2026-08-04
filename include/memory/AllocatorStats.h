#pragma once
#include <atomic>
#include <cstddef>

class AllocatorStats
{
private:
    std::atomic<std::size_t> totalAllocated{0};
    std::atomic<std::size_t> totalDeallocated{0};

    AllocatorStats() = default;

public:
    static AllocatorStats& instance() {
        static AllocatorStats s{};
        return s;
    }

    AllocatorStats(const AllocatorStats&) = delete;
    AllocatorStats& operator=(const AllocatorStats&) = delete;

    void allocateBytes(std::size_t numBytes) {
        totalAllocated.fetch_add(numBytes, std::memory_order_relaxed);
    }
    void deallocateBytes(std::size_t numBytes) {
        totalDeallocated.fetch_add(numBytes, std::memory_order_relaxed);
    }

    std::size_t getTotalAllocated() const { return totalAllocated.load(std::memory_order_relaxed); }
    std::size_t getTotalDeallocated() const { return totalDeallocated.load(std::memory_order_relaxed); }
    std::size_t getLiveBytes() const { return totalAllocated.load() - totalDeallocated.load(); }

    void reset() {
        totalAllocated.store(0, std::memory_order_relaxed);
        totalDeallocated.store(0, std::memory_order_relaxed);
    }
};