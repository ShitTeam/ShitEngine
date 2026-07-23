#pragma once
#include "../Core/Core.h"

namespace Shit {
	class Scene; // 前向声明

	/**
	 * @brief 场景管理器（单例）
	 *
	 * 使用栈结构管理场景生命周期。任何时候只有栈顶场景处于活跃状态。
	 * 所有操作延迟到 Update() 时执行，保证迭代安全。
	 *
	 * 用法：
	 *   SceneManager::PushScene(std::move(myScene));
	 *   SceneManager::ReplaceScene(std::move(gameScene));
	 *   SceneManager::PopScene();
	 *   SceneManager::ClearScene();
	 */
	class SHIT_API SceneManager final {
	public:

		// 禁止拷贝和移动
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;
		SceneManager(SceneManager&&) = delete;
		SceneManager& operator=(SceneManager&&) = delete;

		// --- 静态API ---
		static SceneManager& GetInstance();
		inline static void Update() { GetInstance().update(); }                    ///< 更新当前场景
		inline static void Destroy() { GetInstance().destroy(); }                  ///< 销毁所有场景
		inline static void PushScene(std::unique_ptr<Scene>&& scene) { GetInstance().pushScene(std::move(scene)); }       ///< 压入新场景（下个帧循环生效）
		inline static void PopScene() { GetInstance().popScene(); }                ///< 弹出栈顶场景（下个帧循环生效）
		inline static void ClearScene() { GetInstance().clearScene(); }            ///< 清空场景栈
		inline static void ReplaceScene(std::unique_ptr<Scene>&& scene) { GetInstance().replaceScene(std::move(scene)); } ///< 替换栈顶场景

	private:
		explicit SceneManager();
		~SceneManager();

		void pushScene(std::unique_ptr<Scene>&& scene);
		void popScene();
		void clearScene();
		void replaceScene(std::unique_ptr<Scene>&& scene);

		void update();
		void destroy();

		void processPendingActions();
		void processPushScene(std::unique_ptr<Scene>&& scene);
		void processPopScene();
		void processClearScene();
		void processReplaceScene(std::unique_ptr<Scene>&& scene);

		Scene* getCurrentScene() const;

		enum class StackAction { None, Push, Pop, Clear, Replace };
		struct PendingAction {
			StackAction action;
			std::unique_ptr<Scene> scene;
		};

		std::vector<std::unique_ptr<Scene>> m_sceneStack;
		std::vector<PendingAction> m_pendingActions;
	};
}