#pragma once
#include "../Core/Core.h"
#include "Component.h"
#include "../Math.h"
#include <SFML/Graphics.hpp>

namespace Shit {
	class SHIT_API CameraComponent : public Component {
	public:
		CameraComponent();

		// --- 生命周期 ---
		void onAttach() override;
		void onDestroy() override;

		// --- getter & setter ---
		// 获取 View
		sf::View &getView();

		const Vector2& getSize() const { return Math::ToGLM(m_view.getSize()); }

		int getPriority() const { return m_priority; };

		// 设置大小
		void setSize(Vector2& size) { m_view.setSize({ size.x, size.y }); }

		// 设置相机优先值
		void setPriority(int priority) { m_priority = priority; };

		// 设置缩放
		void setZoom(float zoom) { m_view.zoom(zoom); }

	private:
		sf::View m_view;
		int m_priority = 0; // 相机优先值
	};
}