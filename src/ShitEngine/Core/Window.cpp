#include "ShitEngine/Core/Window.h"

namespace Shit {
	Window& Window::GetInstance()
	{
		static Window instance;
		return instance;
	}

	void Window::init(unsigned int width, unsigned int height, std::string title) { // 初始化
		m_window.create(sf::VideoMode({width, height}), title);
	}
}