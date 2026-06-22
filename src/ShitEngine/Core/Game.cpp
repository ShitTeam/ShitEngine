#include "ShitEngine/Core/pch.h"

#include "ShitEngine/Core/Game.h"

#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Input/Input.h"
#include "ShitEngine/Core/Config.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Scene/SceneManager.h"

namespace Shit {
	Game::Game() = default;
	Game::~Game() = default;

	bool Game::init()
	{
		// 初始化日志系统
		if (!Log::Init()) return false;

		// 初始化配置
		if (!Config::Init()) return false;

		// 初始SDL3
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			ST_CORE_ERROR("SDL 初始化失败: {0}", SDL_GetError());
			return false;
		}

		// 初始化窗口
		if (!Window::Init()) return false;

		// 初始化 Time
		Time::Init();

		// 初始化资源管理器
		ResourceManager::Init();

		return true;
	}

	void Game::run()
	{
		SDL_Event event;
		m_isRunning = true;
		ST_CORE_INFO("游戏开始运行");

		while (Window::IsOpen())
		{
			Time::Update(); //计算上一帧到当前帧的时间差


			// 获取SDL3的Event
			while (SDL_PollEvent(&event))
			{
				Window::HandleEvent(event);
				Input::HandleEvent(event);
			}
			

			if (!Window::IsOpen()) break;

			//ST_CORE_DEBUG("测试");

			SceneManager::Update();

			Input::Update(); // 更新 Input

			Renderer::LimitFPS(); // 限制帧率
		}

		m_isRunning = false;
		ST_CORE_INFO("游戏已退出");
	}

	void Game::Destroy() {
		Window::Destroy(); // 销毁窗口
	}

	Game& Game::GetInstance() {
		static Game instance;
		return instance;
	}
}