#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Config.h"

namespace Shit {
	Window& Window::GetInstance()
	{
		static Window instance;
		return instance;
	}

	void Window::init() { // 初始化
		m_window.create(sf::VideoMode({Config::GetWindowConfig().width, Config::GetWindowConfig().height }), Config::GetWindowConfig().title);
		m_window.setFramerateLimit(Config::GetWindowConfig().framerateLimit);
	}
}