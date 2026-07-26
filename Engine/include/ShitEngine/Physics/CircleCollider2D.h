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
	 * RigidBody2D 必须在 CircleCollider2D 之前添加。
	 *
	 * 使用方式：
	 *   auto* ball = scene->createGameObject("Ball");
	 *   ball->addComponent<Shit::TransformComponent>()->setPosition({400, 100});
	 *   ball->addComponent<Shit::RigidBody2D>()->setBodyType(Shit::RigidBody2D::Type::Dynamic);
	 *   auto* collider = ball->addComponent<Shit::CircleCollider2D>();
	 *   collider->setRadius(24);
	 */
	class SHIT_API CircleCollider2D : public Component {
	public:
		CircleCollider2D();
		explicit CircleCollider2D(float radius);
		~CircleCollider2D() override;

		void onAttach() override;
		void onDetach() override;
		void onDestroy() override;

		// --- 配置（可在 onAttach 前后调用） ---
		void setRadius(float radius);           ///< 像素半径，默认 16.0f
		float getRadius() const { return m_radius; }

		void setDensity(float density);         ///< 密度，默认 1.0f
		float getDensity() const { return m_density; }

		void setFriction(float friction);       ///< 摩擦系数，默认 0.3f
		float getFriction() const { return m_friction; }

		void setRestitution(float restitution); ///< 弹性系数，默认 0.0f
		float getRestitution() const { return m_restitution; }

	private:
		float m_radius = 16.0f;
		float m_density = 1.0f;
		float m_friction = 0.3f;
		float m_restitution = 0.0f;

		// b2ShapeId = {int32_t index1; uint16_t world0; uint16_t generation;}
		int32_t m_shapeIndex = 0;
		uint16_t m_shapeWorld0 = 0;
		uint16_t m_shapeGeneration = 0;
		bool m_shapeValid = false;
	};
}
