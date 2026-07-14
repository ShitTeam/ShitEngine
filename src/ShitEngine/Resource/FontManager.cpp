#include "ShitEngine/Resource/FontManager.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {
	TTF_Font* FontManager::loadFont(const std::string& filePath) {
		if (auto it = m_fonts.find(filePath); it != m_fonts.end()) {
			return it->second.get();
		}

		auto new_font = std::unique_ptr<TTF_Font, TTF_FontDeleter>(TTF_OpenFont(filePath.c_str(), m_fontSize));
		if (!new_font) {
			ST_CORE_ERROR("无法加载字体 {}", filePath);
			return nullptr;
		}

		TTF_Font* ptr = new_font.get();
		m_fonts[filePath] = std::move(new_font);
		return ptr;
	}

	TTF_Font* FontManager::getFont(const std::string& filePath) {
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
		} else {
			ST_CORE_WARN("尝试卸载不存在的字体 {}", filePath);
		}
	}

	void FontManager::clearFont() {
		if (!m_fonts.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的字体。", m_fonts.size());
			m_fonts.clear();
		}
	}
}
