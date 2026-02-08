#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Input.h"

namespace Shit {
	Game::Game() = default;

	Game::~Game() = default;

	void Game::init(unsigned int width, unsigned int height, std::string title)
	{
		// 初始化日志系统
		Log::Init();

		// 初始化窗口
		Window::Init(width, height, std::move(title));

		Window::GetWindow().setFramerateLimit(60);
	}

	void Game::run()
	{
		ST_CORE_INFO("游戏开始运行");

		while (Window::GetWindow().isOpen())
		{
			Time::Update(); //计算上一帧到当前帧的时间差

			Input::Update(); // 更新 Input

			Window::GetWindow().handleEvents([this](const auto& type) {handleEvent(type); });

			if (!Window::GetWindow().isOpen()) break;

			update();
			render();
		}

		ST_CORE_INFO("游戏已退出");
	}

	void Game::handleEvent(const auto&) {} // 默认事件

	void Game::handleEvent(const sf::Event::Closed&) // 窗口关闭事件
	{
		Window::GetWindow().close();
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

	}

	void Game::render()
	{
		Window::GetWindow().clear(sf::Color::Black); //清屏，设置背景色为黑色
		// 一些绘制代码可以放在这里
		Window::GetWindow().display(); //显示渲染结果
	}
}