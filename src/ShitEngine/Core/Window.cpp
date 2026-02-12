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
		if (GetInstance().m_window->isOpen()) {
			GetInstance().m_window->close();
		}

		GetInstance().m_window.reset();
	}

	bool Window::init() { // 初始化
		m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode({ Config::GetWindowConfig().width, Config::GetWindowConfig().height }), Config::GetWindowConfig().title);
		m_window->setFramerateLimit(Config::GetWindowConfig().framerateLimit);

		if (!m_window->isOpen()) {
			ST_CORE_ERROR("窗口创建失败");
			return false;
		}

		return true;
	}
}