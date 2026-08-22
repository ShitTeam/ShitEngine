#pragma once

#include <cstdint>
#include <string>

#include "ShitEngine/Core/Core.h"
#include "ShitEngine/Resource/Resource.h"

struct SDL_Texture;
struct MIX_Audio;
struct TTF_Font;
struct MIX_Mixer;

namespace Shit {
	/**
	 * @brief 资源管理器（引擎上下文持有）
	 *
	 * 统一门面：纹理 / 音频 / 字体的加载与缓存（原 TextureManager/FontManager/
	 * AudioManager 已折叠为 ResourceCache 模板缓存 + Resource 子类封装）。
	 * 同键资源只加载一次；调用方借用裸句柄，缓存在 Game::Destroy 统一按序销毁。
	 * 相对路径经资产根解析（SetAssetRoot，编辑器指向项目根）；未命中回退进程
	 * cwd 语义——Runtime 的 "resource/..." 场景路径不受影响。
	 */
	class SHIT_API ResourceManager final {
	public:
		ResourceManager();
		~ResourceManager();

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = delete;
		ResourceManager& operator=(ResourceManager&&) = delete;

		// --- 静态API ---
		static ResourceManager& GetInstance();
		inline static void Init() { GetInstance().init(); }
		inline static void Destroy() { GetInstance().clear(); }                   ///< 释放所有资源缓存（必须在 SDL_Quit 之前调用）

		// --- 纹理 ---
		static SDL_Texture* LoadTexture(const std::string& filePath);
		static SDL_Texture* GetTexture(const std::string& filePath);
		static Texture* GetTextureAsset(const std::string& filePath);   ///< 完整封装（状态/错误/uuid）
		static void UnloadTexture(const std::string& filePath);
		static void ClearAllTextures();
		static void ClearTexture();

		// --- 音频 ---
		static MIX_Audio* LoadAudio(const std::string& filePath);
		static MIX_Audio* GetAudio(const std::string& filePath);
		static Audio* GetAudioAsset(const std::string& filePath);
		static void UnloadAudio(const std::string& filePath);
		static void ClearAudio();

		// --- 字体（按路径+字号缓存） ---
		static TTF_Font* LoadFont(const std::string& filePath, float fontSize);
		static TTF_Font* GetFont(const std::string& filePath, float fontSize);
		static Font* GetFontAsset(const std::string& filePath, float fontSize);
		static void ClearAllFonts();
		static void ClearFont();

		// 内部接口（供 AudioPlayer 注入混音器）
		static void SetAudioMixer(MIX_Mixer* mixer);

		// --- 资产根 ---
		/// 设置资产根目录（空串清除）。相对路径在资产根下存在时按根解析，否则原样
		///（进程 cwd 语义）。编辑器打开项目时指向项目根，使项目内相对路径可加载。
		static void SetAssetRoot(const std::string& root);
		static const std::string& GetAssetRoot();
		/// 绝对路径原样返回；相对路径先试资产根（存在才用），否则原样返回
		static std::string ResolveAssetPath(const std::string& path);

		/// 按会话级 uuid 查资源（遍历三类缓存；未命中返回 nullptr）
		static Resource* GetResourceByUuid(std::uint64_t uuid);

	private:
		void init();
		void clear();

		Texture* textureAsset(const std::string& filePath);
		Audio* audioAsset(const std::string& filePath);
		Font* fontAsset(const std::string& filePath, float fontSize);

		ResourceCache<std::string, Texture> m_textures;
		ResourceCache<std::string, Audio> m_audioCache;
		ResourceCache<FontKey, Font, FontKeyHash> m_fonts;
		MIX_Mixer* m_mixer = nullptr;     // 非所有，AudioPlayer 注入/清空
		std::string m_assetRoot;          // 相对资产路径的解析根（空 = 仅进程 cwd）
		bool m_ttfInitialized = false;    ///< 本上下文是否调用了 TTF_Init（析构时决定是否 TTF_Quit，防止多上下文互相关闭）
	};
}
