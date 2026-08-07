#pragma once
#include <cstddef>
#include <cstdlib>

class PoolAllocator {

private:
    std::size_t m_chunkSize;
    std::size_t m_chunkCount;
    std::byte* m_start;
    void* m_freeListHead;

public:
    PoolAllocator(std::size_t chunkSize, std::size_t chunkCount):
    m_chunkSize(chunkSize < sizeof(void*) ? sizeof(void*) : chunkSize),
    m_chunkCount(chunkCount),
    m_start(static_cast<std::byte*>(::operator new(m_chunkSize * chunkCount))),
    m_freeListHead(nullptr) {

        for (std::size_t i = 0; i < m_chunkCount; ++i) {
            std::byte* slot = m_start + i * m_chunkSize;
            void* next = (i + 1 < m_chunkCount) ? (m_start + (i + 1) * m_chunkSize) : nullptr;
            *reinterpret_cast<void**>(slot) = next;
        }
        m_freeListHead = m_start;

    };

    ~PoolAllocator() {
        ::operator delete(m_start);
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    void* allocate() {
        if (m_freeListHead == nullptr) {
            return nullptr;
        }
        void* slot = m_freeListHead;
        m_freeListHead = *reinterpret_cast<void**>(slot);
        return slot;
    }
    void deallocate(void* ptr) {
        *reinterpret_cast<void**>(ptr) = m_freeListHead;
        m_freeListHead = ptr;
    }
    std::size_t chunk_size() const { return m_chunkSize; }
    std::size_t chunk_count() const { return m_chunkCount; }

};
