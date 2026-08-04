#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"

#include "ShitEngine/Core/Window.h"

#include "ShitEngine/Core/Config.h"

namespace Shit {
	Window& Window::GetInstance()
	{
		return EngineContext::current().window;
	}

	void Window::Destroy()
	{
		if (IsOpen())
			Close();
	}

	bool Window::init() { // 初始化
		Uint32 flags = m_hidden ? SDL_WINDOW_HIDDEN : 0; // 离屏渲染（编辑器预览）：隐藏窗口但渲染正常
		m_window = std::unique_ptr<SDL_Window, SDLWindowDeleter>(SDL_CreateWindow(Config::GetWindowConfig().title.c_str(), Config::GetWindowConfig().width, Config::GetWindowConfig().height, flags));
		if (!m_window) {
			ST_CORE_ERROR("窗口创建失败: {0}", SDL_GetError());
			return false;
		}

		m_isOpen = true;

		return true;
	}

	void Window::handleEvent(const SDL_Event& event) {
		if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
			m_isOpen = false;  // 只设标志，不销毁窗口
		}
	}

	void Window::close() {
		m_isOpen = false;
		m_window.reset();  // SDL_DestroyWindow
	}
}