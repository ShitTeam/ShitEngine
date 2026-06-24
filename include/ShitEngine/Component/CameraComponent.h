#pragma once
#include "../Core/Core.h"
#include "Component.h"
#include "../Math.h"
#include <SDL3/SDL_rect.h>

namespace Shit {
	class SHIT_API CameraComponent : public Component {
	public:
		CameraComponent();

		// --- 生命周期 ---
		void onAttach() override;
		void onDestroy() override;

		// --- getter & setter ---
		Vector2 getPosition() const { return m_position; }
		Vector2 getSize() const { return m_size; }
		float getRotation() const { return m_rotation; }
		float getZoom() const { return m_zoom; }
		int getPriority() const { return m_priority; }

		void setPosition(const Vector2& pos) { m_position = pos; }
		void setSize(const Vector2& size) { m_size = size; }
		void setRotation(float rotation) { m_rotation = rotation; }
		void setZoom(float zoom) { m_zoom = zoom; }
		void setPriority(int priority) { m_priority = priority; }

	private:
		Vector2 m_position{ 0.0f, 0.0f };
		Vector2 m_size{ 1280.0f, 720.0f };
		float m_rotation = 0.0f;
		float m_zoom = 1.0f;
		int m_priority = 0; // 相机优先值
	};
}
