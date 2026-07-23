#include "ShitEngine/Core/pch.h"

#include "ShitEngine/Core/Window.h"

#include "ShitEngine/Core/Config.h"

namespace Shit {
	Window& Window::GetInstance()
	{
		static Window instance;
		return instance;
	}

	void Window::Destroy()
	{
		if (IsOpen())
			Close();
	}

	bool Window::init() { // 初始化
		m_window = std::unique_ptr<SDL_Window, SDLWindowDeleter>(SDL_CreateWindow(Config::GetWindowConfig().title.c_str(), Config::GetWindowConfig().width, Config::GetWindowConfig().height, 0));
		if (!m_window) {
			ST_CORE_ERROR("窗口创建失败: {0}", SDL_GetError());
			return false;
		}

		m_isOpen = true;

		return true;
	}

	void Window::handleEvent(const SDL_Event& event) {
		if (event.type == SDL_EVENT_QUIT) {
			close();
		}
	}

	void Window::close() {
		m_isOpen = false;
		m_window.reset();
	}
}