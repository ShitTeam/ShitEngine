#pragma once
#include "../Core/Core.h"
#include "../Core/pch.h"
#include <SFML/Graphics.hpp>
#include <optional>

namespace Shit {
	class SHIT_API Sprite final {
	public:
		Sprite();
		Sprite(const std::string& texturePath, const std::optional<sf::IntRect>& rect = std::nullopt) : m_texturePath(texturePath), m_rect(rect) {}

		// --- getter & setter ---
		const std::string& getTexturePath() const { return m_texturePath; }
		const std::optional<sf::IntRect>& getRect() const { return m_rect; }

		void setTexturePath(const std::string& texturePath) { m_texturePath = texturePath; }
		void setRect(const std::optional<sf::IntRect>& rect) { m_rect = rect; }
	private:
		std::string m_texturePath; // 纹理路径
		std::optional<sf::IntRect> m_rect;
	};
}