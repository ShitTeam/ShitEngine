#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {
	ResourceManager& ResourceManager::GetInstance() {
		static ResourceManager instance;
		return instance;
	}

	ResourceManager::ResourceManager() = default;
	ResourceManager::~ResourceManager() = default;

	void ResourceManager::init() {
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