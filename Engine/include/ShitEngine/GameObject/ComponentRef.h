#pragma once
// ═══════════════════════════════════════════════════════════════
// P20: 组件引用（类似 Unity 的 Object 引用字段）
//
//   ComponentRef<T> 是"可序列化的组件引用"：字段内只存目标组件的持久
//   UUID（64 位，随 .scene 落盘），get() 时经当前场景的 uuid → 组件索引
//   懒解析。目标组件被移除/所属对象销毁后，get() 返回 nullptr——永不悬垂。
//
//   用法（配合反射扫描器，字段自动被识别为引用字段）：
//     SHIT_FIELD
//     Shit::ComponentRef<Shit::UIText> coinText;   // 编辑器可拖拽赋引用
//
//     if (auto t = coinText.get()) t->setText("...");
//
//   与 WeakComponentRef 的区别：WeakComponentRef 是"会话期弱引用"
//   （存 owner + 生命周期令牌，不序列化）；ComponentRef 是"持久引用"
//   （存 UUID，序列化到 .scene，目标销毁自动失效）。
//
//   约束：引用仅限同一场景（UUID 索引挂在 Scene 上）；跨场景引用解析为 null。
// ═══════════════════════════════════════════════════════════════

#include <cstdint>

namespace Shit {

	class Scene;     // 前向声明
	class Component; // 前向声明（dynamic_cast 在实例化点解析）

	/// 内部：从当前上下文的活跃场景按 uuid 查组件（实现在 ComponentRef.cpp，跨 TU/DLL 共享）
	SHIT_API Component* ComponentRefLookup(uint64_t uuid);

	/**
	 * @brief 可序列化的组件引用（存 UUID，懒解析，自动失效）
	 * @tparam T 目标组件类型（须继承 Component 的反射类型）
	 */
	template <typename T>
	class ComponentRef {
	public:
		ComponentRef() = default;

		/// @brief 解析引用（当前场景的 uuid 索引查找 + 类型校验）
		/// 目标组件已移除/对象已销毁/类型不符/跨场景时返回 nullptr。
		T* get() const {
			if (!m_uuid) return nullptr;
			Component* comp = ComponentRefLookup(m_uuid);
			if (!comp) return nullptr;
			return dynamic_cast<T*>(comp);
		}

		/// @brief 便捷解引用（目标失效返回 nullptr，调用前可判空）
		T* operator->() const { return get(); }

		/// @brief 引用当前是否有效
		explicit operator bool() const { return get() != nullptr; }

		/// @brief 目标组件 UUID（0 = 空引用；编辑器/序列化用）
		uint64_t uuid() const { return m_uuid; }
		/// @brief 设置引用目标（传入 0 清空引用；编辑器拖拽赋引用用）
		void setUuid(uint64_t uuid) { m_uuid = uuid; }

		bool operator==(const ComponentRef& other) const { return m_uuid == other.m_uuid; }
		bool operator!=(const ComponentRef& other) const { return !(*this == other); }

	private:
		uint64_t m_uuid = 0; ///< 目标组件 UUID（0 = 空引用）
	};

} // namespace Shit