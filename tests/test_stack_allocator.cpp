#include <doctest/doctest.h>
#include <memory/StackAllocator.h>

TEST_CASE("get_marker reflects current usage") {
    StackAllocator a(1024);
    CHECK(a.get_marker() == 0);
    a.allocate(64);
    CHECK(a.get_marker() == 64);
}

TEST_CASE("free_to_marker rewinds usage correctly") {
    StackAllocator a(1024);
    a.allocate(64);
    auto marker = a.get_marker();

    a.allocate(128);
    a.allocate(32);
    CHECK(a.used() == 224);

    a.free_to_marker(marker);
    CHECK(a.used() == 64);
}

TEST_CASE("memory freed to a marker can be reused") {
    StackAllocator a(128);
    auto marker = a.get_marker();

    void* p1 = a.allocate(64);
    a.free_to_marker(marker);

    void* p2 = a.allocate(64);
    CHECK(p1 == p2); // should get the exact same address back
}

TEST_CASE("nested markers work in LIFO order") {
    StackAllocator a(1024);
    auto outer = a.get_marker();
    a.allocate(50);

    auto inner = a.get_marker();
    a.allocate(50);

    a.free_to_marker(inner);
    CHECK(a.used() == 50);

    a.free_to_marker(outer);
    CHECK(a.used() == 0);
}

TEST_CASE("allocate respects alignment same as LinearAllocator") {
    StackAllocator a(1024);
    a.allocate(sizeof(char), alignof(char));
    void* p = a.allocate(sizeof(double), alignof(double));

    auto addr = reinterpret_cast<uintptr_t>(p);
    CHECK(addr % alignof(double) == 0);
}

TEST_CASE("allocate returns nullptr when out of space") {
    StackAllocator a(8);
    CHECK(a.allocate(8) != nullptr);
    CHECK(a.allocate(8) == nullptr);
}