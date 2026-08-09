#include "memory_scene.h"

#include <raymath.h>

#include <algorithm>

namespace
{
    constexpr int BUTTON_HEIGHT = 38;
    constexpr int BUTTON_GAP = 8;

    constexpr Color CHAR_COLOR   = {94, 234, 212, 255};
    constexpr Color INT_COLOR    = {130, 170, 255, 255};
    constexpr Color DOUBLE_COLOR = {198, 160, 246, 255};
    constexpr Color CIRCLE_COLOR = {247, 140, 108, 255};
    constexpr Color SQUARE_COLOR = {166, 227, 161, 255};

    constexpr Color RESET_COLOR = {220, 90, 90, 255};
    constexpr Color ACTION_COLOR = {100, 150, 220, 255};
}

MemoryScene::MemoryScene(
    size_t arenaSize,
    int cellsPerRow,
    float cellSize
)
    : m_arenaSize(arenaSize),
      m_cellsPerRow(cellsPerRow),
      m_cellSize(cellSize)
{
}

float MemoryScene::arenaWidth() const
{
    return m_cellsPerRow * m_cellSize;
}

Vector3 MemoryScene::cellPos(size_t byteIndex) const
{
    const int row =
        static_cast<int>(byteIndex) / m_cellsPerRow;

    const int col =
        static_cast<int>(byteIndex) % m_cellsPerRow;

    const float width = arenaWidth();

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
    Color color
)
{
    if (padding > 0) {
        m_blocks.push_back({
            start - padding,
            padding,
            Fade(ORANGE, 0.65f)
        });
    }

    m_blocks.push_back({
        start,
        size,
        color
    });
}

void MemoryScene::onFreeRange(
    size_t start,
    size_t size
)
{
    const size_t end = start + size;

    for (auto& block : m_blocks) {
        const size_t blockEnd =
            block.start + block.size;

        const bool overlaps =
            block.start < end &&
            blockEnd > start;

        if (overlaps) {
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
}

void MemoryScene::pushLog(const std::string& line)
{
    m_log.push_front(line);

    constexpr size_t MAX_LOG = 7;

    if (m_log.size() > MAX_LOG) {
        m_log.pop_back();
    }
}

void MemoryScene::update(float dt)
{
    const float speed = dt * 10.0f;

    for (auto& block : m_blocks) {
        block.scale = Lerp(
            block.scale,
            block.targetScale,
            std::min(1.0f, speed)
        );
    }

    m_blocks.erase(
        std::remove_if(
            m_blocks.begin(),
            m_blocks.end(),
            [](const MemoryBlock& block)
            {
                return block.dying &&
                       block.scale < 0.02f;
            }
        ),
        m_blocks.end()
    );
}

void MemoryScene::draw3D() const
{
    constexpr float GRID_HEIGHT = 0.04f;
    constexpr float CELL_GAP = 0.06f;

    const float cellSize =
        m_cellSize - CELL_GAP;

    // --------------------------------------------------
    // Memory grid
    // --------------------------------------------------

    for (size_t i = 0; i < m_arenaSize; ++i) {
        const Vector3 p = cellPos(i);

        DrawCube(
            {
                p.x,
                GRID_HEIGHT * 0.5f,
                p.z
            },
            cellSize,
            GRID_HEIGHT,
            cellSize,
            Fade(DARKGRAY, 0.35f)
        );

        DrawCubeWires(
            {
                p.x,
                GRID_HEIGHT * 0.5f,
                p.z
            },
            cellSize,
            GRID_HEIGHT,
            cellSize,
            Fade(LIGHTGRAY, 0.30f)
        );
    }

    // --------------------------------------------------
    // Allocated memory
    // --------------------------------------------------

    for (const auto& block : m_blocks) {
        if (block.scale < 0.01f) {
            continue;
        }

        for (
            size_t i = block.start;
            i < block.start + block.size &&
            i < m_arenaSize;
            ++i
        ) {
            const Vector3 p = cellPos(i);

            const float height =
                block.scale;

            if (height < 0.01f) {
                continue;
            }

            const Vector3 center{
                p.x,
                GRID_HEIGHT + height * 0.5f,
                p.z
            };

            DrawCube(
                center,
                cellSize,
                height,
                cellSize,
                block.color
            );

            DrawCubeWires(
                center,
                cellSize,
                height,
                cellSize,
                Fade(BLACK, 0.45f)
            );
        }
    }

    // --------------------------------------------------
    // Arena border
    // --------------------------------------------------

    const float width = arenaWidth();

    DrawCubeWires(
        {
            0.0f,
            0.03f,
            0.0f
        },
        width,
        0.06f,
        width,
        Fade(WHITE, 0.35f)
    );
}

Rectangle MemoryScene::allocatorButton(
    AllocatorType,
    int index
) const
{
    constexpr int width = 135;

    const int totalWidth =
        width * 4 + BUTTON_GAP * 3;

    const int startX =
        (GetScreenWidth() - totalWidth) / 2;

    return {
        static_cast<float>(
            startX + index * (width + BUTTON_GAP)
        ),
        20.0f,
        static_cast<float>(width),
        static_cast<float>(BUTTON_HEIGHT)
    };
}

Rectangle MemoryScene::actionButton(int index) const
{
    constexpr int width = 115;

    const int totalWidth =
        width * 6 + BUTTON_GAP * 5;

    const int startX =
        (GetScreenWidth() - totalWidth) / 2;

    return {
        static_cast<float>(
            startX + index * (width + BUTTON_GAP)
        ),
        static_cast<float>(
            GetScreenHeight() - 58
        ),
        static_cast<float>(width),
        static_cast<float>(BUTTON_HEIGHT)
    };
}

bool MemoryScene::buttonPressed(Rectangle bounds) const
{
    return CheckCollisionPointRec(
               GetMousePosition(),
               bounds
           ) &&
           IsMouseButtonPressed(
               MOUSE_BUTTON_LEFT
           );
}

void MemoryScene::drawButton(
    Rectangle bounds,
    const char* text,
    Color color,
    bool selected
) const
{
    const bool hovered =
        CheckCollisionPointRec(
            GetMousePosition(),
            bounds
        );

    Color background =
        selected
            ? Fade(color, 0.85f)
            : hovered
                ? Fade(color, 0.65f)
                : Fade(color, 0.30f);

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
        hovered || selected
            ? color
            : Fade(color, 0.55f)
    );

    const int fontSize = 16;

    const int textWidth =
        MeasureText(text, fontSize);

    DrawText(
        text,
        static_cast<int>(
            bounds.x +
            (bounds.width - textWidth) * 0.5f
        ),
        static_cast<int>(
            bounds.y +
            (bounds.height - fontSize) * 0.5f
        ),
        fontSize,
        RAYWHITE
    );
}

SceneInput MemoryScene::handleInput(AllocatorType currentAllocator)
{
    SceneInput result{
        currentAllocator,
        SceneAction::None
    };

    // -----------------------------------------------
    // Allocator selector
    // -----------------------------------------------

    for (int i = 0; i < 4; ++i) {
        if (!buttonPressed(
                allocatorButton(
                    AllocatorType::Linear,
                    i
                )))
        {
            continue;
        }

        result.allocator =
            static_cast<AllocatorType>(i);

        return result;
    }

    // -----------------------------------------------
    // Action buttons
    // -----------------------------------------------

    for (int i = 0; i < 6; ++i) {
        if (!buttonPressed(actionButton(i))) {
            continue;
        }

        switch (i) {
        case 0:
            result.action = SceneAction::Char;
            break;

        case 1:
            result.action = SceneAction::Int;
            break;

        case 2:
            result.action = SceneAction::Double;
            break;

        case 3:
            result.action = SceneAction::Circle;
            break;

        case 4:
            result.action = SceneAction::Square;
            break;

        case 5:
            result.action = SceneAction::Reset;
            break;
        }

        return result;
    }

    return result;
}

void MemoryScene::drawHUD(
    AllocatorType allocator,
    int usedBytes,
    int capacityBytes
) const
{
    // --------------------------------------------------
    // Top allocator selector
    // --------------------------------------------------

    DrawRectangle(
        0,
        0,
        GetScreenWidth(),
        76,
        Fade(BLACK, 0.88f)
    );

    DrawText(
        "ALLOCATOR",
        20,
        31,
        16,
        GRAY
    );

    const char* names[] = {
        "LINEAR",
        "STACK",
        "POOL",
        "TRACKING"
    };

    const Color colors[] = {
        {94, 180, 255, 255},
        {170, 130, 255, 255},
        {100, 220, 170, 255},
        {255, 170, 100, 255}
    };

    for (int i = 0; i < 4; ++i) {
        drawButton(
            allocatorButton(allocator, i),
            names[i],
            colors[i],
            static_cast<int>(allocator) == i
        );
    }

    // --------------------------------------------------
    // Info panel
    // --------------------------------------------------

    const int panelX = 16;
    const int panelY = 96;
    const int panelWidth = 390;
    const int panelHeight = 190;

    DrawRectangle(
        panelX,
        panelY,
        panelWidth,
        panelHeight,
        Fade(BLACK, 0.78f)
    );

    DrawRectangleLines(
        panelX,
        panelY,
        panelWidth,
        panelHeight,
        Fade(WHITE, 0.15f)
    );

    DrawText(
        names[static_cast<int>(allocator)],
        panelX + 18,
        panelY + 14,
        22,
        RAYWHITE
    );

    if (capacityBytes > 0) {
        DrawText(
            TextFormat(
                "Used: %d / %d bytes",
                usedBytes,
                capacityBytes
            ),
            panelX + 18,
            panelY + 48,
            18,
            LIGHTGRAY
        );

        const float usage =
            static_cast<float>(usedBytes) /
            static_cast<float>(capacityBytes);

        const int barX = panelX + 18;
        const int barY = panelY + 78;
        const int barWidth = panelWidth - 36;
        const int barHeight = 14;

        DrawRectangle(
            barX,
            barY,
            barWidth,
            barHeight,
            Fade(WHITE, 0.10f)
        );

        DrawRectangle(
            barX,
            barY,
            static_cast<int>(
                barWidth *
                std::min(1.0f, usage)
            ),
            barHeight,
            SKYBLUE
        );

        DrawRectangleLines(
            barX,
            barY,
            barWidth,
            barHeight,
            Fade(WHITE, 0.20f)
        );
    }
    else {
        DrawText(
            TextFormat(
                "Live: %d bytes",
                usedBytes
            ),
            panelX + 18,
            panelY + 48,
            18,
            LIGHTGRAY
        );
    }

    // --------------------------------------------------
    // Log
    // --------------------------------------------------

    int y = panelY + 105;

    for (const auto& line : m_log) {
        DrawText(
            line.c_str(),
            panelX + 18,
            y,
            13,
            LIGHTGRAY
        );

        y += 17;
    }

    // --------------------------------------------------
    // Bottom controls
    // --------------------------------------------------

    DrawRectangle(
        0,
        GetScreenHeight() - 76,
        GetScreenWidth(),
        76,
        Fade(BLACK, 0.90f)
    );

    DrawText(
        "ALLOCATE",
        20,
        GetScreenHeight() - 67,
        14,
        GRAY
    );

    const char* actions[] = {
        "CHAR",
        "INT",
        "DOUBLE",
        "CIRCLE",
        "SQUARE",
        "RESET"
    };

    const Color actionColors[] = {
        CHAR_COLOR,
        INT_COLOR,
        DOUBLE_COLOR,
        CIRCLE_COLOR,
        SQUARE_COLOR,
        RESET_COLOR
    };

    for (int i = 0; i < 6; ++i) {
        drawButton(
            actionButton(i),
            actions[i],
            actionColors[i]
        );
    }
}