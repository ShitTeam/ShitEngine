#include "ShitEngine/Core/Game.h"

namespace Shit {
	Game::Game(const std::string& _title, const unsigned int& _width, const unsigned int& _height)
		: m_window(sf::VideoMode({ _width, _height }), _title)
	{
		m_window.setFramerateLimit(60); //设置帧率限制为60FPS

		//初始化日志系统
		Log::Init();
	}

	Game::~Game()
	{
	}

	void Game::run()
	{
		while (m_window.isOpen())
		{
			Time::Update(); //计算上一帧到当前帧的时间差

			input();
			update();
			render();
		}
	}

	void Game::input() //输入处理
	{
		while (const std::optional event = m_window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
				m_window.close();
		}
	}

	void Game::update() //更新游戏数据
	{
		ST_CORE_DEBUG(Time::GetDeltaTime());
	}

	void Game::render()
	{
		m_window.clear(sf::Color::Black); //清屏，设置背景色为黑色
		// 一些绘制代码可以放在这里
		m_window.display(); //显示渲染结果
	}
}