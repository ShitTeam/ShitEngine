#pragma once
#include <memory>
#include <SDL3/SDL.h>
#include "Core.h"

struct SDL_Window;

namespace Shit {
	/**
	 * @brief 窗口管理类
	 *
	 * 封装 SDL_Window 的生命周期，提供窗口事件处理与状态查询。
	 */
	class SHIT_API Window {
	public:
		Window() = default;
		~Window() = default;

		// --- 静态API ---
		static Window& GetInstance();                                   ///< 获取单例
		static bool Init() { return GetInstance().init(); }             ///< 初始化窗口
		static SDL_Window* GetWindow() { return GetInstance().m_window.get(); } ///< 获取原生 SDL_Window 指针
		static void HandleEvent(const SDL_Event& event) { GetInstance().handleEvent(event); } ///< 分发窗口事件给子系统
		static bool IsOpen() { return GetInstance().isOpen(); }         ///< 窗口是否仍然打开
		static void Close() { GetInstance().close(); }                  ///< 关闭窗口
		static void Destroy();                                          ///< 销毁窗口及资源

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;

	private:
		bool init();
		void handleEvent(const SDL_Event& event);
		bool isOpen() { return m_isOpen; }
		void close();

		struct SDLWindowDeleter {
			void operator()(SDL_Window* window) const {
				if (window) SDL_DestroyWindow(window);
			}
		};

		std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window;
		bool m_isOpen = false;
	};
}