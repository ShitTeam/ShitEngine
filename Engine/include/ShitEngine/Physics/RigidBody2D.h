#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "../Component/Component.h"
#include <cstdint>

namespace Shit {
	class PhysicsSystem2D;

	/**
	 * @brief 刚体组件
	 *
	 * 为 GameObject 提供物理刚体属性。必须与 TransformComponent 同挂一个
	 * GameObject 上（由 onAttach 从中读取初始位置）。
	 *
	 * 使用方式：
	 *   auto* body = go->addComponent<Shit::RigidBody2D>();
	 *   body->setBodyType(Shit::RigidBody2D::Type::Dynamic);
	 *   body->setGravityScale(1.0f);
	 *
	 * 创建 Collider 前必须先挂 RigidBody2D（BoxCollider2D/CircleCollider2D
	 * 的 onAttach 会自动查找同 GameObject 上的 RigidBody2D）。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) RigidBody2D : public Component {
		SHIT_REFLECT_BODY(RigidBody2D)
	public:
		enum class Type {
			Static    = 0, ///< 不受力，不可移动（默认）
			Kinematic = 1, ///< 用户控制位置，可与 Dynamic 碰撞
			Dynamic   = 2, ///< 受重力/力/碰撞影响
		};

		RigidBody2D();
		~RigidBody2D() override;

		void onCreate() override;
		void onAttach() override;
		void onDetach() override;
		void onDestroy() override;

		// --- 属性 ---
		void setBodyType(Type type);
		Type getBodyType() const { return m_type; }

		void setGravityScale(float scale) { m_gravityScale = scale; }
		float getGravityScale() const { return m_gravityScale; }

		void setLinearDamping(float damping) { m_linearDamping = damping; }
		float getLinearDamping() const { return m_linearDamping; }

		void setFixedRotation(bool fixed) { m_fixedRotation = fixed; }
		bool isFixedRotation() const { return m_fixedRotation; }

		// --- 力的接口 ---
		void applyForce(const Vector2& force, bool wake = true);
		void applyForceToCenter(const Vector2& force, bool wake = true);
		void applyImpulse(const Vector2& impulse, bool wake = true);
		void setLinearVelocity(const Vector2& velocity);
		Vector2 getLinearVelocity() const;

		/// @brief 物理体是否已创建
		bool hasValidBody() const { return m_bodyValid; }

		// --- 供 BoxCollider2D / CircleCollider2D 内部使用 ---
		int32_t getBodyIndex() const { return m_bodyIndex; }
		uint16_t getBodyWorld0() const { return m_bodyWorld0; }
		uint16_t getBodyGeneration() const { return m_bodyGeneration; }

	private:
		friend class PhysicsSystem2D;

		// b2BodyId = {int32_t index1; uint16_t world0; uint16_t generation;}
		SHIT_META(Disable)
		int32_t m_bodyIndex = 0;
		SHIT_META(Disable)
		uint16_t m_bodyWorld0 = 0;
		SHIT_META(Disable)
		uint16_t m_bodyGeneration = 0;
		SHIT_META(Disable)
		bool m_bodyValid = false;

		SHIT_META(({.displayName = "Body Type", .tooltip = "Static=不动, Kinematic=用户控制, Dynamic=物理驱动"}))
		Type m_type = Type::Static;
		SHIT_META(({.displayName = "Gravity Scale", .tooltip = "重力影响系数", .range = {0, 10}, .step = 0.1}))
		float m_gravityScale = 1.0f;
		SHIT_META(({.displayName = "Linear Damping", .tooltip = "线性阻尼", .range = {0, 10}, .step = 0.1}))
		float m_linearDamping = 0.0f;
		SHIT_META(({.displayName = "Fixed Rotation", .tooltip = "锁定旋转"}))
		bool m_fixedRotation = false;
	};
}
