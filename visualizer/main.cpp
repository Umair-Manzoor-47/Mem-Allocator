#include <raylib.h>

#include <memory/LinearAllocator.h>
#include <memory/PoolAllocator.h>
#include <memory/StackAllocator.h>
#include <memory/TrackingAllocator.h>

#include "memory_scene.h"

#include <algorithm>
#include <cstddef>
#include <vector>

struct Preset {
    const char* name;
    size_t size;
    size_t align;
    Color color;
};

static constexpr Preset PRESETS[] = {
    {"char",   1,  1, {94, 234, 212, 255}},
    {"int",    4,  4, {130, 170, 255, 255}},
    {"double", 8,  8, {198, 160, 246, 255}},
    {"Circle", 12, 4, {247, 140, 108, 255}},
    {"Square", 16, 8, {166, 227, 161, 255}},
};

struct Allocation {
    void* ptr;
    size_t start;
    size_t size;
    size_t marker;
};

int main()
{
    InitWindow(
        1100,
        700,
        "Memory Lab"
    );

    SetTargetFPS(60);

    // --------------------------------------------------
    // Camera
    // --------------------------------------------------

    Camera3D camera{};

    camera.position = {
        7.5f,
        9.0f,
        7.5f
    };

    camera.target = {
        0.0f,
        0.0f,
        0.0f
    };

    camera.up = {
        0.0f,
        1.0f,
        0.0f
    };

    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // --------------------------------------------------
    // Allocators
    // --------------------------------------------------

    constexpr size_t ARENA_SIZE = 256;

    LinearAllocator linear(ARENA_SIZE);
    StackAllocator stack(ARENA_SIZE);

    constexpr size_t POOL_CHUNK_SIZE = 16;
    constexpr size_t POOL_CHUNK_COUNT = 16;

    PoolAllocator pool(
        POOL_CHUNK_SIZE,
        POOL_CHUNK_COUNT
    );

    TrackingAllocator<std::byte> tracking;

    // --------------------------------------------------
    // Visualizer
    // --------------------------------------------------

    MemoryScene scene(
        ARENA_SIZE,
        16,
        0.45f
    );

    AllocatorType currentAllocator =
        AllocatorType::Linear;

    scene.pushLog(
        "Linear allocator ready"
    );

    // Stack allocations.
    std::vector<Allocation> stackAllocations;

    // Pool allocations.
    std::vector<Allocation> poolAllocations;

    // Tracking allocations.
    std::vector<Allocation> trackingAllocations;

    // Virtual placement for tracking visualization.
    std::vector<bool> trackingUsed(
        ARENA_SIZE,
        false
    );

    // --------------------------------------------------
    // Main loop
    // --------------------------------------------------

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        // ==================================================
        // UI
        // ==================================================

        SceneInput input =
            scene.handleInput(currentAllocator);

        // --------------------------------------------------
        // Change allocator
        // --------------------------------------------------

        if (input.action == SceneAction::None) {

            if (input.allocator != currentAllocator) {

                currentAllocator =
                    input.allocator;

                scene.onReset();

                switch (currentAllocator) {

                case AllocatorType::Linear:
                    scene.pushLog(
                        "Switched to Linear"
                    );
                    break;

                case AllocatorType::Stack:
                    scene.pushLog(
                        "Switched to Stack"
                    );
                    break;

                case AllocatorType::Pool:
                    scene.pushLog(
                        "Switched to Pool"
                    );
                    break;

                case AllocatorType::Tracking:
                    scene.pushLog(
                        "Switched to Tracking"
                    );
                    break;
                }
            }
        }

        // ==================================================
        // RESET
        // ==================================================

        if (input.action == SceneAction::Reset)
        {
            switch (currentAllocator) {

            case AllocatorType::Linear:
                linear.reset();
                break;

            case AllocatorType::Stack:
                stack.reset();
                stackAllocations.clear();
                break;

            case AllocatorType::Pool:
                for (auto& allocation : poolAllocations) {
                    pool.deallocate(
                        allocation.ptr
                    );
                }

                poolAllocations.clear();
                break;

            case AllocatorType::Tracking:
                for (auto& allocation :
                     trackingAllocations)
                {
                    tracking.deallocate(
                        static_cast<std::byte*>(
                            allocation.ptr
                        ),
                        allocation.size
                    );
                }

                trackingAllocations.clear();

                std::fill(
                    trackingUsed.begin(),
                    trackingUsed.end(),
                    false
                );

                break;
            }

            scene.onReset();

            scene.pushLog("RESET");
        }

        // ==================================================
        // ALLOCATION
        // ==================================================

        const int presetIndex =
            [&]() -> int
        {
            switch (input.action) {
            case SceneAction::Char:
                return 0;

            case SceneAction::Int:
                return 1;

            case SceneAction::Double:
                return 2;

            case SceneAction::Circle:
                return 3;

            case SceneAction::Square:
                return 4;

            default:
                return -1;
            }
        }();

        if (presetIndex >= 0)
        {
            const Preset& preset =
                PRESETS[presetIndex];

            // ==================================================
            // LINEAR
            // ==================================================

            if (currentAllocator ==
                AllocatorType::Linear)
            {
                const size_t before =
                    linear.used();

                void* ptr =
                    linear.allocate(
                        preset.size,
                        preset.align
                    );

                if (ptr) {

                    const size_t start =
                        linear.offset_of(ptr);

                    scene.onAllocate(
                        start,
                        preset.size,
                        start - before,
                        preset.color
                    );

                    scene.pushLog(
                        TextFormat(
                            "ALLOC %s  %zuB  pad=%zu",
                            preset.name,
                            preset.size,
                            start - before
                        )
                    );

                } else {
                    scene.pushLog(
                        TextFormat(
                            "ALLOC %s FAILED",
                            preset.name
                        )
                    );
                }
            }

            // ==================================================
            // STACK
            // ==================================================

            else if (
                currentAllocator ==
                AllocatorType::Stack)
            {
                const size_t before =
                    stack.used();

                const auto marker =
                    stack.get_marker();

                void* ptr =
                    stack.allocate(
                        preset.size,
                        preset.align
                    );

                if (ptr) {

                    const size_t start =
                        before;

                    stackAllocations.push_back({
                        ptr,
                        start,
                        preset.size,
                        marker
                    });

                    scene.onAllocate(
                        start,
                        preset.size,
                        start - before,
                        preset.color
                    );

                    scene.pushLog(
                        TextFormat(
                            "PUSH %s  %zuB",
                            preset.name,
                            preset.size
                        )
                    );

                } else {
                    scene.pushLog(
                        TextFormat(
                            "PUSH %s FAILED",
                            preset.name
                        )
                    );
                }
            }

            // ==================================================
            // POOL
            // ==================================================

            else if (
                currentAllocator ==
                AllocatorType::Pool)
            {
                if (preset.size >
                    POOL_CHUNK_SIZE)
                {
                    scene.pushLog(
                        TextFormat(
                            "%s too large for pool",
                            preset.name
                        )
                    );
                }
                else {

                    void* ptr =
                        pool.allocate();

                    if (ptr) {

                        const auto index =
                            static_cast<size_t>(
                                static_cast<std::byte*>(
                                    ptr
                                ) -
                                static_cast<std::byte*>(
                                    nullptr
                                )
                            );

                        // Pool memory isn't exposed by
                        // PoolAllocator, so visual slots
                        // are assigned separately.

                        size_t slot = 0;

                        bool found = false;

                        for (; slot < poolAllocations.size();
                             ++slot)
                        {
                            bool occupied = false;

                            for (const auto& a :
                                 poolAllocations)
                            {
                                if (a.start == slot) {
                                    occupied = true;
                                    break;
                                }
                            }

                            if (!occupied) {
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            slot = poolAllocations.size();
                        }

                        poolAllocations.push_back({
                            ptr,
                            slot,
                            preset.size,
                            0
                        });

                        scene.onAllocate(
                            slot,
                            1,
                            0,
                            preset.color
                        );

                        scene.pushLog(
                            TextFormat(
                                "POOL ALLOC %s",
                                preset.name
                            )
                        );
                    }
                    else {
                        scene.pushLog(
                            TextFormat(
                                "POOL %s FAILED",
                                preset.name
                            )
                        );
                    }
                }
            }

            // ==================================================
            // TRACKING
            // ==================================================

            else if (
                currentAllocator ==
                AllocatorType::Tracking)
            {
                std::byte* ptr =
                    tracking.allocate(
                        preset.size
                    );

                if (ptr) {

                    // Find a free visual range.
                    size_t start =
                        ARENA_SIZE;

                    for (
                        size_t i = 0;
                        i + preset.size <= ARENA_SIZE;
                        ++i
                    ) {
                        bool free = true;

                        for (
                            size_t j = 0;
                            j < preset.size;
                            ++j
                        ) {
                            if (trackingUsed[i + j]) {
                                free = false;
                                break;
                            }
                        }

                        if (free) {
                            start = i;
                            break;
                        }
                    }

                    if (start < ARENA_SIZE) {

                        for (
                            size_t i = 0;
                            i < preset.size;
                            ++i
                        ) {
                            trackingUsed[
                                start + i
                            ] = true;
                        }

                        trackingAllocations.push_back({
                            ptr,
                            start,
                            preset.size,
                            0
                        });

                        scene.onAllocate(
                            start,
                            preset.size,
                            0,
                            preset.color
                        );

                        scene.pushLog(
                            TextFormat(
                                "TRACK %s  %zuB",
                                preset.name,
                                preset.size
                            )
                        );
                    }
                    else {
                        tracking.deallocate(
                            ptr,
                            preset.size
                        );

                        scene.pushLog(
                            "Visual arena full"
                        );
                    }
                }
            }
        }

        // ==================================================
        // UPDATE
        // ==================================================

        scene.update(dt);

        // ==================================================
        // DRAW
        // ==================================================

        BeginDrawing();

        ClearBackground({
            10,
            13,
            18,
            255
        });

        BeginMode3D(camera);

        scene.draw3D();

        EndMode3D();

        int usedBytes = 0;
        int capacityBytes = 0;

        switch (currentAllocator) {

        case AllocatorType::Linear:
            usedBytes =
                static_cast<int>(
                    linear.used()
                );

            capacityBytes =
                static_cast<int>(
                    linear.capacity()
                );
            break;

        case AllocatorType::Stack:
            usedBytes =
                static_cast<int>(
                    stack.used()
                );

            capacityBytes =
                static_cast<int>(
                    stack.capacity()
                );
            break;

        case AllocatorType::Pool:
            usedBytes =
                static_cast<int>(
                    poolAllocations.size() *
                    POOL_CHUNK_SIZE
                );

            capacityBytes =
                static_cast<int>(
                    POOL_CHUNK_COUNT *
                    POOL_CHUNK_SIZE
                );
            break;

        case AllocatorType::Tracking:
            usedBytes =
                static_cast<int>(
                    tracking.getLiveBytes()
                );

            capacityBytes = 0;
            break;
        }

        scene.drawHUD(
            currentAllocator,
            usedBytes,
            capacityBytes
        );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}