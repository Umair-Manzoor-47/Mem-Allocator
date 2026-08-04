#pragma once
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory/TrackingAllocator.h>
#include <memory/AllocatorStats.h>
#include <memory/linear_allocator.h>
#include <demo/shape.h>

inline void print_section(const std::string& title) {
    std::cout << "\n========== " << title << " ==========\n";
}

// ---------------- TrackingAllocator demo ----------------

inline void demo_tracking_allocator() {
    print_section("TrackingAllocator demo");
    AllocatorStats::instance().reset();

    std::cout << "[start] live bytes: " << AllocatorStats::instance().getLiveBytes() << "\n";

    std::vector<int, TrackingAllocator<int>> v;

    std::cout << "\n-- reserving space for 10 ints --\n";
    v.reserve(10);
    std::cout << "  total allocated: " << AllocatorStats::instance().getTotalAllocated() << " bytes\n";
    std::cout << "  live bytes:      " << AllocatorStats::instance().getLiveBytes() << " bytes\n";
    std::cout << "  (expected >= " << 10 * sizeof(int) << " bytes for 10 ints)\n";

    std::cout << "\n-- pushing 3 elements (within capacity, no reallocation) --\n";
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    std::cout << "  live bytes unchanged: " << AllocatorStats::instance().getLiveBytes() << " bytes\n";
    std::cout << "  (no new allocation happened, since we're within reserved capacity)\n";

    std::cout << "\n-- forcing growth beyond capacity (push 20 more) --\n";
    for (int i = 0; i < 20; ++i) v.push_back(i);
    std::cout << "  total allocated now: " << AllocatorStats::instance().getTotalAllocated() << " bytes\n";
    std::cout << "  total deallocated now: " << AllocatorStats::instance().getTotalDeallocated() << " bytes\n";
    std::cout << "  (deallocated > 0 means the vector outgrew its buffer and moved elements to a bigger one,\n";
    std::cout << "   freeing the old buffer -- this is the 'grow and copy' cost people mean when they say\n";
    std::cout << "   'reserve() to avoid reallocations')\n";
    std::cout << "  live bytes: " << AllocatorStats::instance().getLiveBytes() << " bytes\n";

    std::cout << "\n-- clearing and destructing the vector --\n";
    v.clear();
    std::cout << "  live bytes after clear() (elements destroyed, memory NOT freed yet): "
              << AllocatorStats::instance().getLiveBytes() << " bytes\n";
    std::cout << "  (clear() calls destructors but keeps the buffer -- this is why live bytes doesn't drop to 0 here)\n";

    {
        std::vector<int, TrackingAllocator<int>> scoped;
        scoped.reserve(50);
        std::cout << "\n-- inner scoped vector allocated 50 ints, live bytes now: "
                  << AllocatorStats::instance().getLiveBytes() << " --\n";
    } // scoped vector's destructor runs here, buffer freed
    std::cout << "-- after scoped vector destructed, live bytes: "
              << AllocatorStats::instance().getLiveBytes() << " --\n";
    std::cout << "  (this proves RAII: leaving scope triggered deallocate() automatically)\n";
}

// ---------------- LinearAllocator demo ----------------

inline void demo_linear_allocator() {
    print_section("LinearAllocator demo");

    LinearAllocator allocator(64); // small on purpose, so we can hit the ceiling
    std::cout << "[start] capacity: " << allocator.capacity() << " bytes, used: " << allocator.used() << " bytes\n";

    std::cout << "\n-- allocating a char (1 byte, alignment 1) --\n";
    void* p1 = allocator.allocate(sizeof(char), alignof(char));
    std::cout << "  address: " << p1 << "\n";
    std::cout << "  used: " << allocator.used() << " / " << allocator.capacity() << "\n";

    std::cout << "\n-- allocating a double (8 bytes, needs 8-byte alignment) --\n";
    void* p2 = allocator.allocate(sizeof(double), alignof(double));
    std::cout << "  address: " << p2 << "\n";
    auto gap = static_cast<char*>(p2) - static_cast<char*>(p1);
    std::cout << "  gap from previous allocation: " << gap << " bytes\n";
    std::cout << "  (gap > 1 means padding was inserted to satisfy double's 8-byte alignment requirement)\n";
    std::cout << "  used: " << allocator.used() << " / " << allocator.capacity() << "\n";

    std::cout << "\n-- constructing real Shape objects via placement new --\n";
    void* circleMem = allocator.allocate(sizeof(Circle), alignof(Circle));
    Shape* circle = new (circleMem) Circle();
    std::cout << "  Circle constructed at: " << circleMem << "\n";
    circle->draw();
    std::cout << "  used: " << allocator.used() << " / " << allocator.capacity() << "\n";

    std::cout << "\n-- deliberately exhausting the arena --\n";
    void* overflow = allocator.allocate(1000); // way bigger than remaining capacity
    std::cout << "  requested 1000 bytes (more than remains), result: "
              << (overflow == nullptr ? "nullptr (correctly rejected)" : "UNEXPECTED non-null!") << "\n";

    std::cout << "\n-- manually destroying and resetting --\n";
    circle->~Shape();
    allocator.reset();
    std::cout << "  after reset(), used: " << allocator.used() << " / " << allocator.capacity() << "\n";
    std::cout << "  (reset() rewinds the pointer -- it does NOT call destructors, which is why we called ~Shape() first;\n";
    std::cout << "   forgetting that step would leak any resources the object owned, even though the arena itself is 'freed')\n";
}