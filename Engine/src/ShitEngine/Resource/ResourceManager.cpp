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
		// 手动先清全部类型缓存（字体关闭所有 TTF_Font），再 TTF_Quit()：
		// 成员析构发生在函数体之后，届时不能保证 TTF 仍可用。
		// 仅当本上下文确实调用了 TTF_Init 才 TTF_Quit：TTF 是进程级全局，
		// 未 init 的上下文若依 TTF_WasInit() 退出，会关掉其他活动上下文的字体。
		clear();
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
		// 遍历类型注册表逐缓存清理——自动覆盖未来新增的资源类型
		for (auto& [id, cache] : m_caches) {
			if (!cache || cache->size() == 0) continue;
			ST_CORE_DEBUG("正在清除所有 {} 个缓存的{}。", cache->size(), cache->typeName());
			cache->clear();
		}
	}

	std::unordered_map<std::type_index, std::unique_ptr<TypedCacheBase>>::iterator
	ResourceManager::registerCache(std::type_index id, std::unique_ptr<TypedCacheBase> cache) {
		return m_caches.emplace(id, std::move(cache)).first;
	}

	// ── 混音器注入（AudioPlayer 生命周期管理） ──

	void ResourceManager::SetAudioMixer(MIX_Mixer* mixer) {
		GetInstance().m_mixer = mixer;
	}

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

	// ── uuid 查询（遍历全部已注册缓存；自动覆盖未来新增资源类型） ──

	Resource* ResourceManager::GetResourceByUuid(std::uint64_t uuid) {
		Resource* found = nullptr;
		auto& rm = GetInstance();
		for (auto& [id, cache] : rm.m_caches) {
			if (!cache) continue;
			cache->forEachResource([&found, uuid](Resource* res) {
				if (!found && res && res->getUuid() == uuid) found = res;
			});
			if (found) break;
		}
		return found;
	}
}
