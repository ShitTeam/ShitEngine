#include "ShitEngine/Game.h"

namespace Shit {
	Game::Game(const std::string& _title, const unsigned int& _width, const unsigned int& _height)
		: window(sf::VideoMode({ _width, _height }), _title)
	{
		window.setFramerateLimit(60); //设置帧率限制为60FPS

		//初始化日志系统
		Log::Init();
	}

	Game::~Game()
	{
	}

	void Game::Run()
	{
		while (window.isOpen())
		{
			sf::Time deltaTime = clock.restart(); //计算上一帧到当前帧的时间差

			Input();
			Update(deltaTime);
			Render();
		}
	}

	void Game::Input() //输入处理
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
				window.close();
		}
	}

	void Game::Update(sf::Time& deltaTime) //更新游戏数据
	{
		// 一些游戏逻辑更新代码可以放在这里
	}

	void Game::Render()
	{
		window.clear(sf::Color::Black); //清屏，设置背景色为黑色
		// 一些绘制代码可以放在这里
		window.display(); //显示渲染结果
	}
}