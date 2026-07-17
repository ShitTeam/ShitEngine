#pragma once
#include "Component.h"
#include "../Math.h"

namespace Shit {
	class GameObject;

	/**
	 * @brief 变换组件，决定 GameObject 的位置 / 缩放 / 旋转
	 *
	 * 每个 GameObject 默认不含 TransformComponent，需手动 addComponent。
	 * 位置单位为世界坐标，旋转单位为度。
	 */
	class SHIT_API TransformComponent : public Component {
		friend class GameObject;
	public:
		explicit TransformComponent();
		~TransformComponent() override = default;

		// --- getter & setter ---
		const Vector2& getPosition() const { return m_position; }
		void setPosition(const Vector2& position) { m_position = position; }

		const Vector2& getScale() const { return m_scale; }
		void setScale(const Vector2& scale) { m_scale = scale; }

		float getRotation() const { return m_rotation; }
		void setRotation(float rotation) { m_rotation = rotation; }

	private:
		Vector2 m_position{ 0.0f, 0.0f }; ///< 位置（世界坐标）
		Vector2 m_scale{ 1.0f, 1.0f };    ///< 缩放系数
		float m_rotation = 0.0f;           ///< 旋转角度（度）
	};
}