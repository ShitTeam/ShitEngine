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
		m_textureManager = std::make_unique<TextureManager>();
		// m_audioManager = std::make_unique<AudioManager>();

		ST_CORE_TRACE("资源管理器初始化成功。");
	}

	void ResourceManager::clear() {
		m_textureManager->clearTexture();
		// m_audioManager->clearSound();
	}
}