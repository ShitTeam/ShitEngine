#pragma once
#include "../Core/Core.h"

namespace Shit {
	class Scene; // 前向声明

	/**
	 * @brief 场景管理器
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
		inline static void Update() { GetInstance().update(); }
		inline static void Destroy() { GetInstance().destroy(); }
		inline static void PushScene(std::unique_ptr<Scene>&& scene) { GetInstance().pushScene(std::move(scene)); }
		inline static void PopScene() { GetInstance().popScene(); }
		inline static void ClearScene() { GetInstance().clearScene(); }
		inline static void ReplaceScene(std::unique_ptr<Scene>&& scene) { GetInstance().replaceScene(std::move(scene)); }

	private:
		explicit SceneManager();
		~SceneManager();

		void pushScene(std::unique_ptr<Scene>&& scene); // 压入一个新场景
		void popScene();                                // 弹出一个场景
		void clearScene();                              // 清空场景栈
		void replaceScene(std::unique_ptr<Scene>&& scene); // 清空场景栈并压入一个新场景

		void update();
		void destroy();

		void processPendingActions(); // 处理延迟行为
		void processPushScene(std::unique_ptr<Scene>&& scene);
		void processPopScene();
		void processClearScene();
		void processReplaceScene(std::unique_ptr<Scene>&& scene);

		// --- getter & setter ---
		Scene* getCurrentScene() const;

		enum class StackAction { None, Push, Pop, Clear, Replace }; // 栈行为枚举类
		struct PendingAction
		{
			StackAction action;
			std::unique_ptr<Scene> scene;
		};

		std::vector<std::unique_ptr<Scene>> m_sceneStack;
		std::vector<PendingAction> m_pendingActions; // 延迟行为队列
	};
}