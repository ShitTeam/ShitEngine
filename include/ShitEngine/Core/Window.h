#pragma once
#include <memory>
#include <SDL3/SDL.h>
#include "Core.h"

struct SDL_Window;

namespace Shit {
	/**
	 * @brief 窗口类
	 */
	class SHIT_API Window {
	public:
		// --- 成员方法 ---
		bool init(); // 初始化

		// --- 静态API ---
		// 获取单例
		static Window& GetInstance();
		// 初始化
		static bool Init() { return GetInstance().init(); }
		// 获取Window
		static SDL_Window* GetWindow() { return GetInstance().m_window.get(); }
		// 处理事件
		static void HandleEvent(const SDL_Event& event) { GetInstance().handleEvent(event); }
		// 是否Window打开
		static bool IsOpen() { return GetInstance().isOpen(); }
		// 关闭Window
		static void Close() { GetInstance().close(); }
		// 销毁
		static void Destroy();

		Window() = default;
		~Window() = default;

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;

	private:
		void handleEvent(const SDL_Event& event); // 处理事件
		bool isOpen() { return m_isOpen; }
		void close();

		struct SDLWindowDeleter {
			void operator()(SDL_Window* window) const {
				if (window) {
					SDL_DestroyWindow(window);
				}
			}
		};
		
		std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window; // 窗口
		bool m_isOpen = false;
	};
}