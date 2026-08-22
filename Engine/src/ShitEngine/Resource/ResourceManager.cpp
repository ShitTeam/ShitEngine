#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3_ttf/SDL_ttf.h>

#include <filesystem>

namespace Shit {
	ResourceManager& ResourceManager::GetInstance() {
		return EngineContext::current().resources;
	}

	ResourceManager::ResourceManager() = default;

	ResourceManager::~ResourceManager() {
		// 手动先清缓存（字体关闭所有 TTF_Font），再 TTF_Quit()：
		// 成员析构发生在函数体之后，届时不能保证 TTF 仍可用。
		// 仅当本上下文确实调用了 TTF_Init 才 TTF_Quit：TTF 是进程级全局，
		// 未 init 的上下文若依 TTF_WasInit() 退出，会关掉其他活动上下文的字体。
		m_fonts.clear();
		m_audioCache.clear();
		m_textures.clear();
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
		ST_CORE_TRACE("资源管理器初始化成功。");
	}

	void ResourceManager::clear() {
		if (!m_textures.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的纹理。", m_textures.size());
			m_textures.clear();
		}
		if (!m_audioCache.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的音频。", m_audioCache.size());
			m_audioCache.clear();
		}
		if (!m_fonts.empty()) {
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的字体。", m_fonts.size());
			m_fonts.clear();
		}
	}

	// ── 内部：三类缓存的统一取数（懒加载；失败不缓存，下次重试） ──

	Texture* ResourceManager::textureAsset(const std::string& filePath) {
		if (Texture* hit = m_textures.find(filePath)) return hit;
		ST_CORE_DEBUG("纹理 {} 未命中缓存，正在加载 ...", filePath);
		const std::string resolved = ResolveAssetPath(filePath);
		return m_textures.getOrLoad(filePath, [&] { return std::make_unique<Texture>(resolved); });
	}

	Audio* ResourceManager::audioAsset(const std::string& filePath) {
		if (Audio* hit = m_audioCache.find(filePath)) return hit;
		ST_CORE_DEBUG("音频 {} 未命中缓存，正在加载 ...", filePath);
		const std::string resolved = ResolveAssetPath(filePath);
		// mixer 注入到新建 Audio（旧缓存条目的 mixer 指针在 AudioPlayer 生命周期内有效）
		return m_audioCache.getOrLoad(filePath, [&] {
			auto audio = std::make_unique<Audio>(resolved);
			audio->setMixer(m_mixer);
			return audio;
		});
	}

	Font* ResourceManager::fontAsset(const std::string& filePath, float fontSize) {
		const FontKey key{ filePath, fontSize };
		if (Font* hit = m_fonts.find(key)) return hit;
		ST_CORE_DEBUG("字体 {} ({}) 未命中缓存，正在加载 ...", filePath, fontSize);
		const std::string resolved = ResolveAssetPath(filePath);
		return m_fonts.getOrLoad(key, [&] { return std::make_unique<Font>(resolved, fontSize); });
	}

	// ── 门面：纹理 ──

	SDL_Texture* ResourceManager::LoadTexture(const std::string& filePath) {
		Texture* t = GetInstance().textureAsset(filePath);
		return t ? t->get() : nullptr;
	}

	SDL_Texture* ResourceManager::GetTexture(const std::string& filePath) {
		Texture* t = GetInstance().textureAsset(filePath);
		return t ? t->get() : nullptr;
	}

	Texture* ResourceManager::GetTextureAsset(const std::string& filePath) {
		return GetInstance().textureAsset(filePath);
	}

	void ResourceManager::UnloadTexture(const std::string& filePath) {
		if (!GetInstance().m_textures.unload(filePath))
			ST_CORE_WARN("尝试卸载不存在的纹理 {}", filePath);
		else
			ST_CORE_DEBUG("卸载纹理 {}", filePath);
	}

	void ResourceManager::ClearAllTextures() { GetInstance().m_textures.clear(); }
	void ResourceManager::ClearTexture() { GetInstance().m_textures.clear(); }

	// ── 门面：音频 ──

	MIX_Audio* ResourceManager::LoadAudio(const std::string& filePath) {
		Audio* a = GetInstance().audioAsset(filePath);
		return a ? a->get() : nullptr;
	}

	MIX_Audio* ResourceManager::GetAudio(const std::string& filePath) {
		Audio* a = GetInstance().audioAsset(filePath);
		return a ? a->get() : nullptr;
	}

	Audio* ResourceManager::GetAudioAsset(const std::string& filePath) {
		return GetInstance().audioAsset(filePath);
	}

	void ResourceManager::UnloadAudio(const std::string& filePath) {
		if (!GetInstance().m_audioCache.unload(filePath))
			ST_CORE_WARN("尝试卸载不存在的音频 {}", filePath);
		else
			ST_CORE_DEBUG("卸载音频 {}", filePath);
	}

	void ResourceManager::ClearAudio() { GetInstance().m_audioCache.clear(); }

	void ResourceManager::SetAudioMixer(MIX_Mixer* mixer) {
		GetInstance().m_mixer = mixer;
	}

	// ── 门面：字体 ──

	TTF_Font* ResourceManager::LoadFont(const std::string& filePath, float fontSize) {
		Font* f = GetInstance().fontAsset(filePath, fontSize);
		return f ? f->get() : nullptr;
	}

	TTF_Font* ResourceManager::GetFont(const std::string& filePath, float fontSize) {
		Font* f = GetInstance().fontAsset(filePath, fontSize);
		return f ? f->get() : nullptr;
	}

	Font* ResourceManager::GetFontAsset(const std::string& filePath, float fontSize) {
		return GetInstance().fontAsset(filePath, fontSize);
	}

	void ResourceManager::ClearAllFonts() { GetInstance().m_fonts.clear(); }
	void ResourceManager::ClearFont() { GetInstance().m_fonts.clear(); }

	// ── 资产根 ──

	void ResourceManager::SetAssetRoot(const std::string& root) {
		GetInstance().m_assetRoot = root;
		if (!root.empty())
			ST_CORE_DEBUG("资产根已设置为：{}", root);
	}

	const std::string& ResourceManager::GetAssetRoot() {
		return GetInstance().m_assetRoot;
	}

	std::string ResourceManager::ResolveAssetPath(const std::string& path) {
		if (path.empty()) return path;
		namespace fs = std::filesystem;
		const fs::path p(path);
		if (p.is_absolute()) return path;
		const std::string& root = GetInstance().m_assetRoot;
		if (root.empty()) return path;   // 未设根：保持进程 cwd 语义（Runtime "resource/..." 兼容）
		const fs::path full = fs::path(root) / p;
		std::error_code ec;
		if (fs::exists(full, ec))
			return full.string();
		return path;   // 根下不存在：原样交底层（沿用旧行为，让调用方收到加载失败）
	}

	// ── uuid 查询 ──

	Resource* ResourceManager::GetResourceByUuid(std::uint64_t uuid) {
		auto& rm = GetInstance();
		for (const auto& [key, res] : rm.m_textures.entries())
			if (res->getUuid() == uuid) return res.get();
		for (const auto& [key, res] : rm.m_audioCache.entries())
			if (res->getUuid() == uuid) return res.get();
		for (const auto& [key, res] : rm.m_fonts.entries())
			if (res->getUuid() == uuid) return res.get();
		return nullptr;
	}
}
