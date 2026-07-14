#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <SDL3_ttf/SDL_ttf.h>

#include "ShitEngine/Core/Core.h"

namespace Shit {
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

		TTF_Font* loadFont(const std::string& filePath);
		TTF_Font* getFont(const std::string& filePath);
		void unloadFont(const std::string& filePath);
		void clearFont();

		float m_fontSize = 16.0f;
		std::unordered_map<std::string, std::unique_ptr<TTF_Font, TTF_FontDeleter>> m_fonts;
	};
}
