#pragma once
#include "Core.h"

namespace Shit {
	class SHIT_API Game {
	public:
		Game();
		~Game();

		//初始化游戏
		bool init();

		//启动游戏
		void run();

		// --- 静态API ---
		static Game& GetInstance();
		inline static bool Init() { return GetInstance().init(); }
		inline static void Run() { GetInstance().run(); }
		static void Destroy(); // 销毁函数
		static bool IsRunning() { return GetInstance().m_isRunning; }

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		Game(Game&&) = delete;
		Game& operator=(Game&&) = delete;

	private:
		bool m_isRunning = false;
		bool m_isInited = false;
	};
}