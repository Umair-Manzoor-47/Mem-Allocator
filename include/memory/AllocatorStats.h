#pragma once
#include <cstddef>

struct AllocatorStats
{
private:
    std::size_t totalAllocated = 0;
    std::size_t totalDeallocated = 0;

    AllocatorStats() = default;

public:
    static AllocatorStats& instance() {
        static AllocatorStats s{};
        return s;
    }

    AllocatorStats(const AllocatorStats&) = delete;
    AllocatorStats& operator=(const AllocatorStats&) = delete;

    void allocateBytes(std::size_t numBytes) { totalAllocated += numBytes; }
    void deallocateBytes(std::size_t numBytes) { totalDeallocated += numBytes; }

    std::size_t GetTotalAllocated() const  { return totalAllocated; }
    std::size_t GetTotalDeallocated() const { return totalDeallocated; }

    std::size_t GetLiveBytes() const { return totalAllocated - totalDeallocated; }
    
    void resetForTesting() { totalAllocated = 0; totalDeallocated = 0; }
};
