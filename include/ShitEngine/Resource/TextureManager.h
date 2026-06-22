#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <SDL3/SDL_render.h>

#include "ShitEngine/Core/Core.h"

namespace Shit {
	/**
	 * @brief 纹理管理器
	 */
	class TextureManager final {
		friend class ResourceManager; //设置ResourceManager为友类，只能通过ResourceManager来管理
	
	public:
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = default;
		TextureManager& operator=(TextureManager&&) = default;
	
	private:
		TextureManager() = default;
		~TextureManager() = default;

		struct SDLTextureDeleter {
			void operator()(SDL_Texture* texture) const {
				if (texture) {
					SDL_DestroyTexture(texture);
				}
			}
		};

		SDL_Texture* loadTexture(const std::string& filePath);
		SDL_Texture* getTexture(const std::string& filePath);
		void unloadTexture(const std::string& filePath);
		void clearTexture();

		std::unordered_map<std::string, std::unique_ptr<SDL_Texture, SDLTextureDeleter> > m_textures; //存放Texture
	};
}