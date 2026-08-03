#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <memory/TrackingAllocator.h>
#include <memory/AllocatorStats.h>
#include <vector>
#include <list>

TEST_CASE("allocate increases live bytes by the correct size") {
    AllocatorStats::instance().resetForTesting();

    TrackingAllocator<int> a;
    int* p = a.allocate(4); // 4 * sizeof(int)

    CHECK(a.getLiveBytes() == 4 * sizeof(int));

    a.deallocate(p, 4);
}

TEST_CASE("deallocate brings live bytes back to zero") {
    AllocatorStats::instance().resetForTesting();

    TrackingAllocator<int> a;
    int* p = a.allocate(10);
    a.deallocate(p, 10);

    CHECK(a.getLiveBytes() == 0);
}

TEST_CASE("totalAllocated and totalDeallocated never decrease independently") {
    AllocatorStats::instance().resetForTesting();

    TrackingAllocator<int> a;
    int* p1 = a.allocate(5);
    int* p2 = a.allocate(3);
    a.deallocate(p1, 5);

    CHECK(AllocatorStats::instance().GetTotalAllocated() == 8 * sizeof(int));
    CHECK(AllocatorStats::instance().GetTotalDeallocated() == 5 * sizeof(int));
    CHECK(a.getLiveBytes() == 3 * sizeof(int));

    a.deallocate(p2, 3);
}

TEST_CASE("stats are shared across different TrackingAllocator<T> types") {
    AllocatorStats::instance().resetForTesting();

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
    AllocatorStats::instance().resetForTesting();

    struct Counted {
        static inline int alive = 0;
        Counted() { ++alive; }
        ~Counted() { --alive; }
    };

    TrackingAllocator<Counted> a;
    Counted* p = a.allocate(1);
    a.construct(p);
    CHECK(Counted::alive == 1);

    a.destroy(p);
    CHECK(Counted::alive == 0);

    a.deallocate(p, 1);
}

TEST_CASE("works as a real allocator with std::vector") {
    AllocatorStats::instance().resetForTesting();

    std::vector<int, TrackingAllocator<int>> v;
    v.reserve(100);
    v.push_back(1);
    v.push_back(2);

    CHECK(v.get_allocator().getLiveBytes() >= 100 * sizeof(int));
}

TEST_CASE("rebind lets it work with node-based containers like std::list") {
    AllocatorStats::instance().resetForTesting();

    std::list<int, TrackingAllocator<int>> l;
    l.push_back(1);
    l.push_back(2);

    CHECK(l.front() == 1);
    CHECK(AllocatorStats::instance().GetLiveBytes() > 0);
}