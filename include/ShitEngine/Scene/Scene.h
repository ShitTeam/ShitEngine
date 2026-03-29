#pragma once
#include "../Core/Core.h"
#include "../Core/pch.h"

namespace Shit {
	// 前向声明
	class CameraComponent;
	class SceneManager;
	class GameObject;
	class Behavior;
	class RendererComponent;

	/**
	 * @brief 场景类
	 */
	class SHIT_API Scene {
	public:
		explicit Scene(const std::string& name);
		~Scene();

		// 禁止拷贝和移动构造
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		void update();
		void render();
		void clean();

		void addGameObject(std::unique_ptr<GameObject>&& gameObject);
		void removeGameObject(GameObject* gameObject); // 通过指针来删除游戏物体
		void removeGameObjectByName(const std::string& name); // 通过名称来删除游戏物体

		// 注册 Behavior 组件
		void registerBehavior(Behavior* behavior);
		void unregisterBehavior(Behavior* behavior);

		// 注册 RendererComponent 组件
		void registerRenderer(RendererComponent* renderer);
		void unregisterRenderer(RendererComponent* renderer);

		// 注册 CameraComponent 组件
		void registerCamera(CameraComponent* camera);
		void unregisterCamera(CameraComponent* camera);

		// --- getter & setter ---
		const std::string& getName() const { return m_name; }
		std::vector<std::unique_ptr<GameObject>>& getGameObjects() { return m_gameObjects; }

		void setName(const std::string& name) { m_name = name; }
	private:
		void processPendingAdditions(); // 处理延迟添加
		void processPendingBehaviors();

		std::string m_name; // 场景名称
		std::vector<std::unique_ptr<GameObject>> m_gameObjects; // 游戏物体
		std::vector<std::unique_ptr<GameObject>> m_pendingAdditions; // 延迟添加

		std::vector<Behavior*> m_behaviors; // 挂载的行为组件
		std::vector<Behavior*> m_pendingBehaviors; // 延迟注册 Behavior

		std::vector<RendererComponent*> m_renderers; // 挂载的渲染组件
		bool m_isRenderersNeedSort = false; // 是否需要排序

		std::vector<CameraComponent*> m_cameras; // 挂载的相机组件
		bool m_isCamerasNeedSort = false; // 是否需要排序
	};
}