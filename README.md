<p align="center">
  A high-performance, deterministic custom memory allocator suite engineered in <strong>C++20</strong>, featuring linear bump arenas, LIFO stack allocators with marker rollbacks, intrusive free-list pools, an STL-compliant tracking allocator, and an interactive <strong>Raylib</strong> memory visualizer.
</p>

<p align="center">
  <a href="#quick-start"><img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-0078D6?logo=windows&logoColor=white" alt="Platform"></a>
  <a href="#tech-stack"><img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=c%2B%2B&logoColor=white" alt="C++20"></a>
  <a href="#tech-stack"><img src="https://img.shields.io/badge/Build-CMake%203.20%2B-064F8C?logo=cmake&logoColor=white" alt="CMake"></a>
  <a href="#visualizer"><img src="https://img.shields.io/badge/UI-Raylib-FFFFFF?logo=raylib&logoColor=black" alt="Raylib"></a>
  <a href="#verification--tests"><img src="https://img.shields.io/badge/Tests-Doctest-brightgreen" alt="Doctest"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License MIT"></a>
</p>

---

## Overview

General-purpose heap allocators (`malloc` / `operator new`) suffer from non-deterministic latency, heap fragmentation, and thread contention. **Mem-Allocator** provides specialized, cache-friendly memory allocators tailored for low-latency systems, game engines, and performance-critical backend services where memory allocation patterns are predictable and zero-fragmentation guarantees are required.

---

## Allocator Strategies

| Allocator | Allocation Time | Deallocation Time | Fragmentation | Primary Use Case |
| :--- | :--- | :--- | :--- | :--- |
| **Linear Allocator** | $O(1)$ | Bulk $O(1)$ (Reset) | None | Per-frame scratchpads, level/session loading, command buffers |
| **Stack Allocator** | $O(1)$ | $O(1)$ (Marker rewind) | None | Hierarchical execution scopes, nested task scratchpads, parser state |
| **Pool Allocator** | $O(1)$ | $O(1)$ | None (Internal) | High-frequency entity/particle pools, network packets, event nodes |
| **Tracking Allocator** | Delegates | Delegates | Minimal | Drop-in STL container wrapper (`std::vector`), leak profiling, high-water metrics |

---

## Key Highlights

- **Cache-Friendly & Deterministic**: Eliminates OS heap traversal overhead, keeping allocations contiguous and localized within CPU cache lines.
- **Strict Alignment Enforcement**: Full compliance with arbitrary alignments using `std::align` and padding tracking to prevent unaligned memory access penalties.
- **Intrusive Free-List Mechanics**: The `PoolAllocator` stores free slot pointers directly inside unoccupied blocks, yielding zero external bookkeeping overhead.
- **Marker-Based Rollbacks**: The `StackAllocator` records lightweight `Marker` checkpoints, enabling fine-grained, scoped unwind operations without heap churn.
- **Interactive Visualizer (Raylib)**: Real-time graphic visualizer illustrating memory arena layouts, padding bytes, chunk occupancy, and active stack markers.
- **STL Container Interoperability**: `TrackingAllocator<T>` implements the C++ standard allocator concept, integrating seamlessly into standard library containers like `std::vector` and `std::unordered_map`.

---

## Core Components

### 1. Linear Allocator (`LinearAllocator.h`)
- Sequential pointer-bump arena.
- Enforces user-specified alignment requirements with automatic padding calculation.
- Guaranteed $O(1)$ allocation; freeing is an instantaneous bulk reset operation.

### 2. Stack Allocator (`StackAllocator.h`)
- LIFO memory model with fine-grained marker checkpoints.
- Supports `get_marker()` and `free_to_marker(Marker)` to cleanly rollback memory usage to an earlier snapshot.
- Ideal for nested function calls, scoped frames, or transient sub-task allocations.

### 3. Pool Allocator (`PoolAllocator.h`)
- Allocates fixed-size chunks from a contiguous memory region.
- Employs an intrusive singly-linked free-list embedded directly in unoccupied memory chunks.
- Includes fast bounds-checking (`contains()`) and slot identification (`slot_index()`).

### 4. Tracking Allocator (`TrackingAllocator.h` & `AllocatorStats.h`)
- Conforms to the standard C++ allocator requirements (`std::allocator_traits`).
- Backed by the thread-safe `AllocatorStats` singleton providing live byte tracking, total allocation/deallocation counters, and peak usage diagnostics.

---

## Interactive Visualizer

The project includes a standalone visualizer powered by **Raylib** that displays memory state transitions in real time.

```
cmake-build-debug/visualizer/visualizer.exe
```

- **Visual Arena Map**: Inspect contiguous memory blocks with distinct color coding for data types and alignment padding.
- **Interactive Controls**: Allocate primitives (`char`, `int`, `double`) and custom structs (`Circle`, `Square`), trigger marker rollbacks, and inspect free-list head pointers.
- **Real-Time Telemetry**: Live readout of memory utilization, active offsets, and total arena capacity.

> *Note: Add a recording or screenshot of the visualizer here (`docs/media/visualizer_demo.gif`).*

---

## Verification & Tests

The test harness uses **doctest** to validate memory mechanics under stress and edge conditions:

- **Alignment Validation**: Verifies natural and custom byte boundaries across disparate primitive and struct layouts.
- **Arena Exhaustion**: Asserts deterministic `nullptr` returns when requested memory exceeds remaining capacity.
- **LIFO Stack Invariants**: Validates that rewinding to a marker properly restores capacity without corrupting prior allocations.
- **Free-List Consistency**: Tests cyclical allocation and deallocation across pool chunks to prevent list severance.
- **RAII & Leak Detection**: Asserts that destructors execute correctly and live byte counters return strictly to zero.

---

## Quick Start

### Requirements
- C++20 compliant compiler (MSVC v143+, GCC 11+, or Clang 13+)
- CMake 3.20+

### Build

```powershell
git clone https://github.com/Umair-Manzoor-47/Mem-Allocator.git
cd Mem-Allocator

# Configure and build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Run Tests

```powershell
ctest --test-dir build --output-on-failure
```

### Run Allocator Demos & Visualizer

```powershell
# Run console demo
./build/main

# Run interactive visualizer
./build/visualizer/visualizer
```

---

## Tech Stack

| Component | Technology | Description |
| :--- | :--- | :--- |
| **Language** | C++20 | Strict type safety, concepts, `<ranges>`, placement `new` |
| **Build System** | CMake | Modular target configuration |
| **Testing** | Doctest 2.4.11 | Fast, modern header-only unit testing |
| **Visualizer** | Raylib 4.2+ | 2D/3D hardware-accelerated rendering |

