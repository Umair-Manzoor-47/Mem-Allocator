#include <raylib.h>

#include <memory/AllocatorStats.h>
#include <memory/LinearAllocator.h>
#include <memory/PoolAllocator.h>
#include <memory/StackAllocator.h>
#include <memory/TrackingAllocator.h>

#include <new>

#include "memory_scene.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
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
    void* ptr{};
    size_t start{};
    size_t size{};
    size_t marker{};
};

namespace
{
    constexpr size_t ARENA_SIZE = 256;
    constexpr size_t POOL_CHUNK_SIZE = 16;
    constexpr size_t POOL_CHUNK_COUNT = 16;

    int presetIndex(SceneAction action)
    {
        switch (action) {
        case SceneAction::Char:   return 0;
        case SceneAction::Int:    return 1;
        case SceneAction::Double: return 2;
        case SceneAction::Circle: return 3;
        case SceneAction::Square: return 4;
        default:                  return -1;
        }
    }

    const char* allocatorName(AllocatorType allocator)
    {
        switch (allocator) {
        case AllocatorType::Linear:   return "Linear";
        case AllocatorType::Stack:    return "Stack";
        case AllocatorType::Pool:     return "Pool";
        case AllocatorType::Tracking: return "Tracking";
        }

        return "Unknown";
    }

    const char* allocatorConcept(AllocatorType allocator)
    {
        switch (allocator) {
        case AllocatorType::Linear:
            return "Bump pointer: allocations move forward; only reset releases memory.";

        case AllocatorType::Stack:
            return "LIFO storage: the top can pop, or rewind to a saved marker.";

        case AllocatorType::Pool:
            return "Fixed-size slots: allocation takes one free slot from a free list.";

        case AllocatorType::Tracking:
            return "General heap allocation with live-byte statistics; map below is visual.";
        }

        return "";
    }

    void clearVisualTracking(
        std::vector<bool>& used,
        const Allocation& allocation
    )
    {
        const size_t end =
            std::min(used.size(), allocation.start + allocation.size);

        for (size_t i = allocation.start; i < end; ++i) {
            used[i] = false;
        }
    }
}

int main()
{
    InitWindow(1100, 700, "Memory Lab");
    SetTargetFPS(60);

    Camera3D camera{};
    camera.position = {7.5f, 9.0f, 7.5f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    LinearAllocator linear(ARENA_SIZE);
    StackAllocator stack(ARENA_SIZE);

    PoolAllocator pool(
        POOL_CHUNK_SIZE,
        POOL_CHUNK_COUNT
    );

    TrackingAllocator<std::byte> tracking;

    MemoryScene scene(
        ARENA_SIZE,
        16,
        0.45f
    );

    AllocatorType currentAllocator = AllocatorType::Linear;

    std::vector<Allocation> linearAllocations;
    std::vector<Allocation> stackAllocations;
    std::vector<Allocation> poolAllocations;
    std::vector<Allocation> trackingAllocations;

    std::vector<bool> trackingUsed(
        ARENA_SIZE,
        false
    );

    bool stackMarkerValid = false;
    StackAllocator::Marker stackMarker = 0;

    scene.pushLog("Linear ready: bump pointer starts at 0.");
    scene.setCursor(0);

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        const SceneInput input =
            scene.handleInput(currentAllocator);

        // --------------------------------------------------
        // Allocator selection
        // --------------------------------------------------

        if (input.action == SceneAction::None &&
            input.allocator != currentAllocator)
        {
            currentAllocator = input.allocator;

            scene.onReset();
            scene.clearMarker();
            stackMarkerValid = false;
            linearAllocations.clear();
            stackAllocations.clear();

            // Release every demo allocation before changing the view.
            for (auto& allocation : poolAllocations) {
                pool.deallocate(allocation.ptr);
            }

            for (auto& allocation : trackingAllocations) {
                tracking.deallocate(
                    static_cast<std::byte*>(allocation.ptr),
                    allocation.size
                );
            }

            poolAllocations.clear();
            trackingAllocations.clear();
            linear.reset();
            stack.reset();

            std::fill(
                trackingUsed.begin(),
                trackingUsed.end(),
                false
            );
            AllocatorStats::instance().reset();

            scene.pushLog(
                std::string("Switched to ") + allocatorName(currentAllocator)
            );

            if (currentAllocator == AllocatorType::Linear) {
                scene.setCursor(linear.used());
            }
            else if (currentAllocator == AllocatorType::Stack) {
                scene.setCursor(stack.used());
            }

            continue;
        }

        // --------------------------------------------------
        // RESET
        // --------------------------------------------------

        if (input.action == SceneAction::Reset) {
            switch (currentAllocator) {
            case AllocatorType::Linear:
                linear.reset();
                linearAllocations.clear();
                break;

            case AllocatorType::Stack:
                stack.reset();
                stackAllocations.clear();
                stackMarkerValid = false;
                scene.clearMarker();
                break;

            case AllocatorType::Pool:
                for (auto& allocation : poolAllocations) {
                    pool.deallocate(allocation.ptr);
                }
                poolAllocations.clear();
                break;

            case AllocatorType::Tracking:
                for (auto& allocation : trackingAllocations) {
                    tracking.deallocate(
                        static_cast<std::byte*>(allocation.ptr),
                        allocation.size
                    );
                }
                trackingAllocations.clear();
                std::fill(
                    trackingUsed.begin(),
                    trackingUsed.end(),
                    false
                );
                AllocatorStats::instance().reset();
                break;
            }

            scene.onReset();
            scene.pushLog(
                std::string("RESET ") + allocatorName(currentAllocator)
            );
        }

        // --------------------------------------------------
        // DEALLOCATE / POP / MARKER
        // --------------------------------------------------

        if (input.action == SceneAction::Deallocate) {
            switch (currentAllocator) {
            case AllocatorType::Linear:
                // Disabled in the UI: linear allocator has no individual free.
                scene.pushLog("Linear: individual deallocate is not supported.");
                break;

            case AllocatorType::Stack:
                // Stack uses POP instead of generic deallocate.
                scene.pushLog("Stack: use POP or FREE TO MARKER.");
                break;

            case AllocatorType::Pool:
                if (poolAllocations.empty()) {
                    scene.pushLog("Pool deallocate: no live slots.");
                    break;
                }

                {
                    const Allocation allocation =
                        poolAllocations.back();

                    pool.deallocate(allocation.ptr);
                    poolAllocations.pop_back();

                    scene.onFreeRange(
                        allocation.start * POOL_CHUNK_SIZE,
                        POOL_CHUNK_SIZE
                    );

                    scene.pushLog(
                        "POOL FREE slot " +
                        std::to_string(allocation.start) +
                        " (16B returned to free list)"
                    );
                }
                break;

            case AllocatorType::Tracking:
                if (trackingAllocations.empty()) {
                    scene.pushLog("Tracking deallocate: no live allocations.");
                    break;
                }

                {
                    const Allocation allocation =
                        trackingAllocations.back();

                    tracking.deallocate(
                        static_cast<std::byte*>(allocation.ptr),
                        allocation.size
                    );

                    clearVisualTracking(
                        trackingUsed,
                        allocation
                    );

                    trackingAllocations.pop_back();
                    scene.onFreeRange(
                        allocation.start,
                        allocation.size
                    );

                    scene.pushLog(
                        "TRACK FREE " +
                        std::to_string(allocation.size) +
                        "B at " +
                        std::to_string(allocation.start)
                    );
                }
                break;
            }
        }

        if (input.action == SceneAction::Pop) {
            if (stackAllocations.empty()) {
                scene.pushLog("Stack POP: stack is empty.");
            }
            else {
                const Allocation allocation =
                    stackAllocations.back();

                const size_t oldUsed = stack.used();

                stack.free_to_marker(
                    allocation.marker
                );

                stackAllocations.pop_back();

                scene.onFreeRange(
                    allocation.marker,
                    oldUsed - allocation.marker
                );

                scene.setCursor(stack.used());

                scene.pushLog(
                    "POP -> " +
                    std::to_string(stack.used()) +
                    "B used"
                );

                if (stackMarkerValid &&
                    stackMarker > stack.used())
                {
                    stackMarkerValid = false;
                    scene.clearMarker();
                }
            }
        }

        if (input.action == SceneAction::SetMarker &&
            currentAllocator == AllocatorType::Stack)
        {
            stackMarker = stack.get_marker();
            stackMarkerValid = true;
            scene.setMarker(stackMarker);

            scene.pushLog(
                "MARKER set at " +
                std::to_string(stackMarker) +
                "B"
            );
        }

        if (input.action == SceneAction::FreeMarker &&
            currentAllocator == AllocatorType::Stack)
        {
            if (!stackMarkerValid) {
                scene.pushLog("No stack marker is set.");
            }
            else if (stackMarker > stack.used()) {
                scene.pushLog("Marker is beyond current stack; ignored.");
                stackMarkerValid = false;
                scene.clearMarker();
            }
            else {
                const size_t oldUsed = stack.used();

                stack.free_to_marker(stackMarker);

                while (!stackAllocations.empty() &&
                       stackAllocations.back().marker >= stackMarker)
                {
                    stackAllocations.pop_back();
                }

                scene.onFreeRange(
                    stackMarker,
                    oldUsed - stackMarker
                );

                scene.setCursor(stack.used());
                scene.pushLog(
                    "FREE TO MARKER -> " +
                    std::to_string(stackMarker) +
                    "B used"
                );

                stackMarkerValid = false;
                scene.clearMarker();
            }
        }

        // --------------------------------------------------
        // ALLOCATION
        // --------------------------------------------------

        const int index = presetIndex(input.action);

        if (index >= 0) {
            const Preset& preset = PRESETS[index];

            switch (currentAllocator) {
            case AllocatorType::Linear:
            {
                const size_t before = linear.used();

                void* ptr =
                    linear.allocate(
                        preset.size,
                        preset.align
                    );

                if (!ptr) {
                    scene.pushLog(
                        std::string("ALLOC ") +
                        preset.name +
                        " FAILED: not enough aligned space."
                    );
                    break;
                }

                const size_t start =
                    linear.offset_of(ptr);

                const size_t padding =
                    start - before;

                scene.onAllocate(
                    start,
                    preset.size,
                    padding,
                    preset.color,
                    std::string(preset.name) +
                    " " +
                    std::to_string(preset.size) +
                    "B"
                );

                linearAllocations.push_back({
                    ptr,
                    start,
                    preset.size,
                    before
                });

                scene.setCursor(linear.used());

                scene.pushLog(
                    "ALLOC " +
                    std::string(preset.name) +
                    " " +
                    std::to_string(preset.size) +
                    "B, pad " +
                    std::to_string(padding) +
                    "B"
                );
                break;
            }

            case AllocatorType::Stack:
            {
                const size_t before = stack.used();
                const auto marker = stack.get_marker();

                void* ptr =
                    stack.allocate(
                        preset.size,
                        preset.align
                    );

                if (!ptr) {
                    scene.pushLog(
                        std::string("PUSH ") +
                        preset.name +
                        " FAILED: stack full."
                    );
                    break;
                }

                // The allocator's real pointer is deliberately not used
                // for visualization. Its logical offset is deterministic.
                const size_t logicalStart =
                    stack.used() - preset.size;

                const size_t padding =
                    logicalStart - before;

                stackAllocations.push_back({
                    ptr,
                    logicalStart,
                    preset.size,
                    marker
                });

                scene.onAllocate(
                    logicalStart,
                    preset.size,
                    padding,
                    preset.color,
                    std::string(preset.name) +
                    " " +
                    std::to_string(preset.size) +
                    "B"
                );

                scene.setCursor(stack.used());

                scene.pushLog(
                    "PUSH " +
                    std::string(preset.name) +
                    " " +
                    std::to_string(preset.size) +
                    "B, pad " +
                    std::to_string(padding) +
                    "B"
                );

                break;
            }

            case AllocatorType::Pool:
            {
                if (preset.size > POOL_CHUNK_SIZE) {
                    scene.pushLog(
                        std::string(preset.name) +
                        " does not fit in a 16B pool slot."
                    );
                    break;
                }

                void* ptr = pool.allocate();

                if (!ptr) {
                    scene.pushLog("POOL ALLOC FAILED: all slots are busy.");
                    break;
                }

                const size_t slot =
                    pool.slot_index(ptr);

                poolAllocations.push_back({
                    ptr,
                    slot,
                    preset.size,
                    0
                });

                scene.onAllocate(
                    slot * POOL_CHUNK_SIZE,
                    POOL_CHUNK_SIZE,
                    0,
                    preset.color,
                    "slot " +
                    std::to_string(slot)
                );

                scene.pushLog(
                    "POOL ALLOC slot " +
                    std::to_string(slot) +
                    " <- " +
                    preset.name +
                    " (" +
                    std::to_string(preset.size) +
                    "B / 16B)"
                );
                break;
            }

            case AllocatorType::Tracking:
            {
                std::byte* ptr = nullptr;

                try {
                    ptr = tracking.allocate(preset.size);
                }
                catch (const std::bad_alloc&) {
                    scene.pushLog(
                        "TRACK ALLOC FAILED: system heap allocation failed."
                    );
                    break;
                }

                // TrackingAllocator uses the real heap. The visual arena is
                // only a deterministic map so fragmentation can be taught.
                size_t start = ARENA_SIZE;

                for (
                    size_t i = 0;
                    i + preset.size <= ARENA_SIZE;
                    ++i
                ) {
                    bool free = true;

                    for (size_t j = 0; j < preset.size; ++j) {
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

                if (start == ARENA_SIZE) {
                    tracking.deallocate(ptr, preset.size);
                    scene.pushLog(
                        "TRACK: visual map is full; real allocation rolled back."
                    );
                    break;
                }

                for (size_t i = 0; i < preset.size; ++i) {
                    trackingUsed[start + i] = true;
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
                    preset.color,
                    std::string(preset.name) +
                    " " +
                    std::to_string(preset.size) +
                    "B"
                );

                scene.pushLog(
                    "TRACK ALLOC " +
                    std::string(preset.name) +
                    " " +
                    std::to_string(preset.size) +
                    "B at visual offset " +
                    std::to_string(start)
                );
                break;
            }
            }
        }

        // Keep the cursor synchronized even after actions.
        if (currentAllocator == AllocatorType::Linear) {
            scene.setCursor(linear.used());
        }
        else if (currentAllocator == AllocatorType::Stack) {
            scene.setCursor(stack.used());
        }

        scene.update(dt);

        // --------------------------------------------------
        // Build HUD data
        // --------------------------------------------------

        MemoryView view{};
        view.allocator = currentAllocator;
        view.description = allocatorConcept(currentAllocator);
        view.visualCapacityBytes = ARENA_SIZE;

        switch (currentAllocator) {
        case AllocatorType::Linear:
            view.usedBytes = linear.used();
            view.capacityBytes = linear.capacity();
            view.allocationCount = linearAllocations.size();
            view.cursorBytes = linear.used();
            view.status =
                "Individual free is impossible: reset rewinds the whole arena.";
            break;

        case AllocatorType::Stack:
            view.usedBytes = stack.used();
            view.capacityBytes = stack.capacity();
            view.allocationCount = stackAllocations.size();
            view.cursorBytes = stack.used();
            view.markerBytes = stackMarker;
            view.markerValid = stackMarkerValid;
            view.status =
                stackMarkerValid
                    ? "Yellow marker = saved rewind point."
                    : "Set a marker before FREE TO MARKER.";
            break;

        case AllocatorType::Pool:
            view.usedBytes =
                poolAllocations.size() * POOL_CHUNK_SIZE;
            view.capacityBytes =
                pool.chunk_count() * pool.chunk_size();
            view.allocationCount = poolAllocations.size();
            view.poolSlotsUsed = poolAllocations.size();
            view.poolSlotsTotal = pool.chunk_count();
            view.status =
                "Each allocation consumes exactly one fixed-size slot.";
            break;

        case AllocatorType::Tracking:
            view.usedBytes = tracking.getLiveBytes();
            view.capacityBytes = 0;
            view.allocationCount = trackingAllocations.size();
            view.totalAllocated =
                AllocatorStats::instance().getTotalAllocated();
            view.totalDeallocated =
                AllocatorStats::instance().getTotalDeallocated();
            view.visualCapacityBytes = ARENA_SIZE;
            for (const auto& allocation : trackingAllocations) {
                view.visualUsedBytes += allocation.size;
            }
            view.status =
                "The 256B map is visual only; the allocator uses the real heap.";
            break;
        }

        BeginDrawing();

        ClearBackground({
            10,
            13,
            18,
            255
        });

        BeginMode3D(camera);
        scene.draw3D(camera);
        EndMode3D();

        scene.drawHUD(view);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}