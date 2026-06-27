#pragma once
#include "../Core/Core.h"
#include <string>
#include <SDL3/SDL_rect.h>
#include <optional>

namespace Shit {
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

	private:
		std::string m_texturePath;                    // 纹理路径
		std::optional<SDL_FRect> m_sourceRect;        // 源矩形（留空 = 整张纹理）
		bool m_isFlipped = false;                     // 是否水平翻转
	};
}
