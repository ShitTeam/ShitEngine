#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3_ttf/SDL_ttf.h>

namespace Shit {
	ResourceManager& ResourceManager::GetInstance() {
		static ResourceManager instance;
		return instance;
	}

	ResourceManager::ResourceManager() = default;

	ResourceManager::~ResourceManager() {
		// m_fontManager 已按成员逆序先析构（所有 TTF_Font 已 TTF_CloseFont），此时 Quit 安全。
		if (TTF_WasInit()) {
			TTF_Quit();
		}
	}

	void ResourceManager::init() {
		// 初始化 SDL_ttf：必须在任何 TTF_OpenFont 之前调用，否则报 "Library not initialized"。
		// UIText 的进程级字体缓存与本管理器的 FontManager 都依赖此初始化。
		if (!TTF_Init()) {
			ST_CORE_WARN("SDL_ttf 初始化失败：{}。字体功能将不可用。", SDL_GetError());
		}

		m_textureManager = std::unique_ptr<TextureManager>(new TextureManager());
		m_audioManager = std::unique_ptr<AudioManager>(new AudioManager());
		m_fontManager = std::unique_ptr<FontManager>(new FontManager());

		ST_CORE_TRACE("资源管理器初始化成功。");
	}

	void ResourceManager::clear() {
		m_textureManager->clearTexture();
		m_audioManager->clearAudio();
		m_fontManager->clearFont();
	}
}