#pragma once
#include "Core.h"

namespace Shit {
	/**
	 * @brief 引擎主控类
	 *
	 * 管理引擎的初始化、主循环与销毁。
	 * 所有静态方法内聚为单例调用。
	 *
	 * 使用方式：
	 *   Game::Init();
	 *   Game::Run();
	 *   Game::Destroy();
	 */
	class SHIT_API Game {
	public:
		Game();
		~Game();

		bool init();    ///< 初始化引擎所有子系统
		void run();     ///< 启动主循环（阻塞直至窗口关闭）

		// --- 静态API ---
		static Game& GetInstance();
		inline static bool Init() { return GetInstance().init(); }
		inline static void Run() { GetInstance().run(); }
		static void Destroy();                      ///< 反初始化，按依赖逆序清理子系统
		static bool IsRunning() { return GetInstance().m_isRunning; } ///< 主循环是否仍在运行

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		Game(Game&&) = delete;
		Game& operator=(Game&&) = delete;

	private:
		bool m_isRunning = false;
		bool m_isInited = false;
	};
}