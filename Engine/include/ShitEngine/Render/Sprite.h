#pragma once
#include "../Core/Core.h"
#include <string>
#include <SDL3/SDL_rect.h>
#include <optional>

namespace Shit {

class SpriteSheet;

/**
 * @brief 精灵数据，描述"画什么"
 */
class SHIT_API Sprite final {
public:
    Sprite();
    Sprite(const std::string& texturePath, const std::optional<SDL_FRect>& sourceRect = std::nullopt, bool flipped = false)
        : m_texturePath(texturePath), m_sourceRect(sourceRect), m_isFlipped(flipped) {}

    // --- getter & setter ---
    const std::string& getTexturePath() const { return m_texturePath; }
    const std::optional<SDL_FRect>& getSourceRect() const { return m_sourceRect; }
    bool isFlipped() const { return m_isFlipped; }

    void setTexturePath(const std::string& texturePath) { m_texturePath = texturePath; }
    void setSourceRect(const std::optional<SDL_FRect>& rect) { m_sourceRect = rect; }
    void setFlipped(bool flipped) { m_isFlipped = flipped; }

    /// 从 SpriteSheet 中取第 frameIndex 帧作为源矩形
    void setFrame(const SpriteSheet& sheet, int frameIndex);

private:
    std::string m_texturePath;
    std::optional<SDL_FRect> m_sourceRect;
    bool m_isFlipped = false;
};

} // namespace Shit
