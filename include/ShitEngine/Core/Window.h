#pragma once
#include <memory>
#include "Core.h"
#include <SFML/Graphics.hpp>

namespace Shit {
	/**
	 * @brief 窗口类
	 */
	class SHIT_API Window {
	public:
		// --- 成员方法 ---
		bool init(); // 初始化

		// --- 静态API ---
		// 初始化
		inline static bool Init() { return GetInstance().init(); }
		// 获取Window
		inline static sf::RenderWindow* GetWindow() { return GetInstance().m_window.get(); }
		// 获取单例
		static Window& GetInstance();
		// 销毁
		static void Destroy();

		Window() = default;
		~Window() = default;

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;

	private:

		std::unique_ptr<sf::RenderWindow> m_window; // 渲染窗口
	};
}