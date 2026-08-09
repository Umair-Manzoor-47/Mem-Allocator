#pragma once

#include <raylib.h>

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

struct MemoryBlock {
    size_t start;
    size_t size;
    Color color;

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

    // Allocation buttons
    Char,
    Int,
    Double,
    Circle,
    Square,

    // Allocator actions
    Reset,
    Pop,
    Free
};

struct SceneInput {
    AllocatorType allocator;
    SceneAction action;
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
        Color color
    );

    void onFreeRange(size_t start, size_t size);

    void onReset();

    void pushLog(const std::string& line);

    void update(float dt);

    // Called inside BeginMode3D / EndMode3D.
    void draw3D() const;

    // Handles mouse input for the UI.
    SceneInput handleInput(AllocatorType currentAllocator);

    // Called outside BeginMode3D / EndMode3D.
    void drawHUD(
        AllocatorType allocator,
        int usedBytes,
        int capacityBytes
    ) const;

private:
    size_t m_arenaSize;
    int m_cellsPerRow;
    float m_cellSize;

    std::vector<MemoryBlock> m_blocks;
    std::deque<std::string> m_log;

    Vector3 cellPos(size_t byteIndex) const;

    float arenaWidth() const;

    Rectangle allocatorButton(
        AllocatorType type,
        int index
    ) const;

    Rectangle actionButton(int index) const;

    bool buttonPressed(Rectangle bounds) const;

    void drawButton(
        Rectangle bounds,
        const char* text,
        Color color,
        bool selected = false
    ) const;
};