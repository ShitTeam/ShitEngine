#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "../System/System.h"
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace Shit {
	class RigidBody2D;
	class Joint2D;
	class GameObject;

	/**
	 * @brief 2D 物理系统
	 *
	 * 包装 Box2D 3.x（C 语言 API）物理世界。
	 * 默认使用 b2SetLengthUnitsPerMeter(32) 以像素为长度单位，
	 * 用户无需手动转换像素↔米。
	 *
	 * 使用方式：
	 *   scene->registerSystem<Shit::PhysicsSystem2D>();
	 *
	 * 物理步进后按接触事件驱动冲突对象的 Behavior 碰撞回调：
	 *   onCollisionEnter / onCollisionStay / onCollisionExit(对方 GameObject)。
	 *
	 * priority=50，晚于 BehaviorSystem(0) 早于 RenderSystem(100)。
	 */
class SHIT_API SHIT_REFLECT(BlackList) PhysicsSystem2D final : public System {
			SHIT_REFLECT_BODY(PhysicsSystem2D)
		public:
			explicit PhysicsSystem2D(int priority = 50);
			~PhysicsSystem2D() override;

			void init() override;
			void update() override;
			void destroy() override;

			// 反射字段被检查器直写后回调（把重力等变更实时同步到 Box2D 世界）
			void onFieldChanged(const std::string& fieldName) override;

			// 组件认领：RigidBody2D（创建/销毁物理体）
			bool onComponentAttached(Component* component) override;
			void onComponentDetached(Component* component) override;

			// --- 配置（必须在 init() 之前调用） ---
			void setGravity(const Vector2& gravity);       ///< 重力（像素/秒²），默认 {0, 320}
			const Vector2& getGravity() const { return m_gravity; }

			/// @brief 设置像素↔米比例（必须在 init() 之前调用），默认 32
			void setPixelsPerMeter(float ppm);
			/// @brief 当前像素↔米比例（实例字段，各物理系统独立）
			float getPixelsPerMeter() const { return m_pixelsPerMeter; }

		// --- 供 RigidBody2D 内部调用 ---
		void registerRigidBody(RigidBody2D* body);
		void unregisterRigidBody(RigidBody2D* body);
		/// @brief 为 RigidBody2D 创建物理体（组件认领时调用，从 Transform 读初始位姿）
		void createRigidBody(RigidBody2D* body);
		/// @brief 销毁 RigidBody2D 的物理体并注销（组件卸下时调用）
		void destroyRigidBody(RigidBody2D* body);

		// --- 供 Joint2D 内部调用 ---
		/// @brief 注册关节组件（onAttach 时调用），并尝试创建 Box2D 关节
		void registerJoint(Joint2D* joint);
		/// @brief 注销关节组件（onDetach/onDestroy 时调用）
		void unregisterJoint(Joint2D* joint);
		/// @brief 为 Joint2D 创建 Box2D 关节（幂等；bodyA/bodyB 就绪才创建）
		void createJoint(Joint2D* joint);
		/// @brief 销毁 Joint2D 的 Box2D 关节并重置标志
		void destroyJoint(Joint2D* joint);
		/// @brief 重建 Box2D 关节（字段改动/类型切换/目标变更后调用）：销毁旧关节按当前字段重建
		void rebuildJoint(Joint2D* joint);

private:
			friend class RigidBody2D;

			/// @brief 接触对键（刚体指针规范化排序 a < b）。仅作集合身份比较，绝不解引用。
			struct ContactPair {
				const RigidBody2D* a = nullptr;
				const RigidBody2D* b = nullptr;
				bool operator==(const ContactPair& o) const { return a == o.a && b == o.b; }
			};
			struct ContactPairHash {
				size_t operator()(const ContactPair& p) const {
					std::hash<const void*> h;
					return h(p.a) ^ (h(p.b) << 1);
				}
			};

			/// @brief 碰撞回调阶段（对应 Behavior::onCollisionEnter/Stay/Exit）
			enum class CollisionPhase { Enter, Stay, Exit };

			/// @brief 刚体指针对规范化（a < b），保证 (a,b) 与 (b,a) 视为同一接触对
			static ContactPair makeContactPair(const RigidBody2D* a, const RigidBody2D* b);

			/// @brief 由 b2BodyId 的三个字段定位已注册的 RigidBody2D
			RigidBody2D* findRigidBody(int32_t bodyIndex, uint16_t world0, uint16_t generation) const;

			/// @brief 清除接触对集合中所有包含指定刚体的项（刚体/形状销毁时调用，防残留泄漏）
			void cleanupContactPairs(const RigidBody2D* body);

			/// @brief 收集当前子步的接触事件到 entered/exited（每个 b2World_Step 后调用：
			/// Box2D 的事件缓冲随步进清空，多子步帧必须逐步收集，否则前面子步的事件丢失）
			void collectContactEvents(std::vector<ContactPair>& entered, std::vector<ContactPair>& exited);

			/// @brief 把一对接触对象的碰撞回调派发给两个 GameObject 上已启动的 Behavior
			void dispatchContact(const ContactPair& pair, CollisionPhase phase);

			/// @brief 单个 GameObject 上的碰撞回调（逐轮重扫防回调内增删组件悬垂）
			void invokeCollisionCallbacks(GameObject* self, GameObject* other, CollisionPhase phase);

			SHIT_META(({.displayName = "Gravity", .tooltip = "重力（像素/秒²）"}))
			Vector2 m_gravity{ 0.0f, 320.0f };

			SHIT_META(Disable)
			std::vector<RigidBody2D*> m_bodies;
			SHIT_META(Disable)
			std::vector<Joint2D*> m_joints; ///< 已注册的关节组件（Box2D 关节句柄管理）
			// 当前接触中的刚体对集合（进入/持续/结束 基于它判定；元素仅作键，不触碰指针所指对象）
			SHIT_META(Disable)
			std::unordered_set<ContactPair, ContactPairHash> m_activeContacts;
			SHIT_META(Disable)
			float m_accumulator = 0.0f; // 物理固定步长累积器

			// b2WorldId = {uint16_t index1; uint16_t generation;}
			SHIT_META(Disable)
			uint16_t m_worldIndex = 0;
			SHIT_META(Disable)
			uint16_t m_worldGeneration = 0;
			SHIT_META(Disable)
			bool m_initialized = false;

			// 像素↔米比例（实例字段；init() 时应用 b2SetLengthUnitsPerMeter——
			// 该 API 是 Box2D 进程级全局，多实例不同比例时后者生效，属 Box2D 限制）
			SHIT_META(({.displayName = "Pixels Per Meter", .tooltip = "每米对应的像素数（默认 32，下次 init 生效）", .range = {1, 1024}, .step = 1}))
			float m_pixelsPerMeter = 32.0f;
	};
}