#pragma once

#include <new>
#include <utility>
#include "TrackingAllocator.h"

template <class T>
T* TrackingAllocator<T>::allocate(std::size_t numObjects)
{
    if (numObjects > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
        throw std::bad_array_new_length();
    }
    std::size_t bytes = sizeof(T) * numObjects;
    if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
        return static_cast<T*>(operator new(bytes, std::align_val_t{alignof(T)}));
    }
    AllocatorStats::instance().allocateBytes(bytes);
    return static_cast<T*>(operator new(bytes));
}

template <class T>
T* TrackingAllocator<T>::allocate(std::size_t numObjects, const void* hint)
{
    return allocate(numObjects);
}

template <class T>
template <class U, class ... Args>
void TrackingAllocator<T>::construct(U* p, Args&&... args)
{
    new (p) U(std::forward<Args>(args)...);
}

template <class T>
template <class U>
void TrackingAllocator<T>::destroy(U* p)
{
    p->~U();
}

template <class T>
void TrackingAllocator<T>::deallocate(T* p, std::size_t numObjects) noexcept
{
    AllocatorStats::instance().deallocateBytes(sizeof(T) * numObjects);
    if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
        operator delete(p, std::align_val_t{alignof(T)});
    } else {
        operator delete(p);
    }
}
