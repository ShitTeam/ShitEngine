#pragma once

#include "../Core/Core.h"
#include <SDL3/SDL_rect.h>
#include <vector>
#include <cstddef>

namespace Shit {

/**
 * @brief 精灵图集（sprite-sheet）网格切割器
 *
 * 把一张按“rows 行 × cols 列”规则排列的大图，按帧索引切出每一帧的源矩形 (SDL_FRect)。
 * 可选 margin（图集四周留白）与 spacing（帧间间隔），单位为像素。
 *
 * 典型用法：
 *   Shit::SpriteSheet sheet(4, 8, 32, 32);        // 4行8列，每帧32×32
 *   SDL_FRect f = sheet.getFrameRect(5);          // 全局第5帧
 *   std::vector<int> walk{0,1,2,3,4,5};
 *   animComp->play("walk", &sheet, walk, 0.1f, true);
 */
class SHIT_API SpriteSheet final {
public:
    /**
     * @brief 构造网格切割参数
     * @param rows 行数
     * @param cols 列数
     * @param frameWidth 单帧宽（像素）
     * @param frameHeight 单帧高（像素）
     * @param margin 图集左上角留白（像素，默认 0）
     * @param spacing 帧与帧之间的间隔（像素，默认 0）
     */
    SpriteSheet(int rows, int cols, float frameWidth, float frameHeight,
                float margin = 0.0f, float spacing = 0.0f);

    ~SpriteSheet() = default;

    SpriteSheet(const SpriteSheet&) = default;
    SpriteSheet& operator=(const SpriteSheet&) = default;
    SpriteSheet(SpriteSheet&&) noexcept = default;
    SpriteSheet& operator=(SpriteSheet&&) noexcept = default;

    /// 全局帧索引（0..rows*cols-1）对应的源矩形
    SDL_FRect getFrameRect(int frameIndex) const;

    /// 指定行列（0 基）对应的源矩形
    SDL_FRect getFrameRect(int row, int col) const;

    int getRows() const { return m_rows; }
    int getCols() const { return m_cols; }
    float getFrameWidth() const { return m_frameWidth; }
    float getFrameHeight() const { return m_frameHeight; }
    float getMargin() const { return m_margin; }
    float getSpacing() const { return m_spacing; }
    int getFrameCount() const { return m_rows * m_cols; }

    void setRows(int rows) { m_rows = rows; }
    void setCols(int cols) { m_cols = cols; }
    void setFrameWidth(float width) { m_frameWidth = width; }
    void setFrameHeight(float height) { m_frameHeight = height; }
    void setMargin(float margin) { m_margin = margin; }
    void setSpacing(float spacing) { m_spacing = spacing; }

private:
    int m_rows = 0;
    int m_cols = 0;
    float m_frameWidth = 0.0f;
    float m_frameHeight = 0.0f;
    float m_margin = 0.0f;
    float m_spacing = 0.0f;
};

} // namespace Shit
