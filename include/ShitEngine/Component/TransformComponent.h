#pragma once
#include "Component.h"
#include "../Math.h"

namespace Shit {
	class GameObject; // 前向声明

	/**
	 * @brief 变换组件
	 */
	class SHIT_API TransformComponent : public Component {
		friend class GameObject;
	public:
		explicit TransformComponent();
		~TransformComponent() override = default;

		// --- getter & setter ---
		inline const Vector2& getPosition() const { return m_position; }
		inline void setPosition(const Vector2& position) { m_position = position; }

		inline const Vector2& getScale() const { return m_scale; }
		inline void setScale(const Vector2& scale) { m_scale = scale; }

		inline float getRotation() const { return m_rotation; }
		inline void setRotation(const float rotation) { m_rotation = rotation; }
	private:
		Vector2 m_position; // 位置
		Vector2 m_scale; // 缩放
		float m_rotation; // 角度 (单位：度)
	};
}