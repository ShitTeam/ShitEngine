#pragma once
#include "pch.h"
#include "Core.h"
#include <SFML/Graphics.hpp>

namespace Shit {
	/**
	 * @brief 窗口类
	 */
	class SHIT_API Window {
	public:
		// --- 成员方法 ---
		void init(unsigned int width, unsigned int height, std::string title); // 初始化
		inline sf::RenderWindow& getWindow() { return m_window; }

		// --- 静态API ---
		// 初始化
		inline static void Init(unsigned int width, unsigned int height, std::string title) { GetInstance().init(width, height, std::move(title)); }
		// 获取Window
		inline static sf::RenderWindow& GetWindow() { return GetInstance().m_window; }
		// 获取单例
		static Window& GetInstance();
	private:
		Window() = default;
		~Window() = default;

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;

		sf::RenderWindow m_window; // 渲染窗口
	};
}