#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "../Component/Component.h"
#include <cstdint>

namespace Shit {
	/**
	 * @brief 盒子碰撞体组件
	 *
	 * 在同 GameObject 上添加一个矩形碰撞形状，附着于其 RigidBody2D。
	 * RigidBody2D 必须在 BoxCollider2D 之前添加。
	 *
	 * 使用方式：
	 *   auto* box = go->addComponent<Shit::BoxCollider2D>();
	 *   box->setSize({ 64, 64 });       // 像素尺寸
	 *   box->setDensity(1.0f);
	 *   box->setFriction(0.3f);
	 */
	class SHIT_API BoxCollider2D : public Component {
	public:
		BoxCollider2D();
		explicit BoxCollider2D(const Vector2& size);
		~BoxCollider2D() override;

		void onAttach() override;
		void onDetach() override;
		void onDestroy() override;

		// --- 配置（可在 onAttach 前后调用） ---
		void setSize(const Vector2& size);    ///< 像素尺寸（宽/高）默认 {32, 32}
		const Vector2& getSize() const { return m_size; }

		void setDensity(float density);       ///< 密度，默认 1.0f
		float getDensity() const { return m_density; }

		void setFriction(float friction);     ///< 摩擦系数，默认 0.3f
		float getFriction() const { return m_friction; }

		void setRestitution(float restitution); ///< 弹性系数，默认 0.0f
		float getRestitution() const { return m_restitution; }

	private:
		Vector2 m_size{ 32.0f, 32.0f };
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
