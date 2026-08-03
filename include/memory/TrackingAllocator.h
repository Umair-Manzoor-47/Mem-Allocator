#pragma once
#include <cstddef>
#include <limits>


template <class T>
class TrackingAllocator
{
    static std::size_t mAllocations;

public:

    using value_type = T;

    TrackingAllocator() = default;
    ~TrackingAllocator() = default;

    T* allocate(std::size_t numObjects);
    T* allocate(std::size_t numObjects, const void* hint);

    template <class U, class... Args>
    void construct(U* p, Args&&... args);


    template <class U>
    void destroy(U* p);

    void deallocate(T* p, std::size_t numObjects);

    std::size_t getAllocations() const { return mAllocations; };

    std::size_t max_size() const { return std::numeric_limits<std::size_t>::max(); };
};
template <class T>
std::size_t  TrackingAllocator<T>::mAllocations = 0;
#include "TrackingAllocator.inl"
