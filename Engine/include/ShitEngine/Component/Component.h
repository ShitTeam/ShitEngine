#pragma once
#include "../Core/Config.h"
#include "../Reflection/Macros.h"

#include <cstdint>

namespace Shit {
	class GameObject; // 前向声明
	class System;     // 前向声明（friend：系统销毁时重置认领组件的注册状态）

	/// @brief 生成随机组件 UUID（64 位；0 保留为空引用，永不返回 0）
	SHIT_API uint64_t GenerateComponentUuid();

	/**
	 * @brief 组件基类
	 *
	 * 所有组件必须继承此类。
	 *
	 * 生命周期（按调用顺序）：
	 *   onCreate  — 组件刚被构造、已绑定 owner，尚未挂载到场景
	 *   onAttach  — 组件所属 GameObject 进入活动场景时
	 *   onDetach  — 组件即将从场景中移除时（在 onDestroy 前调用）
	 *   onDestroy — 组件被销毁时
	 */
	class SHIT_API SHIT_REFLECT(BlackList) Component {
		friend class GameObject; // 只能通过GameObject初始化组件
		friend class System;     // 系统销毁时通过 System::resetComponent 重置组件注册状态
		SHIT_REFLECT_BODY(Component)

	public:
		Component();
		virtual ~Component() = default;

virtual void onCreate() {}   // 组件创建时（有 owner，但可能尚未挂到场景）
		virtual void onAttach() { m_isRegistered = true; }   // 组件挂载到活动场景时（基类默认标记已注册；依赖系统的组件在找不到系统时自行置回 false 以便补挂）
		virtual void onDetach() {}   // 组件从场景卸下时
		virtual void onDestroy() {}  // 组件销毁时

		/// 反序列化完成后回调（Prefab::apply 逐组件调用一次，字段已写入）。
		/// 用于重建"直写字段绕过了 setter、无法自动触发的引擎状态"（如精灵纹理加载）。
		virtual void onAfterDeserialize() {}

		// 禁止拷贝和移动
		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(Component&&) = delete;

		GameObject* getOwner() const { return m_owner; }
		bool isRegistered() const { return m_isRegistered; }

		/// @brief 组件持久 ID（随 .scene 序列化，是 ComponentRef<T> 引用字段的寻址依据）
		uint64_t getUuid() const { return m_uuid; }
		/// @brief 直接设置组件 ID（仅引擎反序列化/索引维护使用；正常流程由构造函数随机分配）
		void setUuid(uint64_t uuid) { m_uuid = uuid; }

	private:
		// 只允许 GameObject 设置
		inline void setOwner(GameObject* owner) { m_owner = owner; }

	protected:
		// 内部：仅 System 通过 resetComponent 调用，用于系统注销后重置注册状态，
		// 使已存在组件在同类系统重新注册时能被 System::init 补扫认领。
		inline void setRegistered(bool registered) { m_isRegistered = registered; }

	protected:
		SHIT_META(({.displayName = "Owner", .readOnly = true}))
		GameObject* m_owner = nullptr; // 组件的拥有者
		SHIT_META(({.displayName = "Registered", .readOnly = true}))
		bool m_isRegistered = false;
		SHIT_META(Disable)
		uint64_t m_uuid = 0; ///< 持久 ID（构造时随机分配；0 仅在序列化前尚未赋值时短暂存在）
	};
}