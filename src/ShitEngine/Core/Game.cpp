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
#include "ShitEngine/Audio/AudioPlayer.h"
#include "ShitEngine/Event/EventBus.h"
#include "ShitEngine/Core/TextInputGate.h"

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
		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
			ST_CORE_ERROR("SDL 初始化失败: {0}", SDL_GetError());
			return false;
		}

		// 初始化窗口
		if (!Window::Init()) return false;

		// 初始化渲染器
		if (!Renderer::Init()) return false;

		// 初始化 Time
		Time::Init();

		// 初始化资源管理器
		ResourceManager::Init();

		// 初始化音频播放器
		if (!AudioPlayer::Init()) {
			ST_CORE_WARN("音频系统初始化失败，音频功能将不可用");
		}

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
				TextInputGate::HandleEvent(event);
			}


			if (!Window::IsOpen()) break;

			//ST_CORE_DEBUG("测试");

			// 派发事件（缓冲队列在统一时刻处理，避免递归派发与迭代器失效）
			EventBus::ProcessEvents();

			SceneManager::Update();

			Input::Update(); // 更新 Input

			// 更新音频：清理已播完的 track，避免内存泄漏与 group 悬空指针
			AudioPlayer::Update();

			// 所有画面绘制（游戏世界 + UI）完成后统一刷新缓冲区
			Renderer::Present();
		}

		m_isRunning = false;
		ST_CORE_INFO("游戏已退出");
	}

	void Game::Destroy() {
		// 按依赖逆序清理子系统，避免静态单例在 SDL_Quit 后析构导致 UB
		EventBus::ClearAll();
		AudioPlayer::Destroy();
		Window::Destroy(); // 销毁窗口
		SDL_Quit();
	}

	Game& Game::GetInstance() {
		static Game instance;
		return instance;
	}
}
