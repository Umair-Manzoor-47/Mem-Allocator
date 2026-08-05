#pragma once
#include <cstddef>
#include <memory>

class LinearAllocator {
public:
    LinearAllocator(size_t sizeBytes)
        : m_size(sizeBytes),
          m_start(static_cast<std::byte*>(::operator new(sizeBytes))),
          m_current(m_start) {}

    ~LinearAllocator() {
        ::operator delete(m_start);
    }

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        void* ptr = m_current;
        size_t space = m_size - (m_current - m_start);
        
        if (!std::align(alignment, size, ptr, space)) {
            return nullptr;
        }

        m_current = static_cast<std::byte*>(ptr) + size;
        return ptr;
    }


    void reset() {
        m_current = m_start;
    }

    [[nodiscard]] size_t used() const { return m_current - m_start; }
    [[nodiscard]] size_t capacity() const { return m_size; }

private:
    size_t m_size;
    std::byte* m_start;
    std::byte* m_current;
};