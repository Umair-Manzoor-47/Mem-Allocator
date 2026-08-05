#pragma once
#include <cstddef>
#include <memory>
#include <ranges>

class StackAllocator {

private:
    std::size_t m_size;
    std::byte* m_start;
    std::byte* m_current;

public:
    using Marker = std::size_t;

    explicit StackAllocator(std::size_t sizeByte):
    m_size(sizeByte),
    m_start(static_cast<std::byte *>(::operator new(sizeByte))),
    m_current(m_start) {};

    ~StackAllocator() { ::operator delete(m_start); }

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {

        void* ptr = m_current;
        std::size_t space = m_size - (m_current - m_start);

        if (!std::align(alignment, size, ptr, space)) {
            return nullptr;
        }

        m_current = static_cast<std::byte*>(ptr) + size;
        return ptr;
    }

    [[nodiscard]] Marker get_marker() const { return static_cast<std::size_t>(m_current - m_start); }

    void free_to_marker(Marker marker) { m_current = m_start + marker; }

    void reset() { m_current = m_start; }

    [[nodiscard]] std::size_t used() const { return m_current - m_start; }

    [[nodiscard]] std::size_t capacity() const { return m_size; }
};
