#pragma once
#include "../Core/Core.h"
#include <string>
#include <SDL3/SDL_rect.h>
#include <optional>

namespace Shit {
	class SHIT_API Sprite final {
	public:
		Sprite();
		Sprite(const std::string& texturePath, const std::optional<SDL_FRect>& rect = std::nullopt) : m_texturePath(texturePath), m_rect(rect) {}

		// --- getter & setter ---
		const std::string& getTexturePath() const { return m_texturePath; }
		const std::optional<SDL_FRect>& getRect() const { return m_rect; }

		void setTexturePath(const std::string& texturePath) { m_texturePath = texturePath; }
		void setRect(const std::optional<SDL_FRect>& rect) { m_rect = rect; }
	private:
		std::string m_texturePath; // 纹理路径
		std::optional<SDL_FRect> m_rect; // 纹理裁剪区域（留空 = 整张纹理）
	};
}