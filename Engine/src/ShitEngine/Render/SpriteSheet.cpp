#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Render/SpriteSheet.h"

namespace Shit {

SpriteSheet::SpriteSheet(int rows, int cols, float frameWidth, float frameHeight,
                         float margin, float spacing)
    : m_rows(rows)
    , m_cols(cols)
    , m_frameWidth(frameWidth)
    , m_frameHeight(frameHeight)
    , m_margin(margin)
    , m_spacing(spacing)
{
}

SDL_FRect SpriteSheet::getFrameRect(int row, int col) const {
    // 越界 clamp 到合法范围
    if (m_cols > 0) {
        if (col < 0) col = 0;
        else if (col >= m_cols) col = m_cols - 1;
    } else {
        col = 0;
    }
    if (m_rows > 0) {
        if (row < 0) row = 0;
        else if (row >= m_rows) row = m_rows - 1;
    } else {
        row = 0;
    }

    return SDL_FRect{
        m_margin + col * (m_frameWidth + m_spacing),
        m_margin + row * (m_frameHeight + m_spacing),
        m_frameWidth,
        m_frameHeight
    };
}

SDL_FRect SpriteSheet::getFrameRect(int frameIndex) const {
    if (m_cols <= 0) return SDL_FRect{};
    // 全局索引 → 行列
    if (frameIndex < 0) frameIndex = 0;
    const int total = m_rows * m_cols;
    if (total > 0 && frameIndex >= total) frameIndex = total - 1;

    const int row = frameIndex / m_cols;
    const int col = frameIndex % m_cols;
    return getFrameRect(row, col);
}

} // namespace Shit
