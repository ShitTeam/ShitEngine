#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Input.h"
#include "ShitEngine/Core/Config.h"

namespace Shit {
	Game::Game() = default;

	Game::~Game() = default;

	bool Game::init()
	{
		// 初始化日志系统
		if (!Log::Init()) return false;

		// 初始化配置
		if (!Config::Init()) return false;

		// 初始化窗口
		if (!Window::Init()) return false;

		return true;
	}

	void Game::run()
	{
		ST_CORE_INFO("游戏开始运行");

		while (Window::GetWindow()->isOpen())
		{
			Time::Update(); //计算上一帧到当前帧的时间差


			Window::GetWindow()->handleEvents([this](const auto& type) {this->handleEvent(type); });

			if (!Window::GetWindow()->isOpen()) break;

			//ST_CORE_DEBUG("测试");

			update();

			Input::Update(); // 更新 Input

			render();
		}

		ST_CORE_INFO("游戏已退出");
	}

	void Game::handleEvent(const auto&) {} // 默认事件

	void Game::handleEvent(const sf::Event::Closed&) // 窗口关闭事件
	{
		Window::GetWindow()->close();
		//ST_CORE_DEBUG("窗口关闭");
	}

	void Game::handleEvent(const sf::Event::KeyPressed& keyPressed) // 按键被按下
	{
		Input::HandleEvent(keyPressed);
		//ST_CORE_DEBUG("按键被按下");
	}

	void Game::handleEvent(const sf::Event::KeyReleased& keyReleased) // 按键被释放
	{
		Input::HandleEvent(keyReleased);
		//ST_CORE_DEBUG("按键被释放");
	}

	void Game::handleEvent(const sf::Event::MouseButtonPressed& mouseButtonPressed) // 鼠标按键被按下
	{
		Input::HandleEvent(mouseButtonPressed);
	}

	void Game::handleEvent(const sf::Event::MouseButtonReleased& mouseButtonReleased)
	{
		Input::HandleEvent(mouseButtonReleased);
	}

	void Game::update() //更新游戏数据
	{
		/*if (Input::IsKeyDown(Mouse::Left))
			ST_CORE_DEBUG("test");*/
	}

	void Game::render()
	{
		Window::GetWindow()->clear(sf::Color::Black); //清屏，设置背景色为黑色
		// 一些绘制代码可以放在这里
		Window::GetWindow()->display(); //显示渲染结果
	}

	void Game::Destroy() {
		Window::Destroy(); // 销毁窗口
	}

	Game& Game::GetInstance() {
		static Game instance;
		return instance;
	}
}