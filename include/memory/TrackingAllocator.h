#pragma once
#include <cstddef>
#include <limits>
#include <memory/AllocatorStats.h>

template <class T>
class TrackingAllocator
{
public:
    using value_type = T;

    TrackingAllocator() = default;
    ~TrackingAllocator() = default;

    template <class U>
    TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

    T* allocate(std::size_t numObjects);
    T* allocate(std::size_t numObjects, const void* hint);

    template <class U, class... Args>
    void construct(U* p, Args&&... args);

    template <class U>
    void destroy(U* p);

    void deallocate(T* p, std::size_t numObjects);

    std::size_t getLiveBytes() const { return AllocatorStats::instance().GetLiveBytes(); }
    std::size_t max_size() const { return std::numeric_limits<std::size_t>::max(); }
};

template <class T, class U>
bool operator==(const TrackingAllocator<T>&, const TrackingAllocator<U>&) noexcept { return true; }

template <class T, class U>
bool operator!=(const TrackingAllocator<T>&, const TrackingAllocator<U>&) noexcept { return false; }
#include "TrackingAllocator.inl"
