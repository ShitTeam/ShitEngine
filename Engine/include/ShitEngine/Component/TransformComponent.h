#pragma once
#include "Component.h"
#include "../Math.h"

namespace Shit {
	class GameObject;

	/**
	 * @brief 变换组件，决定 GameObject 的位置 / 缩放 / 旋转
	 */
	class SHIT_API SHIT_REFLECT(BlackList) TransformComponent : public Component {
		friend class GameObject;
		SHIT_REFLECT_BODY(TransformComponent)
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
		SHIT_META(({.displayName = "Position"}))
		Vector2 m_position{ 0.0f, 0.0f };
		SHIT_META(({.displayName = "Scale", .range = {0, 10}, .step = 0.1}))
		Vector2 m_scale{ 1.0f, 1.0f };
		SHIT_META(({.displayName = "Rotation", .range = {-360, 360}}))
		float m_rotation = 0.0f;
	};
}