#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3_ttf/SDL_ttf.h>

namespace Shit {
	ResourceManager& ResourceManager::GetInstance() {
		return EngineContext::current().resources;
	}

	ResourceManager::ResourceManager() = default;

	ResourceManager::~ResourceManager() {
		// 先手动释放 FontManager（关闭所有 TTF_Font），再 TTF_Quit()。
		// 仅当本上下文确实调用了 TTF_Init 才 TTF_Quit：TTF 是进程级全局，一个从未
		// init 的上下文析构时若依据 TTF_WasInit() 调 TTF_Quit，会关掉其他活动上下文的字体。
		m_fontManager.reset();
		if (m_ttfInitialized) {
			TTF_Quit();
			m_ttfInitialized = false;
		}
	}

	void ResourceManager::init() {
		// 初始化 SDL_ttf：必须在任何 TTF_OpenFont 之前调用，否则报 "Library not initialized"。
		// TTF_Init 返回 bool（true=成功），且内部引用计数；本上下文成功 init 一次，
		// 析构时对应 TTF_Quit 一次，保持多上下文下的平衡。
		if (TTF_Init()) {
			m_ttfInitialized = true;
		} else {
			ST_CORE_WARN("SDL_ttf 初始化失败：{}。字体功能将不可用。", SDL_GetError());
		}

		m_textureManager = std::unique_ptr<TextureManager>(new TextureManager());
		m_audioManager = std::unique_ptr<AudioManager>(new AudioManager());
		m_fontManager = std::unique_ptr<FontManager>(new FontManager());

		ST_CORE_TRACE("资源管理器初始化成功。");
	}

	void ResourceManager::clear() {
		// null 保护：init() 中途失败（或从未调用）时管理器未创建，Game::Destroy 仍会走到这里
		if (m_textureManager) m_textureManager->clearTexture();
		if (m_audioManager) m_audioManager->clearAudio();
		if (m_fontManager) m_fontManager->clearFont();
	}
}