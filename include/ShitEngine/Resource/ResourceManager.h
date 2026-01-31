#pragma once

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Config.h"
#include "TextureManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

namespace Shit {
	/**
	* @brief 资源管理器
	*
	*/
	class SHIT_API ResourceManager final {
	public:
		ResourceManager();
		~ResourceManager();

		void clear();

		//资源访问入口
		// Texture
		sf::Texture* loadTexture(const std::string& filePath) { return m_textureManager->loadTexture(filePath); }
		sf::Texture* getTexture(const std::string& filePath) { return m_textureManager->getTexture(filePath); }
		void unloadTexture(const std::string& filePath) { m_textureManager->unloadTexture(filePath); }
		void clearTexture() { m_textureManager->clearTexture(); }

		//// Sound
		//sf::SoundBuffer* loadSound(const std::string& filePath);
		//sf::SoundBuffer* getSound(const std::string& filePath);
		//void unloadSound(const std::string& filePath);
		//void clearSound();

		//// Music
		//sf::Music* loadMusic(const std::string& filePath);
		//sf::Music* getMusic(const std::string& filePath);
		//void unloadMusic(const std::string& filePath);
		//void clearMusic();

		//// Fonts
		//sf::Font* loadFont(const std::string& filePath);
		//sf::Font* getFont(const std::string& filePath);
		//void unloadFont(const std::string& filePath);
		//void clearFont();
		//
	private:
		std::unique_ptr<TextureManager> m_textureManager;
	};
}