#pragma once

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Core.h"
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

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = delete;
		ResourceManager& operator=(ResourceManager&&) = delete;

		// --- 静态API ---
		static ResourceManager& GetInstance();
		inline static void Init() { GetInstance().init(); }
		//资源访问入口
		// Texture
		static sf::Texture* LoadTexture(const std::string& filePath) { return GetInstance().m_textureManager->loadTexture(filePath); }
		static sf::Texture* GetTexture(const std::string& filePath) { return GetInstance().m_textureManager->getTexture(filePath); }
		static void UnloadTexture(const std::string& filePath) { GetInstance().m_textureManager->unloadTexture(filePath); }
		static void ClearTexture() { GetInstance().m_textureManager->clearTexture(); }

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
		void clear();
		void init();

		std::unique_ptr<TextureManager> m_textureManager;
	};
}