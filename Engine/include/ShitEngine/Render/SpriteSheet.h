#pragma once

#include "../Core/Core.h"
#include "../Math.h"
#include <vector>
#include <cstddef>
#include <algorithm>

namespace Shit {

/**
 * @brief Sprite sheet grid cutter
 *
 * Cuts a texture arranged in "rows x cols" grid into per-frame source rectangles (Rect).
 * Optional margin (padding around the edges) and spacing (gap between frames), in pixels.
 *
 * Usage:
 *   Shit::SpriteSheet sheet(4, 8, 32, 32);        // 4 rows, 8 cols, 32x32 per frame
 *   Rect f = sheet.getFrameRect(5);               // global frame 5
 *   std::vector<int> walk{0,1,2,3,4,5};
 *   animComp->play("walk", &sheet, walk, 0.1f, true);
 */
class SHIT_API SpriteSheet final {
public:
    SpriteSheet(int rows, int cols, float frameWidth, float frameHeight,
                float margin = 0.0f, float spacing = 0.0f);

    ~SpriteSheet() = default;

    SpriteSheet(const SpriteSheet&) = default;
    SpriteSheet& operator=(const SpriteSheet&) = default;
    SpriteSheet(SpriteSheet&&) noexcept = default;
    SpriteSheet& operator=(SpriteSheet&&) noexcept = default;

    Rect getFrameRect(int frameIndex) const;
    Rect getFrameRect(int row, int col) const;

    int getRows() const { return m_rows; }
    int getCols() const { return m_cols; }
    float getFrameWidth() const { return m_frameWidth; }
    float getFrameHeight() const { return m_frameHeight; }
    float getMargin() const { return m_margin; }
    float getSpacing() const { return m_spacing; }
    int getFrameCount() const { return m_rows * m_cols; }

    void setRows(int rows) { m_rows = std::max(1, rows); }
    void setCols(int cols) { m_cols = std::max(1, cols); }
    void setFrameWidth(float width) { m_frameWidth = std::max(0.0f, width); }
    void setFrameHeight(float height) { m_frameHeight = std::max(0.0f, height); }
    void setMargin(float margin) { m_margin = std::max(0.0f, margin); }
    void setSpacing(float spacing) { m_spacing = std::max(0.0f, spacing); }

private:
    int m_rows = 0;
    int m_cols = 0;
    float m_frameWidth = 0.0f;
    float m_frameHeight = 0.0f;
    float m_margin = 0.0f;
    float m_spacing = 0.0f;
};

} // namespace Shit
