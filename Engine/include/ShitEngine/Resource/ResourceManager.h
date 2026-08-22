#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include "ShitEngine/Core/Core.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Resource/Resource.h"

struct SDL_Texture;
struct MIX_Audio;
struct TTF_Font;
struct MIX_Mixer;

namespace Shit {

	// 前向声明：完整定义在 ResourceManager 类之后（traits 工厂需访问其成员）
	template <typename Res> struct ResourceTraits;
	template <typename Res> class TypedResourceCache;

	/**
	 * @brief 资源管理器（引擎上下文持有）—— 模板化统一门面
	 *
	 * 通过 Load<T>(key...) 按资源类型访问缓存；每种类型由 ResourceTraits<T> 特化
	 * 描述（缓存键 / 哈希 / 工厂），首次使用时自动在类型注册表创建独立缓存。
	 *
	 * 扩展新资源类型（引擎或插件侧）：继承 Resource 实现 load()/release()，再
	 * 特化 ResourceTraits<T>（KeyType/HashType/kTypeName/create）即可——缓存、
	 * GetResourceByUuid、全量清理、日志计数自动接入，管理器零改动。
	 *
	 * 语义：懒加载；加载失败不缓存（下次请求重试，文件后补/修复后自愈）。
	 * 同键资源只加载一次；调用方借用指针，缓存生命周期 = 引擎会话
	 * （Game::Destroy 统一按序销毁），借用裸指针安全。
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
		inline static void Destroy() { GetInstance().clear(); }  ///< 释放所有资源缓存（必须在 SDL_Quit 之前调用）

		// --- 模板门面（Res 须已特化 ResourceTraits，见文件尾部内置三特化） ---

		/// 取或加载（懒加载）。键参数直接构造 ResourceTraits<Res>::KeyType：
		///   Load<Texture>("a.png") / Load<Font>("a.ttf", 24.f)
		/// 返回封装资源（->get() 取底层句柄）；失败返回 nullptr（未缓存，可重试）。
		template <typename Res, typename... Args>
		static Res* Load(Args&&... args) {
			typename ResourceTraits<Res>::KeyType key{ std::forward<Args>(args)... };
			return GetInstance().cacheFor<Res>().getOrLoad(key);
		}

		/// 仅查询缓存（命中返回封装，未命中不加载返回 nullptr）
		template <typename Res, typename... Args>
		static Res* Find(Args&&... args) {
			typename ResourceTraits<Res>::KeyType key{ std::forward<Args>(args)... };
			return GetInstance().cacheFor<Res>().find(key);
		}

		/// 卸载单个资源（返回是否确实缓存过）
		template <typename Res, typename... Args>
		static bool Unload(Args&&... args) {
			typename ResourceTraits<Res>::KeyType key{ std::forward<Args>(args)... };
			return GetInstance().cacheFor<Res>().unload(key);
		}

		/// 清空某类资源的全部缓存（如 Clear<Audio>()）
		template <typename Res>
		static void Clear() { GetInstance().cacheFor<Res>().clear(); }

		// 内部接口（供 AudioPlayer 注入混音器；Audio 的 traits 工厂经 audioMixer() 取用）
		static void SetAudioMixer(MIX_Mixer* mixer);
		MIX_Mixer* audioMixer() const { return m_mixer; }

		// --- 资产根 ---
		/// 设置资产根目录（空串清除）。相对路径在资产根下存在时按根解析，否则原样
		///（进程 cwd 语义）。编辑器打开项目时指向项目根，使项目内相对路径可加载。
		static void SetAssetRoot(const std::string& root);
		static const std::string& GetAssetRoot();
		/// 绝对路径原样返回；相对路径先试资产根（存在才用），否则原样返回
		static std::string ResolveAssetPath(const std::string& path);

		/// 按会话级 uuid 查资源（遍历全部已注册缓存；未命中返回 nullptr）
		static Resource* GetResourceByUuid(std::uint64_t uuid);

	private:
		void init();
		void clear();

		/// 懒注册：Res 的缓存首次访问时创建（注册表容器操作走 cpp 辅助函数）
		template <typename Res>
		TypedResourceCache<Res>& cacheFor() {
			const std::type_index id(typeid(Res));
			auto it = m_caches.find(id);
			if (it == m_caches.end())
				it = registerCache(id, std::make_unique<TypedResourceCache<Res>>(*this));
			return static_cast<TypedResourceCache<Res>&>(*it->second);
		}

		std::unordered_map<std::type_index, std::unique_ptr<TypedCacheBase>>::iterator
			registerCache(std::type_index id, std::unique_ptr<TypedCacheBase> cache);

		std::unordered_map<std::type_index, std::unique_ptr<TypedCacheBase>> m_caches;
		MIX_Mixer* m_mixer = nullptr;     // 非所有，AudioPlayer 注入/清空
		std::string m_assetRoot;          // 相对资产路径的解析根（空 = 仅进程 cwd）
		bool m_ttfInitialized = false;    ///< 本上下文是否调用了 TTF_Init（析构时决定是否 TTF_Quit，防止多上下文互相关闭）
	};

	// ═══════════════════════════════════════════════════════════
	// 资源类型 traits —— 扩展点
	//
	// 主模板故意不定义：漏特化的类型在 Load<T> 实例化处编译报错
	//（incomplete type ResourceTraits<T>），防止忘注册。
	// 内置三特化如下；新资源类型照此模式在自己的头文件/插件里特化即可。
	// ═══════════════════════════════════════════════════════════

	template <typename Res> struct ResourceTraits;

	template <> struct ResourceTraits<Texture> {
		using KeyType = std::string;
		using HashType = std::hash<std::string>;
		static constexpr std::string_view kTypeName = "纹理";
		static std::unique_ptr<Texture> create(ResourceManager& owner, const KeyType& key) {
			return std::make_unique<Texture>(owner.ResolveAssetPath(key));
		}
	};

	template <> struct ResourceTraits<Font> {
		using KeyType = FontKey;
		using HashType = FontKeyHash;
		static constexpr std::string_view kTypeName = "字体";
		static std::unique_ptr<Font> create(ResourceManager& owner, const KeyType& key) {
			return std::make_unique<Font>(owner.ResolveAssetPath(key.path), key.size);
		}
	};

	template <> struct ResourceTraits<Audio> {
		using KeyType = std::string;
		using HashType = std::hash<std::string>;
		static constexpr std::string_view kTypeName = "音频";
		static std::unique_ptr<Audio> create(ResourceManager& owner, const KeyType& key) {
			// mixer 注入到新建 Audio（缓存条目的 mixer 指针在 AudioPlayer 生命周期内有效）
			auto audio = std::make_unique<Audio>(owner.ResolveAssetPath(key));
			audio->setMixer(owner.audioMixer());
			return audio;
		}
	};

	/**
	 * @brief 类型化缓存 —— 统一「查缓存 → 工厂创建 → 加载 → 插入」模式
	 *
	 * 加载失败不缓存：下次请求重试（文件后补/修复后自愈）。
	 * 经 TypedCacheBase 基类进入管理器注册表（无跨 DLL 模板实例化问题）。
	 */
	template <typename Res>
	class TypedResourceCache final : public TypedCacheBase {
	public:
		using KeyType = typename ResourceTraits<Res>::KeyType;
		using HashType = typename ResourceTraits<Res>::HashType;

		explicit TypedResourceCache(ResourceManager& owner) : m_owner(owner) {}

		/// 查缓存命中直接返回；未命中用 traits 工厂创建并 load()，成功才入缓存
		Res* getOrLoad(const KeyType& key) {
			if (auto* hit = find(key)) return hit;
			ST_CORE_DEBUG("{} {} 未命中缓存，正在加载 ...", ResourceTraits<Res>::kTypeName, logKey(key));
			std::unique_ptr<Res> res = ResourceTraits<Res>::create(m_owner, key);
			if (!res || !res->load())
				return nullptr;   // 失败不缓存：下次请求重试
			Res* ptr = res.get();
			m_cache.emplace(key, std::move(res));
			return ptr;
		}

		Res* find(const KeyType& key) const {
			if (auto it = m_cache.find(key); it != m_cache.end())
				return it->second.get();
			return nullptr;
		}

		bool unload(const KeyType& key) { return m_cache.erase(key) > 0; }

		void clear() override { m_cache.clear(); }
		size_t size() const override { return m_cache.size(); }
		std::string_view typeName() const override { return ResourceTraits<Res>::kTypeName; }
		void forEachResource(const std::function<void(Resource*)>& fn) const override {
			for (const auto& [key, res] : m_cache) fn(res.get());
		}

	private:
		// 日志键显示：字符串键原样，复合键取路径部分
		static const std::string& logKey(const std::string& k) { return k; }
		static const std::string& logKey(const FontKey& k) { return k.path; }

		ResourceManager& m_owner;
		std::unordered_map<KeyType, std::unique_ptr<Res>, HashType> m_cache;
	};
}