#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <SDL3_ttf/SDL_ttf.h>

#include "ShitEngine/Core/Core.h"

namespace Shit {
	/**
	 * @brief 字体缓存管理器（按路径+字号复合键缓存 TTF_Font）
	 *
	 * 同一字体不同字号各自缓存一份。私有构造，仅 ResourceManager 可创建。
	 */
	class FontManager final {
		friend class ResourceManager;
	public:
		FontManager(const FontManager&) = delete;
		FontManager& operator=(const FontManager&) = delete;
		FontManager(FontManager&&) = default;
		FontManager& operator=(FontManager&&) = default;
		~FontManager() { clearFont(); }

	private:
		FontManager() = default;

		struct TTF_FontDeleter {
			void operator()(TTF_Font* font) const {
				if (font) TTF_CloseFont(font);
			}
		};

		struct FontKey {
			std::string path;
			float size;
			bool operator==(const FontKey& o) const { return path == o.path && size == o.size; }
		};
		struct FontKeyHash {
			std::size_t operator()(const FontKey& k) const {
				return std::hash<std::string>{}(k.path) ^ std::hash<float>{}(k.size);
			}
		};

		TTF_Font* loadFont(const std::string& filePath, float fontSize);
		TTF_Font* getFont(const std::string& filePath, float fontSize);
		void unloadFont(const std::string& filePath, float fontSize);
		void clearFont();

		std::unordered_map<FontKey, std::unique_ptr<TTF_Font, TTF_FontDeleter>, FontKeyHash> m_fonts;
	};
}
