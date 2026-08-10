#include "memory_scene.h"

#include <algorithm>
#include <cstdio>

#include "raymath.h"

namespace
{
    constexpr int TOP_BAR_HEIGHT = 52;
    constexpr int BOTTOM_BAR_HEIGHT = 92;
    constexpr int BUTTON_HEIGHT = 28;
    constexpr int BUTTON_GAP = 6;
    constexpr int SIDE_MARGIN = 14;

    constexpr Color CHAR_COLOR   = {94, 234, 212, 255};
    constexpr Color INT_COLOR    = {130, 170, 255, 255};
    constexpr Color DOUBLE_COLOR = {198, 160, 246, 255};
    constexpr Color CIRCLE_COLOR = {247, 140, 108, 255};
    constexpr Color SQUARE_COLOR = {166, 227, 161, 255};

    constexpr Color RESET_COLOR = {220, 90, 90, 255};
    constexpr Color ACTION_COLOR = {100, 150, 220, 255};
    constexpr Color MARKER_COLOR = {255, 205, 80, 255};

    constexpr Color PANEL = {9, 13, 20, 238};
    constexpr Color PANEL_LINE = {255, 255, 255, 35};
}

MemoryScene::MemoryScene(size_t arenaSize, int cellsPerRow, float cellSize)
    : m_arenaSize(arenaSize),
      m_cellsPerRow(cellsPerRow),
      m_cellSize(cellSize)
{
}

Vector3 MemoryScene::cellPos(size_t byteIndex) const
{
    const int row =
        static_cast<int>(byteIndex) / m_cellsPerRow;

    const int col =
        static_cast<int>(byteIndex) % m_cellsPerRow;

    const float width = m_cellsPerRow * m_cellSize;

    return {
        col * m_cellSize - width * 0.5f + m_cellSize * 0.5f,
        0.0f,
        row * m_cellSize - width * 0.5f + m_cellSize * 0.5f
    };
}

void MemoryScene::onAllocate(
    size_t start,
    size_t size,
    size_t padding,
    Color color,
    const std::string& label
)
{
    if (padding > 0 && start >= padding) {
        m_blocks.push_back({
            start - padding,
            padding,
            Fade(ORANGE, 0.55f),
            "PAD"
        });
    }

    m_blocks.push_back({
        start,
        size,
        color,
        label
    });
}

void MemoryScene::onFreeRange(size_t start, size_t size)
{
    if (size == 0) {
        return;
    }

    const size_t end = std::min(m_arenaSize, start + size);

    for (auto& block : m_blocks) {
        const size_t blockEnd = block.start + block.size;

        if (block.start < end && blockEnd > start) {
            block.dying = true;
            block.targetScale = 0.0f;
        }
    }
}

void MemoryScene::onReset()
{
    for (auto& block : m_blocks) {
        block.dying = true;
        block.targetScale = 0.0f;
    }

    m_cursor = 0;
    clearMarker();
}

void MemoryScene::setCursor(size_t offset)
{
    m_cursor = std::min(offset, m_arenaSize);
}

void MemoryScene::setMarker(size_t offset)
{
    m_marker = std::min(offset, m_arenaSize);
    m_markerValid = true;
}

void MemoryScene::clearMarker()
{
    m_marker = 0;
    m_markerValid = false;
}

void MemoryScene::pushLog(const std::string& line)
{
    m_log.push_front(line);

    constexpr size_t MAX_LOG = 8;
    if (m_log.size() > MAX_LOG) {
        m_log.pop_back();
    }
}

void MemoryScene::update(float dt)
{
    const float speed = std::min(1.0f, dt * 12.0f);

    for (auto& block : m_blocks) {
        block.scale = Lerp(
            block.scale,
            block.targetScale,
            speed
        );
    }

    m_blocks.erase(
        std::remove_if(
            m_blocks.begin(),
            m_blocks.end(),
            [](const MemoryBlock& block)
            {
                return block.dying && block.scale < 0.02f;
            }
        ),
        m_blocks.end()
    );
}

void MemoryScene::draw3D(const Camera3D& camera) const
{
    constexpr float GRID_HEIGHT = 0.04f;
    constexpr float CELL_GAP = 0.06f;
    constexpr float BLOCK_HEIGHT = 0.48f;

    const float cellSize = m_cellSize - CELL_GAP;

    for (size_t i = 0; i < m_arenaSize; ++i) {
        const Vector3 p = cellPos(i);

        DrawCube(
            {p.x, GRID_HEIGHT * 0.5f, p.z},
            cellSize,
            GRID_HEIGHT,
            cellSize,
            Fade(DARKGRAY, 0.28f)
        );

        DrawCubeWires(
            {p.x, GRID_HEIGHT * 0.5f, p.z},
            cellSize,
            GRID_HEIGHT,
            cellSize,
            Fade(LIGHTGRAY, 0.20f)
        );
    }

    // Allocation blocks.
    for (const auto& block : m_blocks) {
        if (block.scale < 0.01f || block.size == 0) {
            continue;
        }

        for (
            size_t i = block.start;
            i < block.start + block.size && i < m_arenaSize;
            ++i
        ) {
            const Vector3 p = cellPos(i);
            const float height = BLOCK_HEIGHT * block.scale;

            DrawCube(
                {p.x, GRID_HEIGHT + height * 0.5f, p.z},
                cellSize,
                height,
                cellSize,
                block.color
            );

            DrawCubeWires(
                {p.x, GRID_HEIGHT + height * 0.5f, p.z},
                cellSize,
                height,
                cellSize,
                Fade(BLACK, 0.45f)
            );
        }
    }

    // Cursor: first byte that will be used by linear/stack.
    if (m_cursor < m_arenaSize) {
        const Vector3 p = cellPos(m_cursor);

        DrawCube(
            {p.x, 0.56f, p.z},
            cellSize * 0.22f,
            0.14f,
            cellSize * 0.22f,
            SKYBLUE
        );
    }

    // Stack marker.
    if (m_markerValid && m_marker < m_arenaSize) {
        const Vector3 p = cellPos(m_marker);

        DrawCube(
            {p.x, 0.72f, p.z},
            cellSize * 0.78f,
            0.08f,
            cellSize * 0.78f,
            MARKER_COLOR
        );
    }

    const float width = m_cellsPerRow * m_cellSize;

    DrawCubeWires(
        {0.0f, 0.03f, 0.0f},
        width,
        0.06f,
        width,
        Fade(WHITE, 0.30f)
    );
}

Rectangle MemoryScene::allocatorButton(int index) const
{
    constexpr int width = 108;
    const int totalWidth = width * 4 + BUTTON_GAP * 3;
    const int startX = (GetScreenWidth() - totalWidth) / 2;

    return {
        static_cast<float>(startX + index * (width + BUTTON_GAP)),
        20.0f,
        static_cast<float>(width),
        static_cast<float>(BUTTON_HEIGHT)
    };
}

Rectangle MemoryScene::presetButton(int index) const
{
    constexpr int width = 96;
    constexpr int count = 5;

    const int totalWidth = width * count + BUTTON_GAP * (count - 1);
    const int startX = (GetScreenWidth() - totalWidth) / 2;

    return {
        static_cast<float>(startX + index * (width + BUTTON_GAP)),
        static_cast<float>(GetScreenHeight() - BOTTOM_BAR_HEIGHT + 22),
        static_cast<float>(width),
        static_cast<float>(BUTTON_HEIGHT)
    };
}

Rectangle MemoryScene::actionButton(int index, int count) const
{
    constexpr int width = 104;

    const int totalWidth = width * count + BUTTON_GAP * (count - 1);
    const int startX = (GetScreenWidth() - totalWidth) / 2;

    return {
        static_cast<float>(startX + index * (width + BUTTON_GAP)),
        static_cast<float>(GetScreenHeight() - 34),
        static_cast<float>(width),
        static_cast<float>(BUTTON_HEIGHT)
    };
}

int MemoryScene::actionCount(AllocatorType allocator) const
{
    switch (allocator) {
    case AllocatorType::Linear:
        return 2; // Reset + Deallocate (disabled)
    case AllocatorType::Stack:
        return 4; // Pop + marker controls + reset
    case AllocatorType::Pool:
        return 2; // Deallocate + reset
    case AllocatorType::Tracking:
        return 2; // Deallocate + reset
    }

    return 0;
}

bool MemoryScene::buttonPressed(Rectangle bounds) const
{
    return CheckCollisionPointRec(
               GetMousePosition(),
               bounds
           ) &&
           IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void MemoryScene::drawButton(
    Rectangle bounds,
    const char* text,
    Color color,
    bool selected,
    bool enabled
) const
{
    const bool hovered =
        enabled &&
        CheckCollisionPointRec(
            GetMousePosition(),
            bounds
        );

    Color background =
        !enabled
            ? Fade(GRAY, 0.10f)
            : selected
                ? Fade(color, 0.85f)
                : hovered
                    ? Fade(color, 0.62f)
                    : Fade(color, 0.25f);

    DrawRectangleRounded(
        bounds,
        0.18f,
        8,
        background
    );

    DrawRectangleRoundedLines(
        bounds,
        0.18f,
        8,
        !enabled
            ? Fade(GRAY, 0.25f)
            : hovered || selected
                ? color
                : Fade(color, 0.50f)
    );

    const int fontSize = 12;
    const int textWidth = MeasureText(text, fontSize);

    DrawText(
        text,
        static_cast<int>(bounds.x + (bounds.width - textWidth) * 0.5f),
        static_cast<int>(bounds.y + (bounds.height - fontSize) * 0.5f),
        fontSize,
        enabled ? RAYWHITE : Fade(RAYWHITE, 0.35f)
    );
}

void MemoryScene::drawMetric(
    int x,
    int y,
    const char* label,
    const char* value
) const
{
    DrawText(label, x, y, 12, GRAY);
    DrawText(value, x, y + 17, 17, RAYWHITE);
}

SceneInput MemoryScene::handleInput(AllocatorType currentAllocator)
{
    SceneInput result{currentAllocator, SceneAction::None};

    for (int i = 0; i < 4; ++i) {
        if (buttonPressed(allocatorButton(i))) {
            result.allocator = static_cast<AllocatorType>(i);
            return result;
        }
    }

    const SceneAction presets[] = {
        SceneAction::Char,
        SceneAction::Int,
        SceneAction::Double,
        SceneAction::Circle,
        SceneAction::Square
    };

    for (int i = 0; i < 5; ++i) {
        if (buttonPressed(presetButton(i))) {
            result.action = presets[i];
            return result;
        }
    }

    const int count = actionCount(currentAllocator);

    for (int i = 0; i < count; ++i) {
        if (!buttonPressed(actionButton(i, count))) {
            continue;
        }

        switch (currentAllocator) {
        case AllocatorType::Linear:
            if (i == 0) result.action = SceneAction::Reset;
            // i == 1 is intentionally disabled.
            break;

        case AllocatorType::Stack:
            if (i == 0) result.action = SceneAction::Pop;
            if (i == 1) result.action = SceneAction::SetMarker;
            if (i == 2) result.action = SceneAction::FreeMarker;
            if (i == 3) result.action = SceneAction::Reset;
            break;

        case AllocatorType::Pool:
        case AllocatorType::Tracking:
            if (i == 0) result.action = SceneAction::Deallocate;
            if (i == 1) result.action = SceneAction::Reset;
            break;
        }

        return result;
    }

    return result;
}

const char* MemoryScene::allocatorName(AllocatorType allocator)
{
    switch (allocator) {
    case AllocatorType::Linear:   return "LINEAR";
    case AllocatorType::Stack:    return "STACK";
    case AllocatorType::Pool:     return "POOL";
    case AllocatorType::Tracking: return "TRACKING";
    }

    return "UNKNOWN";
}

void MemoryScene::drawHUD(const MemoryView& view) const
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();

    // Compact top allocator bar.
    DrawRectangle(0, 0, width, TOP_BAR_HEIGHT, Fade(BLACK, 0.88f));
    DrawText("ALLOCATOR", SIDE_MARGIN, 8, 10, GRAY);

    const char* names[] = {"LINEAR", "STACK", "POOL", "TRACKING"};
    const Color colors[] = {
        {94, 180, 255, 255},
        {170, 130, 255, 255},
        {100, 220, 170, 255},
        {255, 170, 100, 255}
    };

    for (int i = 0; i < 4; ++i) {
        drawButton(
            allocatorButton(i),
            names[i],
            colors[i],
            static_cast<int>(view.allocator) == i
        );
    }

    // Small allocator stats card: intentionally limited to the essentials.
    constexpr int panelX = 14;
    constexpr int panelY = 66;
    constexpr int panelW = 330;
    constexpr int panelH = 132;

    DrawRectangle(panelX, panelY, panelW, panelH, PANEL);
    DrawRectangleLines(panelX, panelY, panelW, panelH, PANEL_LINE);

    DrawText(allocatorName(view.allocator), panelX + 12, panelY + 10, 18, RAYWHITE);

    char value[64]{};
    if (view.capacityBytes > 0) {
        std::snprintf(value, sizeof(value), "%zu / %zu B", view.usedBytes, view.capacityBytes);
        drawMetric(panelX + 12, panelY + 38, "USED", value);
    } else {
        std::snprintf(value, sizeof(value), "%zu B", view.usedBytes);
        drawMetric(panelX + 12, panelY + 38, "LIVE", value);
    }

    std::snprintf(value, sizeof(value), "%zu", view.allocationCount);
    drawMetric(panelX + 115, panelY + 38, "ALLOCATIONS", value);

    if (view.cursorBytes > 0 ||
        view.allocator == AllocatorType::Linear ||
        view.allocator == AllocatorType::Stack)
    {
        std::snprintf(value, sizeof(value), "%zu B", view.cursorBytes);
        drawMetric(panelX + 230, panelY + 38, "CURSOR", value);
    }

    if (view.markerValid) {
        std::snprintf(value, sizeof(value), "%zu B", view.markerBytes);
        drawMetric(panelX + 12, panelY + 82, "MARKER", value);
    }
    else if (view.poolSlotsTotal > 0) {
        std::snprintf(value, sizeof(value), "%zu / %zu", view.poolSlotsUsed, view.poolSlotsTotal);
        drawMetric(panelX + 12, panelY + 82, "SLOTS", value);
    }
    else if (view.totalAllocated > 0 || view.totalDeallocated > 0) {
        std::snprintf(value, sizeof(value), "+%zu / -%zu B", view.totalAllocated, view.totalDeallocated);
        drawMetric(panelX + 12, panelY + 82, "TOTAL", value);
    }

    if (view.capacityBytes > 0) {
        const float usage = static_cast<float>(view.usedBytes) /
                            static_cast<float>(view.capacityBytes);
        constexpr int barX = panelX + 115;
        constexpr int barY = panelY + 91;
        constexpr int barW = 203;
        constexpr int barH = 8;

        DrawRectangle(barX, barY, barW, barH, Fade(WHITE, 0.10f));
        DrawRectangle(
            barX, barY,
            static_cast<int>(barW * std::clamp(usage, 0.0f, 1.0f)),
            barH,
            SKYBLUE
        );
    }

    // Docked log console: fixed top-right, separate from the stats card.
    constexpr int logW = 390;
    constexpr int logH = 154;
    const int logX = width - logW - 14;
    const int logY = 66;

    DrawRectangle(logX, logY, logW, logH, PANEL);
    DrawRectangleLines(logX, logY, logW, logH, PANEL_LINE);
    DrawText("LOG CONSOLE", logX + 12, logY + 10, 10, GRAY);

    int logTextY = logY + 31;
    for (const auto& line : m_log) {
        DrawText(line.c_str(), logX + 12, logTextY, 11, LIGHTGRAY);
        logTextY += 15;
        if (logTextY > logY + logH - 10) {
            break;
        }
    }

    // Bottom controls stay compact and contain only actions.
    const int bottomY = height - BOTTOM_BAR_HEIGHT;
    DrawRectangle(0, bottomY, width, BOTTOM_BAR_HEIGHT, Fade(BLACK, 0.90f));

    DrawText("ALLOCATE", SIDE_MARGIN, bottomY + 8, 10, GRAY);

    const char* presets[] = {
        "CHAR 1B", "INT 4B", "DOUBLE 8B", "CIRCLE 12B", "SQUARE 16B"
    };
    const Color presetColors[] = {
        CHAR_COLOR, INT_COLOR, DOUBLE_COLOR, CIRCLE_COLOR, SQUARE_COLOR
    };

    for (int i = 0; i < 5; ++i) {
        drawButton(presetButton(i), presets[i], presetColors[i]);
    }

    DrawText("ACTIONS", SIDE_MARGIN, height - 42, 10, GRAY);

    const int count = actionCount(view.allocator);
    const char* actions[4]{};
    Color actionColors[4]{};
    bool enabled[4]{true, true, true, true};

    switch (view.allocator) {
    case AllocatorType::Linear:
        actions[0] = "RESET";
        actions[1] = "DEALLOCATE N/A";
        actionColors[0] = RESET_COLOR;
        actionColors[1] = Fade(GRAY, 0.8f);
        enabled[1] = false;
        break;
    case AllocatorType::Stack:
        actions[0] = "POP";
        actions[1] = "SET MARKER";
        actions[2] = "FREE MARKER";
        actions[3] = "RESET";
        actionColors[0] = ACTION_COLOR;
        actionColors[1] = MARKER_COLOR;
        actionColors[2] = MARKER_COLOR;
        actionColors[3] = RESET_COLOR;
        break;
    case AllocatorType::Pool:
    case AllocatorType::Tracking:
        actions[0] = "DEALLOCATE";
        actions[1] = "RESET";
        actionColors[0] = ACTION_COLOR;
        actionColors[1] = RESET_COLOR;
        break;
    }

    for (int i = 0; i < count; ++i) {
        drawButton(actionButton(i, count), actions[i], actionColors[i], false, enabled[i]);
    }
}
