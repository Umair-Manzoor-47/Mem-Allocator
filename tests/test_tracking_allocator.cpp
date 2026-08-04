#include <doctest/doctest.h>
#include <memory/TrackingAllocator.h>
#include <memory/AllocatorStats.h>
#include <vector>
#include <list>

struct Counted {
    static inline int alive = 0;
    int value;
    Counted(int v = 0) : value(v) { ++alive; }
    ~Counted() { --alive; }
};

struct alignas(32) OverAligned {
    char data[32];
};

TEST_CASE("allocate increases live bytes by the correct size") {
    AllocatorStats::instance().reset();

    TrackingAllocator<int> a;
    int* p = a.allocate(4); // 4 * sizeof(int)

    CHECK(a.getLiveBytes() == 4 * sizeof(int));

    a.deallocate(p, 4);
}

TEST_CASE("deallocate brings live bytes back to zero") {
    AllocatorStats::instance().reset();

    TrackingAllocator<int> a;
    int* p = a.allocate(10);
    a.deallocate(p, 10);

    CHECK(a.getLiveBytes() == 0);
}

TEST_CASE("totalAllocated and totalDeallocated never decrease independently") {
    AllocatorStats::instance().reset();

    TrackingAllocator<int> a;
    int* p1 = a.allocate(5);
    int* p2 = a.allocate(3);
    a.deallocate(p1, 5);

    CHECK(AllocatorStats::instance().getTotalAllocated() == 8 * sizeof(int));
    CHECK(AllocatorStats::instance().getTotalDeallocated() == 5 * sizeof(int));
    CHECK(a.getLiveBytes() == 3 * sizeof(int));

    a.deallocate(p2, 3);
}

TEST_CASE("stats are shared across different TrackingAllocator<T> types") {
    AllocatorStats::instance().reset();

    TrackingAllocator<int> ai;
    TrackingAllocator<double> ad;

    int* pi = ai.allocate(4);     // 4 * sizeof(int)
    double* pd = ad.allocate(2);  // 2 * sizeof(double)

    std::size_t expected = 4 * sizeof(int) + 2 * sizeof(double);
    CHECK(ai.getLiveBytes() == expected);
    CHECK(ad.getLiveBytes() == expected); // same singleton, same total

    ai.deallocate(pi, 4);
    ad.deallocate(pd, 2);

    CHECK(ai.getLiveBytes() == 0);
}

TEST_CASE("construct and destroy actually invoke ctor/dtor") {
    AllocatorStats::instance().reset();



    TrackingAllocator<Counted> a;
    Counted* p = a.allocate(1);
    a.construct(p);
    CHECK(Counted::alive == 1);

    a.destroy(p);
    CHECK(Counted::alive == 0);

    a.deallocate(p, 1);
}

TEST_CASE("works as a real allocator with std::vector") {
    AllocatorStats::instance().reset();

    std::vector<int, TrackingAllocator<int>> v;
    v.reserve(100);
    v.push_back(1);
    v.push_back(2);

    CHECK(v.get_allocator().getLiveBytes() >= 100 * sizeof(int));
}

TEST_CASE("rebind lets it work with node-based containers like std::list") {
    AllocatorStats::instance().reset();

    std::list<int, TrackingAllocator<int>> l;
    l.push_back(1);
    l.push_back(2);

    CHECK(l.front() == 1);
    CHECK(AllocatorStats::instance().getLiveBytes() > 0);
}

TEST_CASE("allocate(0) does not crash and adds zero bytes") {
    AllocatorStats::instance().reset();
    TrackingAllocator<int> a;

    int* p = a.allocate(0);
    CHECK(AllocatorStats::instance().getTotalAllocated() == 0);
    // deallocate(p, 0) must also be safe even if p happens to be non-null
    a.deallocate(p, 0);
}

TEST_CASE("allocate throws on overflowing size request") {
    AllocatorStats::instance().reset();
    TrackingAllocator<int> a;

    std::size_t huge = std::numeric_limits<std::size_t>::max() / sizeof(int) + 1;
    CHECK_THROWS_AS(a.allocate(huge), std::bad_array_new_length);
}

TEST_CASE("over-aligned types get properly aligned memory") {
    AllocatorStats::instance().reset();
    TrackingAllocator<OverAligned> a;

    OverAligned* p = a.allocate(1);
    auto addr = reinterpret_cast<uintptr_t>(p);
    CHECK(addr % alignof(OverAligned) == 0);

    a.deallocate(p, 1);
}

TEST_CASE("repeated alloc/dealloc cycles never drift") {
    AllocatorStats::instance().reset();
    TrackingAllocator<int> a;

    for (int i = 0; i < 1000; ++i) {
        int* p = a.allocate(10);
        a.deallocate(p, 10);
    }

    CHECK(a.getLiveBytes() == 0);
    CHECK(AllocatorStats::instance().getTotalAllocated() == 1000 * 10 * sizeof(int));
    CHECK(AllocatorStats::instance().getTotalDeallocated() == 1000 * 10 * sizeof(int));
}

TEST_CASE("equality operators report allocators as equal (stateless)") {
    TrackingAllocator<int> a1;
    TrackingAllocator<int> a2;
    TrackingAllocator<double> a3;

    CHECK(a1 == a2);
    CHECK_FALSE(a1 != a2);
    CHECK(a1 == a3); // cross-type comparison, must compile and be true
}

TEST_CASE("construct forwards arguments to the constructor") {
    AllocatorStats::instance().reset();
    TrackingAllocator<Counted> a;

    Counted* p = a.allocate(1);
    a.construct(p, 42);
    CHECK(p->value == 42);
    CHECK(Counted::alive == 1);

    a.destroy(p);
    a.deallocate(p, 1);
}

TEST_CASE("rebind constructor works when converting between allocator types") {
    AllocatorStats::instance().reset();
    TrackingAllocator<int> a_int;

    // this exercises the templated `TrackingAllocator(const TrackingAllocator<U>&)`
    TrackingAllocator<double> a_double(a_int);
    double* p = a_double.allocate(2);
    CHECK(AllocatorStats::instance().getTotalAllocated() == 2 * sizeof(double));

    a_double.deallocate(p, 2);
}