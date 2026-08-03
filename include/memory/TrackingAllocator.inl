#pragma once

#include <utility>
#include "TrackingAllocator.h"

template <class T>
T* TrackingAllocator<T>::allocate(std::size_t numObjects)
{
    mAllocations += numObjects;
    return static_cast<T*>(operator new(sizeof(T) * numObjects));
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
void TrackingAllocator<T>::deallocate(T* p, std::size_t numObjects)
{
    operator delete(p);
}
