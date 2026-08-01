#pragma once
#include "../Core/Core.h"
#include <memory>

namespace Shit {
	class Scene; // 前向声明

	/**
	 * @brief 场景管理器（单例）
	 *
	 * 持有当前活跃场景（单一场景模型，与 Unity/Godot 一致）：
	 *   - LoadScene() 切换场景：销毁旧场景、加载新场景（同帧生效）
	 *   - 暂停用 Game::SetPaused()：冻结 Behavior/物理，UI 叠层照常响应
	 *
	 * 用法：
	 *   Shit::SceneManager::LoadScene(std::move(levelScene));
	 *   Shit::Game::SetPaused(true);   // 暂停（冻结行为/物理，UI 菜单照常）
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
		inline static void LoadScene(std::unique_ptr<Scene>&& scene) { GetInstance().loadScene(std::move(scene)); }  ///< 切换场景（销毁当前，加载新场景；update 期间调用时延迟到 update 结束后生效）
		inline static void Update() { GetInstance().update(); }
		inline static void Destroy() { GetInstance().destroy(); }
		inline static Scene* GetCurrentScene() { return GetInstance().getCurrentScene(); }

	private:
		friend class EngineContext;
		explicit SceneManager();
		~SceneManager();

		void loadScene(std::unique_ptr<Scene>&& scene);
		void applyLoadScene(std::unique_ptr<Scene>&& scene);  ///< 实际执行切换（销毁旧场景 + 自动 init + 装载）
		void update();
		void destroy();
		Scene* getCurrentScene() const;

		std::unique_ptr<Scene> m_currentScene;  ///< 当前活跃场景（唯一）
		std::unique_ptr<Scene> m_pendingScene;  ///< 场景 update 期间请求的延迟切换（帧末生效，避免 UAF）
		bool m_isUpdating = false;              ///< 当前是否正在 update 当前场景
	};
}
