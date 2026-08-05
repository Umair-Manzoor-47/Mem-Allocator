#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <memory/LinearAllocator.h>

TEST_CASE("allocations are properly aligned") {
    LinearAllocator allocator(1024);

    void* p1 = allocator.allocate(sizeof(char), alignof(char));
    void* p2 = allocator.allocate(sizeof(double), alignof(double));
    
    auto addr = reinterpret_cast<uintptr_t>(p2);
    CHECK(addr % alignof(double) == 0);
}

TEST_CASE("allocator returns nullptr when out of space") {
    LinearAllocator allocator(8);
    void* p1 = allocator.allocate(8);
    CHECK(p1 != nullptr);

    void* p2 = allocator.allocate(8);
    CHECK(p2 == nullptr);
}

TEST_CASE("allocate A struct with int + float + string") {
    LinearAllocator allocator(64);

    struct A {
        int a{4};
        float b{3.14f};
        std::string c{"Hello"};
    };

    allocator.allocate(sizeof(A));
    CHECK(allocator.used() == 40);
}

TEST_CASE("reset reclaims the whole arena") {
    LinearAllocator allocator(64);
    allocator.allocate(32);
    CHECK(allocator.used() == 32);

    allocator.reset();
    CHECK(allocator.used() == 0);
}