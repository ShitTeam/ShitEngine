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
		inline static void Destroy() { GetInstance().destroy(); }    ///< 反初始化，按依赖逆序清理子系统（init 部分失败时也能安全调用）
		static bool IsRunning() { return GetInstance().m_isRunning; } ///< 是否处于运行态（主循环运行中，或编辑器显式置位）
		/// @brief 标记运行态。编辑器内嵌预览（外部驱动主循环，不调用 run()）在播放期间
		/// 置 true，使 Scene 增删走与 Runtime 一致的延时安全路径（帧末统一删除/添加），
		/// 游戏逻辑在迭代中删除对象不会使容器迭代器失效。停止播放时置回 false。
		static void SetIsRunning(bool running) { GetInstance().m_isRunning = running; }
		inline static void SetPaused(bool paused) { GetInstance().m_isPaused = paused; }  ///< 全局暂停（冻结 Behavior/物理，UI 叠层照常）
		inline static bool IsPaused() { return GetInstance().m_isPaused; }                ///< 是否处于暂停状态

		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		Game(Game&&) = delete;
		Game& operator=(Game&&) = delete;

	private:
		void destroy();  ///< 内部实现：按依赖逆序清理子系统

		bool m_isRunning = false;
		bool m_isInited = false;
		bool m_isPaused = false;  ///< 全局暂停状态
	};
}