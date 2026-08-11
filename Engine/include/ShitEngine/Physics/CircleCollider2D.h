#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "../Component/Component.h"
#include <cstdint>

namespace Shit {
	/**
	 * @brief 圆形碰撞体组件
	 *
	 * 在同 GameObject 上添加一个圆形碰撞形状，附着于其 RigidBody2D。
	 * 挂载顺序不敏感：若碰撞体先于 RigidBody2D 挂载，刚体创建后会补建碰撞形状
	 *（由 PhysicsSystem2D 在 createRigidBody 时调用 ensureShape）。
	 *
	 * 使用方式：
	 *   auto* ball = scene->createGameObject("Ball");
	 *   ball->addComponent<Shit::TransformComponent>()->setPosition({400, 100});
	 *   ball->addComponent<Shit::RigidBody2D>()->setBodyType(Shit::RigidBody2D::Type::Dynamic);
	 *   auto* collider = ball->addComponent<Shit::CircleCollider2D>();
	 *   collider->setRadius(24);
	 */
	class SHIT_API SHIT_REFLECT(BlackList) CircleCollider2D : public Component {
		SHIT_REFLECT_BODY(CircleCollider2D)
	public:
		CircleCollider2D();
		explicit CircleCollider2D(float radius);
		~CircleCollider2D() override;

		void onAttach() override;
		void onDetach() override;
		void onDestroy() override;

		/// @brief 内部：若同物体存在有效刚体则创建碰撞形状（幂等）。刚体后置挂载时由物理系统补调。
		void ensureShape();

		// --- 配置（可在 onAttach 前后调用） ---
		void setRadius(float radius);           ///< 像素半径，默认 16.0f
		float getRadius() const { return m_radius; }

		/// 检查器直写反射字段后回调：把半径改动同步到已创建的 Box2D 圆形状
		void onFieldChanged(const std::string& fieldName) override;

		void setDensity(float density);         ///< 密度，默认 1.0f
		float getDensity() const { return m_density; }

		void setFriction(float friction);       ///< 摩擦系数，默认 0.3f
		float getFriction() const { return m_friction; }

		void setRestitution(float restitution); ///< 弹性系数，默认 0.0f
		float getRestitution() const { return m_restitution; }

	private:
		SHIT_META(({.displayName = "Radius", .tooltip = "碰撞圆半径（像素）", .range = {1, 1024}, .step = 1}))
		float m_radius = 16.0f;
		SHIT_META(({.displayName = "Density", .range = {0, 100}, .step = 0.1}))
		float m_density = 1.0f;
		SHIT_META(({.displayName = "Friction", .tooltip = "摩擦系数", .range = {0, 1}, .step = 0.05}))
		float m_friction = 0.3f;
		SHIT_META(({.displayName = "Restitution", .tooltip = "弹性系数", .range = {0, 1}, .step = 0.05}))
		float m_restitution = 0.0f;

		// b2ShapeId = {int32_t index1; uint16_t world0; uint16_t generation;}
		SHIT_META(Disable)
		int32_t m_shapeIndex = 0;
		SHIT_META(Disable)
		uint16_t m_shapeWorld0 = 0;
		SHIT_META(Disable)
		uint16_t m_shapeGeneration = 0;
		SHIT_META(Disable)
		bool m_shapeValid = false;
	};
}
