#pragma once

#include <memory>
#include <string>

#include "ShitEngine/Core/Core.h"
#include "TextureManager.h"
#include "AudioManager.h"
#include "FontManager.h"

struct SDL_Texture;
struct MIX_Audio;
struct TTF_Font;

namespace Shit {
	/**
	 * @brief 资源管理器（单例）
	 *
	 * 统一门面类，封装纹理、音频、字体的加载与缓存。
	 * 同路径资源只加载一次，后续请求从缓存返回。
	 * 在 Game::Init() 中自动初始化。
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

		// --- 纹理 ---
		static SDL_Texture* LoadTexture(const std::string& filePath)   { return GetInstance().m_textureManager->loadTexture(filePath); }
		static SDL_Texture* GetTexture(const std::string& filePath)    { return GetInstance().m_textureManager->getTexture(filePath); }
		static void UnloadTexture(const std::string& filePath)         { GetInstance().m_textureManager->unloadTexture(filePath); }
		static void ClearTexture()                                      { GetInstance().m_textureManager->clearTexture(); }

		// --- 音频 ---
		static MIX_Audio* LoadAudio(const std::string& filePath)       { return GetInstance().m_audioManager->loadAudio(filePath); }
		static MIX_Audio* GetAudio(const std::string& filePath)        { return GetInstance().m_audioManager->getAudio(filePath); }
		static void UnloadAudio(const std::string& filePath)           { GetInstance().m_audioManager->unloadAudio(filePath); }
		static void ClearAudio()                                        { GetInstance().m_audioManager->clearAudio(); }

		// --- 字体 ---
		static TTF_Font* LoadFont(const std::string& filePath)         { return GetInstance().m_fontManager->loadFont(filePath); }
		static TTF_Font* GetFont(const std::string& filePath)          { return GetInstance().m_fontManager->getFont(filePath); }
		static void UnloadFont(const std::string& filePath)            { return GetInstance().m_fontManager->unloadFont(filePath); }
		static void ClearFont()                                         { GetInstance().m_fontManager->clearFont(); }

		// 内部接口（供 AudioPlayer 注入混音器）
		static void SetAudioMixer(struct MIX_Mixer* mixer) { GetInstance().m_audioManager->setMixer(mixer); }

	private:
		void clear();
		void init();

		std::unique_ptr<TextureManager> m_textureManager;
		std::unique_ptr<AudioManager> m_audioManager;
		std::unique_ptr<FontManager> m_fontManager;
	};
}
