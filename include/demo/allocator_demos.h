#pragma once
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory/TrackingAllocator.h>
#include <memory/AllocatorStats.h>
#include <memory/LinearAllocator.h>
#include <demo/shape.h>
#include <memory/StackAllocator.h>
#include <memory/PoolAllocator.h>

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

// -------------------------------- Stack Allocator ----------------------------------------------------

inline void demo_stack_allocator() {
    print_section("StackAllocator demo");

    StackAllocator allocator(64); // small on purpose

    std::cout << "[start] capacity: "
              << allocator.capacity()
              << " bytes, used: "
              << allocator.used()
              << " bytes\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- allocating an int (4 bytes) --\n";

    int* a = static_cast<int*>(
        allocator.allocate(sizeof(int), alignof(int)));

    *a = 42;

    std::cout << "  address: " << a << "\n";
    std::cout << "  value: " << *a << "\n";
    std::cout << "  used: "
              << allocator.used()
              << " / "
              << allocator.capacity()
              << "\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- saving a stack marker --\n";

    StackAllocator::Marker marker = allocator.get_marker();

    std::cout << "  marker = " << marker << " bytes\n";
    std::cout << "  (everything allocated after this marker can be freed together)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- allocating a float and a double --\n";

    float* b = static_cast<float*>(
        allocator.allocate(sizeof(float), alignof(float)));

    double* c = static_cast<double*>(
        allocator.allocate(sizeof(double), alignof(double)));

    *b = 3.14f;
    *c = 99.99;

    std::cout << "  float  address: " << b << "\n";
    std::cout << "  double address: " << c << "\n";

    std::cout << "  used: "
              << allocator.used()
              << " / "
              << allocator.capacity()
              << "\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- constructing a Circle using placement new --\n";

    void* circleMem =
        allocator.allocate(sizeof(Circle), alignof(Circle));

    Circle* circle = new (circleMem) Circle();

    std::cout << "  Circle constructed at: " << circle << "\n";
    circle->draw();

    std::cout << "  used: "
              << allocator.used()
              << " / "
              << allocator.capacity()
              << "\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- rolling back to the saved marker --\n";

    // Placement-new objects must be destroyed manually before rollback.
    circle->~Circle();

    allocator.free_to_marker(marker);

    std::cout << "  used: "
              << allocator.used()
              << " / "
              << allocator.capacity()
              << "\n";

    std::cout << "  (the float, double, and Circle memory became available\n"
                 "   instantly because the allocator simply moved its pointer\n"
                 "   back to the saved marker)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- allocating again after rollback --\n";

    char* d = static_cast<char*>(
        allocator.allocate(16, alignof(char)));

    std::cout << "  new allocation address: "
              << static_cast<void*>(d)
              << "\n";

    std::cout << "  used: "
              << allocator.used()
              << " / "
              << allocator.capacity()
              << "\n";

    std::cout << "  (notice this allocation likely reuses the same memory\n"
                 "   previously occupied by the float, double, and Circle)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- deliberately exhausting the stack allocator --\n";

    void* overflow = allocator.allocate(1000);

    std::cout << "  requested 1000 bytes, result: "
              << (overflow ? "UNEXPECTED non-null!" : "nullptr (correctly rejected)")
              << "\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- resetting the allocator --\n";

    allocator.reset();

    std::cout << "  after reset(), used: "
              << allocator.used()
              << " / "
              << allocator.capacity()
              << "\n";

    std::cout << "  (reset() rewinds the stack to the beginning.\n"
                 "   Like free_to_marker(), it DOES NOT call destructors,\n"
                 "   so any placement-new objects must be destroyed first.)\n";
}

// -------------------------------- Pool Allocator ----------------------------------------------------

inline void demo_pool_allocator() {
    print_section("PoolAllocator demo");

    // A pool allocator gives us a fixed number of equally-sized chunks.
    // Every allocation returns exactly one chunk.
    //
    // We'll use a small pool on purpose so that we can easily demonstrate:
    //   1. Allocation from the free list
    //   2. Releasing chunks back into the pool
    //   3. Reusing released chunks
    //   4. Running out of chunks

    constexpr std::size_t chunkSize = sizeof(int);
    constexpr std::size_t chunkCount = 4;

    PoolAllocator allocator(chunkSize, chunkCount);

    std::cout << "[start] chunk size:  "
              << allocator.chunk_size()
              << " bytes\n";

    std::cout << "        chunk count: "
              << allocator.chunk_count()
              << "\n";

    std::cout << "        total pool:  "
              << allocator.chunk_size() * allocator.chunk_count()
              << " bytes\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- allocating four integers --\n";

    int* a = static_cast<int*>(allocator.allocate());
    int* b = static_cast<int*>(allocator.allocate());
    int* c = static_cast<int*>(allocator.allocate());
    int* d = static_cast<int*>(allocator.allocate());

    *a = 10;
    *b = 20;
    *c = 30;
    *d = 40;

    std::cout << "  a = " << *a << " at " << static_cast<void*>(a) << "\n";
    std::cout << "  b = " << *b << " at " << static_cast<void*>(b) << "\n";
    std::cout << "  c = " << *c << " at " << static_cast<void*>(c) << "\n";
    std::cout << "  d = " << *d << " at " << static_cast<void*>(d) << "\n";

    std::cout << "  (each allocation consumes exactly one fixed-size chunk)\n";
    std::cout << "  (there is no searching for a suitable block and no heap allocation per integer)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- pool is now exhausted --\n";

    void* exhausted = allocator.allocate();

    std::cout << "  fifth allocation result: "
              << (exhausted ? "UNEXPECTED non-null!" : "nullptr (correctly rejected)")
              << "\n";

    std::cout << "  (the pool contains exactly "
              << allocator.chunk_count()
              << " chunks, so the fifth allocation cannot succeed)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- returning two chunks to the pool --\n";

    allocator.deallocate(b);
    allocator.deallocate(d);

    std::cout << "  returned b and d to the free list\n";
    std::cout << "  (deallocate() does not call delete or return memory to the OS;\n"
                 "   it simply puts the chunk back into the allocator's free list)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- allocating again after deallocation --\n";

    int* e = static_cast<int*>(allocator.allocate());
    int* f = static_cast<int*>(allocator.allocate());

    *e = 50;
    *f = 60;

    std::cout << "  e = " << *e << " at " << static_cast<void*>(e) << "\n";
    std::cout << "  f = " << *f << " at " << static_cast<void*>(f) << "\n";

    std::cout << "\n  address reuse:\n";
    std::cout << "    b was at " << static_cast<void*>(b) << "\n";
    std::cout << "    e is  at " << static_cast<void*>(e) << "\n";

    std::cout << "    d was at " << static_cast<void*>(d) << "\n";
    std::cout << "    f is  at " << static_cast<void*>(f) << "\n";

    std::cout << "\n  (notice how the allocator reuses the chunks that were just freed)\n";
    std::cout << "  (this is the key idea behind a pool allocator:\n"
                 "   allocation and deallocation are extremely cheap because\n"
                 "   the memory was already reserved up front)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- demonstrating constant-size allocation --\n";

    std::cout << "  sizeof(int): "
              << sizeof(int)
              << " bytes\n";

    std::cout << "  pool chunk size: "
              << allocator.chunk_size()
              << " bytes\n";

    std::cout << "  every allocate() returns exactly one chunk of this size\n";
    std::cout << "  (unlike a general-purpose allocator, this pool cannot satisfy\n"
                 "   arbitrary allocation sizes)\n";

    // ---------------------------------------------------------------------
    std::cout << "\n-- cleaning up allocated chunks --\n";

    allocator.deallocate(a);
    allocator.deallocate(c);
    allocator.deallocate(e);
    allocator.deallocate(f);

    std::cout << "  all chunks returned to the pool\n";
    std::cout << "  pool allocator destructor will release the entire backing block\n";

    std::cout << "\n  PoolAllocator takeaway:\n";
    std::cout << "    * fixed-size allocations\n"
                 "    * O(1) allocate()\n"
                 "    * O(1) deallocate()\n"
                 "    * no per-object heap allocation\n"
                 "    * freed chunks are immediately reusable\n"
                 "    * excellent for many objects of the same size\n";
}