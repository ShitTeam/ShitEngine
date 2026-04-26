#pragma once

#include <memory>
#include <string>

#include "ShitEngine/Core/Core.h"
#include "TextureManager.h"
#include "AudioManager.h"
#include "FontManager.h"

namespace sf {
	class Texture;
	class SoundBuffer;
	class Music;
	class Font;
}

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
		// Textures
		static sf::Texture* LoadTexture(const std::string& filePath) { return GetInstance().m_textureManager->loadTexture(filePath); }
		static sf::Texture* GetTexture(const std::string& filePath) { return GetInstance().m_textureManager->getTexture(filePath); }
		static void UnloadTexture(const std::string& filePath) { GetInstance().m_textureManager->unloadTexture(filePath); }
		static void ClearTexture() { GetInstance().m_textureManager->clearTexture(); }

		// Sounds
		static sf::SoundBuffer* LoadSound(const std::string& filePath) { return GetInstance().m_audioManager->loadSound(filePath); }
		static sf::SoundBuffer* GetSound(const std::string& filePath) { return GetInstance().m_audioManager->getSound(filePath); }
		static void UnloadSound(const std::string& filePath) { GetInstance().m_audioManager->unloadSound(filePath); }
		static void ClearSound() { GetInstance().m_audioManager->clearSound(); }

		// Fonts
		static sf::Font* LoadFont(const std::string& filePath) { return GetInstance().m_fontManager->loadFont(filePath); }
		static sf::Font* GetFont(const std::string& filePath) { return GetInstance().m_fontManager->getFont(filePath); }
		static void UnloadFont(const std::string& filePath) { GetInstance().m_fontManager->unloadFont(filePath); }
		static void ClearFont() { GetInstance().m_fontManager->clearFont(); }
		
	private:
		void clear();
		void init();

		std::unique_ptr<TextureManager> m_textureManager;
		std::unique_ptr<AudioManager> m_audioManager;
		std::unique_ptr<FontManager> m_fontManager;
	};
}