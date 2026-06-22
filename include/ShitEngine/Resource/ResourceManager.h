#pragma once

#include <memory>
#include <string>

#include "ShitEngine/Core/Core.h"
#include "TextureManager.h"
#include "AudioManager.h"
#include "FontManager.h"

// 前向声明
struct SDL_Texture;
struct MIX_Audio;
struct TTF_Font;

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
		static SDL_Texture* LoadTexture(const std::string& filePath) { return GetInstance().m_textureManager->loadTexture(filePath); }
		static SDL_Texture* GetTexture(const std::string& filePath) { return GetInstance().m_textureManager->getTexture(filePath); }
		static void UnloadTexture(const std::string& filePath) { GetInstance().m_textureManager->unloadTexture(filePath); }
		static void ClearTexture() { GetInstance().m_textureManager->clearTexture(); }

		// // Sounds
		// static MIX_Audio* LoadSound(const std::string& filePath) { return GetInstance().m_audioManager->loadSound(filePath); }
		// static MIX_Audio* GetSound(const std::string& filePath) { return GetInstance().m_audioManager->getSound(filePath); }
		// static void UnloadSound(const std::string& filePath) { GetInstance().m_audioManager->unloadSound(filePath); }
		// static void ClearSound() { GetInstance().m_audioManager->clearSound(); }

		// // Music
		// static MIX_Audio* LoadMusic(const std::string& filePath) { return GetInstance().m_audioManager->loadMusic(filePath); }
		// static MIX_Audio* GetMusic(const std::string& filePath) { return GetInstance().m_audioManager->getMusic(filePath); }
		// static void UnloadMusic(const std::string& filePath) { GetInstance().m_audioManager->unloadMusic(filePath); }
		// static void ClearMusic() { GetInstance().m_audioManager->clearMusic(); }

		// // Fonts
		// static TTF_Font* LoadFont(const std::string& filePath) { return GetInstance().m_fontManager->loadFont(filePath); }
		// static TTF_Font* GetFont(const std::string& filePath) { return GetInstance().m_fontManager->getFont(filePath); }
		// static void UnloadFont(const std::string& filePath) { GetInstance().m_fontManager->unloadFont(filePath); }
		// static void ClearFont() { GetInstance().m_fontManager->clearFont(); }
		
	private:
		void clear();
		void init();

		std::unique_ptr<TextureManager> m_textureManager;
		// std::unique_ptr<AudioManager> m_audioManager;
		// std::unique_ptr<FontManager> m_fontManager;
	};
}