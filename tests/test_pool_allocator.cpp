#include <doctest/doctest.h>
#include <memory/PoolAllocator.h>

TEST_CASE("allocate returns non-null while slots remain") {
    PoolAllocator pool(sizeof(int), 4);
    void* p1 = pool.allocate();
    void* p2 = pool.allocate();
    CHECK(p1 != nullptr);
    CHECK(p2 != nullptr);
    CHECK(p1 != p2);
}

TEST_CASE("allocate returns nullptr once pool is exhausted") {
    PoolAllocator pool(sizeof(int), 2);
    pool.allocate();
    pool.allocate();
    CHECK(pool.allocate() == nullptr);
}

TEST_CASE("deallocate returns a slot to the free list for reuse") {
    PoolAllocator pool(sizeof(int), 1);
    void* p1 = pool.allocate();
    CHECK(pool.allocate() == nullptr);

    pool.deallocate(p1);
    void* p2 = pool.allocate();
    CHECK(p2 == p1);
}

TEST_CASE("all slots are eventually reachable via repeated alloc") {
    PoolAllocator pool(sizeof(int), 8);
    void* slots[8];
    for (int i = 0; i < 8; ++i) {
        slots[i] = pool.allocate();
        CHECK(slots[i] != nullptr);
    }
    CHECK(pool.allocate() == nullptr);
}