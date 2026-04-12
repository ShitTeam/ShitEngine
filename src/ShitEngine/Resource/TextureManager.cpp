#include "ShitEngine/Resource/TextureManager.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {
	sf::Texture* TextureManager::loadTexture(const std::string& filePath)
	{
		// 检查是否已加载
		if (auto it = m_textures.find(filePath); it != m_textures.end()) {
			return it->second.get();
		}
		
		auto new_texture = std::make_unique<sf::Texture>();

		if (!new_texture->loadFromFile(filePath)) {
			ST_CORE_ERROR("无法加载 {}", filePath);
			return nullptr;
		}

		sf::Texture* ptr = new_texture.get();

		//插入纹理
		m_textures.insert({ filePath, std::move(new_texture)});

		return ptr;
	}

	sf::Texture* TextureManager::getTexture(const std::string& filePath)
	{
		// 检查是否已加载
		if (auto it = m_textures.find(filePath); it != m_textures.end()) {
			return it->second.get();
		}

		ST_CORE_ERROR("没有找到纹理 {} ，正在加载 ...", filePath);

		return loadTexture(filePath);
	}

	void TextureManager::unloadTexture(const std::string& filePath)
	{
		if (auto it = m_textures.find(filePath); it != m_textures.end()) {
			ST_CORE_DEBUG("卸载纹理 {}", filePath);
			m_textures.erase(it);
		}
		else {
			ST_CORE_WARN("尝试卸载不存在的纹理 {}", filePath);
		}
	}

	void TextureManager::clearTexture()
	{
		if (!m_textures.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的纹理。", m_textures.size());
			m_textures.clear();
		}
	}
}