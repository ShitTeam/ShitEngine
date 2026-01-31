#pragma once

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Config.h"
#include <SFML/Graphics.hpp>

namespace Shit {
	/**
	 * @brief 纹理管理器
	 */
	class TextureManager final {
		friend class ResourceManager; //设置ResourceManager为友类，只能通过ResourceManager来管理
	
	public:
		TextureManager() = default;
		~TextureManager() = default;
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = default;
		TextureManager& operator=(TextureManager&&) = default;

	private:
		sf::Texture* loadTexture(const std::string& filePath);
		sf::Texture* getTexture(const std::string& filePath);
		void unloadTexture(const std::string& filePath);
		void clearTexture();

		std::unordered_map<std::string, std::unique_ptr<sf::Texture> > m_textures; //存放Texture
	};
}