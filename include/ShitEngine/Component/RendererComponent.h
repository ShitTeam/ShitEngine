#pragma once
#include "../Core/Core.h"
#include "Component.h"

namespace Shit {
	/**
	 * @brief 渲染组件基类
	 * 
	 * 所有的渲染组件必须继承自 RendererComponent ，并重写 onRender 函数
	 */
	class SHIT_API RendererComponent : public Component {
	public:
		RendererComponent();
		~RendererComponent() override = default;

		void onAttach() override;
		virtual void onRender() const = 0; // 渲染虚函数
		void onDestroy() override;

		// --- getter & setter ---
		int getZIndex() const { return m_zIndex; }
		bool isVisible() const { return m_isVisible; }

		void setZIndex(int zIndex) { m_zIndex = zIndex; }
		void setVisible(bool isVisible) { m_isVisible = isVisible; }
		
	protected:
		int m_zIndex; // 渲染顺序
		bool m_isVisible; // 是否显示
	};
}