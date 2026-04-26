#include "ShitEngine/Resource/FontManager.h"
#include "ShitEngine/Core/Log.h"

#include <SFML/Graphics/Font.hpp>

namespace Shit {
    sf::Font* FontManager::loadFont(const std::string& filePath) {
        // 检查是否已加载
		if (auto it = m_fonts.find(filePath); it != m_fonts.end()) {
			return it->second.get();
		}

        auto new_font = std::make_unique<sf::Font>();

        if (!new_font->openFromFile(filePath)) {
            ST_CORE_ERROR("无法加载 {}", filePath);
			return nullptr;
        }

        sf::Font* ptr = new_font.get();

		//插入纹理
		m_fonts.insert({ filePath, std::move(new_font)});

		return ptr;
    }

    sf::Font* FontManager::getFont(const std::string& filePath) {
        // 检查是否已加载
		if (auto it = m_fonts.find(filePath); it != m_fonts.end()) {
			return it->second.get();
		}

		ST_CORE_ERROR("没有找到字体 {} ，正在加载 ...", filePath);

		return loadFont(filePath);
    }

    void FontManager::unloadFont(const std::string& filePath) {
        if (auto it = m_fonts.find(filePath); it != m_fonts.end()) {
			ST_CORE_DEBUG("卸载字体 {}", filePath);
			m_fonts.erase(it);
		}
		else {
			ST_CORE_WARN("尝试卸载不存在的字体 {}", filePath);
		}
    }

    void FontManager::clearFont() {
        if (!m_fonts.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的纹理。", m_fonts.size());
			m_fonts.clear();
		}
    }
}