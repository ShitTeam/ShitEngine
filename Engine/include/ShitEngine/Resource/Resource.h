#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "ShitEngine/Core/Core.h"

struct SDL_Texture;
struct TTF_Font;
struct MIX_Audio;
struct MIX_Mixer;

namespace Shit {

	/// 会话级资源 UUID 生成（与组件 UUID 同源的随机数方案；持久化资产注册表留待后续）
	SHIT_API std::uint64_t GenerateResourceUuid();

	/// 资源加载状态
	enum class ResourceState { Unloaded, Loaded, Failed };

	/**
	 * @brief 资源基类 —— 所有引擎资产（Texture / Font / Audio / 自定义类型）的统一封装
	 *
	 * 所有权模型：管理器独占（缓存持 unique_ptr），调用方借用指针。
	 * 缓存生命周期 = 整个引擎会话（Game::Destroy 统一销毁），销毁顺序集中受控
	 * （音频先于 mixer、纹理先于 Renderer、再 SDL_Quit）——因此借用裸指针安全。
	 * 将来热重载应走 load() 原地重载（同一 wrapper 换内部句柄），借用指针保持有效。
	 *
	 * 扩展新资源类型：继承本类实现 load()/release()，再特化 ResourceManager.h 中的
	 * ResourceTraits<T> 即接入缓存/uuid 查询/全量清理——管理器零改动。
	 */
	class SHIT_API Resource {
	public:
		explicit Resource(std::string path)
			: m_path(std::move(path)), m_uuid(GenerateResourceUuid()) {}
		virtual ~Resource() = default;   // 句柄由各子类析构经 release() 释放

		Resource(const Resource&) = delete;
		Resource& operator=(const Resource&) = delete;

		const std::string& getPath() const { return m_path; }
		ResourceState getState() const { return m_state; }
		bool isLoaded() const { return m_state == ResourceState::Loaded; }
		const std::string& getError() const { return m_error; }  ///< Failed 时的诊断信息
		std::uint64_t getUuid() const { return m_uuid; }         ///< 会话级 uuid（进程内随机，不落盘）

		/// 加载底层句柄（子类实现；失败记录 m_error 并返回 false，内部已打日志）
		virtual bool load() = 0;
		/// 释放底层句柄（幂等；子类析构与管理器回收时调用；基类无句柄可释放）
		virtual void release() {}

	protected:
		std::string m_path;
		std::string m_error;
		ResourceState m_state = ResourceState::Unloaded;

	private:
		std::uint64_t m_uuid;
	};

	/// 纹理：封装 SDL_Texture*（渲染用；加载需渲染器已初始化）
	class SHIT_API Texture final : public Resource {
	public:
		explicit Texture(std::string path) : Resource(std::move(path)) {}
		~Texture() override { release(); }

		bool load() override;
		void release() override;

		SDL_Texture* get() const { return m_texture; }  ///< 借用句柄（空 = 未加载/失败）

	private:
		SDL_Texture* m_texture = nullptr;
	};

	/// 字体缓存键：同字体不同字号各缓存一份
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

	/// 字体：封装 TTF_Font*（按路径+字号复合键缓存）
	class SHIT_API Font final : public Resource {
	public:
		Font(std::string path, float size) : Resource(std::move(path)), m_size(size) {}
		~Font() override { release(); }

		bool load() override;
		void release() override;

		TTF_Font* get() const { return m_font; }
		float getSize() const { return m_size; }

	private:
		float m_size;
		TTF_Font* m_font = nullptr;
	};

	/// 音频：封装 MIX_Audio*（加载需 mixer 就绪，由管理器注入）
	class SHIT_API Audio final : public Resource {
	public:
		explicit Audio(std::string path) : Resource(std::move(path)) {}
		~Audio() override { release(); }

		bool load() override;
		void release() override;

		MIX_Audio* get() const { return m_audio; }
		void setMixer(MIX_Mixer* mixer) { m_mixer = mixer; }

	private:
		MIX_Mixer* m_mixer = nullptr;   // 非所有，AudioPlayer 注入（生命周期由 AudioPlayer 管理）
		MIX_Audio* m_audio = nullptr;
	};

	/**
	 * @brief 类型擦除缓存接口 —— ResourceManager 按资源类型注册缓存，经此统一遍历/清理
	 *
	 * 引擎/插件侧新增的资源类型缓存都以此基类指针进入管理器的类型注册表，
	 * GetResourceByUuid / 全量清理 / 日志计数只依赖本接口（无跨 DLL 模板问题）。
	 *
	 * 注意：插件侧实例化的 TypedResourceCache<T> 其虚函数实现在插件 DLL 内——
	 * 插件热卸载前必须先 ResourceManager::Clear<T>() 清掉该类型缓存，否则引擎
	 * 关停遍历注册表会调进已卸载模块（将来做插件资源需补 System::
	 * unregisterSystemsBySource 同款按来源注销机制；引擎内置三类型不受影响）。
	 */
	class SHIT_API TypedCacheBase {
	public:
		virtual ~TypedCacheBase() = default;

		virtual void clear() = 0;
		virtual size_t size() const = 0;
		virtual std::string_view typeName() const = 0;   ///< 日志用类型名（来自 ResourceTraits<T>::kTypeName）
		/// 遍历缓存内全部资源封装（uuid 查询用；顺序不保证）
		virtual void forEachResource(const std::function<void(Resource*)>& fn) const = 0;
	};
}
