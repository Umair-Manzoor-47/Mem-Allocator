#pragma once

#include <raylib.h>

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

struct MemoryBlock {
    size_t start{};
    size_t size{};
    Color color{};
    std::string label;

    float scale = 0.0f;
    float targetScale = 1.0f;
    bool dying = false;
};

enum class AllocatorType {
    Linear,
    Stack,
    Pool,
    Tracking
};

enum class SceneAction {
    None,

    Char,
    Int,
    Double,
    Circle,
    Square,

    Reset,
    Deallocate,
    Pop,
    SetMarker,
    FreeMarker
};

struct SceneInput {
    AllocatorType allocator;
    SceneAction action;
};

struct MemoryView {
    AllocatorType allocator{};
    const char* description = "";

    size_t usedBytes = 0;
    size_t capacityBytes = 0;
    size_t allocationCount = 0;

    size_t cursorBytes = 0;
    size_t markerBytes = 0;
    bool markerValid = false;

    size_t poolSlotsUsed = 0;
    size_t poolSlotsTotal = 0;
    size_t visualUsedBytes = 0;
    size_t visualCapacityBytes = 0;

    size_t totalAllocated = 0;
    size_t totalDeallocated = 0;
    const char* status = "";
};

class MemoryScene {
public:
    MemoryScene(
        size_t arenaSize,
        int cellsPerRow = 16,
        float cellSize = 0.45f
    );

    void onAllocate(
        size_t start,
        size_t size,
        size_t padding,
        Color color,
        const std::string& label
    );

    void onFreeRange(size_t start, size_t size);
    void onReset();

    void setCursor(size_t offset);
    void setMarker(size_t offset);
    void clearMarker();

    void pushLog(const std::string& line);

    void update(float dt);
    void draw3D(const Camera3D& camera) const;

    SceneInput handleInput(AllocatorType currentAllocator);
    void drawHUD(const MemoryView& view) const;

private:
    size_t m_arenaSize;
    int m_cellsPerRow;
    float m_cellSize;

    std::vector<MemoryBlock> m_blocks;
    std::deque<std::string> m_log;

    size_t m_cursor = 0;
    size_t m_marker = 0;
    bool m_markerValid = false;

    Vector3 cellPos(size_t byteIndex) const;
    Rectangle allocatorButton(int index) const;
    Rectangle presetButton(int index) const;
    Rectangle actionButton(int index, int count) const;

    int actionCount(AllocatorType allocator) const;
    bool buttonPressed(Rectangle bounds) const;

    void drawButton(
        Rectangle bounds,
        const char* text,
        Color color,
        bool selected = false,
        bool enabled = true
    ) const;

    void drawMetric(
        int x,
        int y,
        const char* label,
        const char* value
    ) const;

    static const char* allocatorName(AllocatorType allocator);
};