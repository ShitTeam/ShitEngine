#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Component/RendererComponent.h"
#include "ShitEngine/Render/RenderSystem.h"

namespace Shit {
	Scene::Scene(const std::string& name) : m_name(name) {
		ST_CORE_TRACE("场景 {} ：创建成功。", m_name);
	}
	
	Scene::~Scene() = default;

	void Scene::update() {
		processPendingBehaviors();

		for (auto& b : m_behaviors) {
			if (!b->isStarted()) {
				b->onStart(); // 启动组件
				b->setStarted(true);
			}
			b->onUpdate();
		}

		// 删除需要销毁的游戏对象
		auto it = std::remove_if(m_gameObjects.begin(), m_gameObjects.end(), [](const std::unique_ptr<GameObject>& go) {
			if (go && go->isNeedDestroy()) {
				go->clean();
				return true;
			}
			return false;
			});

		if (it != m_gameObjects.end()) {
			m_gameObjects.erase(it, m_gameObjects.end());
		}

		processPendingAdditions();
	}

	void Scene::render() {
		// 按照 Priority 从小到大排序
		if (m_isCamerasNeedSort) {
			std::sort(m_cameras.begin(), m_cameras.end(), [](CameraComponent* a, CameraComponent* b) {
				return a->getPriority() < b->getPriority();
			});
			m_isCamerasNeedSort = false;
		}

		// 按照 Z-Index 排序，小的在底部
		if (m_isRenderersNeedSort) {
			std::sort(m_renderers.begin(), m_renderers.end(),[](RendererComponent* a, RendererComponent* b) {
				return a->getZIndex() < b->getZIndex();
			});
			m_isRenderersNeedSort = false;
		}

		for (auto* camera : m_cameras) {
			RenderSystem::SetView(camera->getView());

			for (auto* renderer : m_renderers) {
				renderer->onRender();
			}
		}
	}

	void Scene::clean() {
		for (auto& go : m_gameObjects) {
			if (go) go->clean();
		}

		m_gameObjects.clear();

		ST_CORE_TRACE("场景 {} 已清除", m_name);
	}

	void Scene::addGameObject(std::unique_ptr<GameObject>&& gameObject)
	{
		if (gameObject) {
			if (Game::IsRunning()) { // 如果游戏正在运行，则使用延时添加
				m_pendingAdditions.push_back(std::move(gameObject));
			}
			else { // 否则，直接添加
				gameObject->setScene(this);
				m_gameObjects.push_back(std::move(gameObject));
			}
		}
		else {
			ST_CORE_WARN("试图向场景 {} 中添加空游戏对象！", m_name);
		}
	}

	void Scene::removeGameObject(GameObject* gameObject) {
		if (!gameObject) {
			ST_CORE_WARN("试图从场景 {} 中移除一个空的游戏对象指针！", m_name);
			return;
		}

		if (Game::IsRunning()) { // 如果正在运行，则使用更安全的移除
			gameObject->setNeedDestroy(true);
		}
		else {
			auto it = std::remove_if(m_gameObjects.begin(), m_gameObjects.end(), [&gameObject](const std::unique_ptr<GameObject>& go) {
				return go.get() == gameObject;
				});

			if (it != m_gameObjects.end()) {
				(*it)->clean();
				m_gameObjects.erase(it, m_gameObjects.end());
			}
			else {
				ST_CORE_WARN("场景 {} 中没有找到对应的游戏对象 ！", m_name);
			}
		}
	}

	void Scene::removeGameObjectByName(const std::string& name)
	{
		if (Game::IsRunning()) {
			for (auto& go : m_gameObjects) {
				if (go->getName() == name) {
					go->setNeedDestroy(true);
				}
			}
		}
		else {
			auto it = std::remove_if(m_gameObjects.begin(), m_gameObjects.end(), [&name](const std::unique_ptr<GameObject>& go) {
				return go->getName() == name;
				});

			if (it != m_gameObjects.end()) {
				(*it)->clean();
				m_gameObjects.erase(it, m_gameObjects.end());
			}
			else {
				ST_CORE_WARN("没有在场景 {} 中找到名称为 {} 的游戏对象！", m_name, name);
			}
		}
	}

	void Scene::processPendingAdditions()
	{
		for (auto& go : m_pendingAdditions) {
			if(go) {
				go->setScene(this);
				m_gameObjects.push_back(std::move(go));
			}
			else ST_CORE_WARN("试图向场景 {} 中添加空游戏对象！", m_name);
		}
		m_pendingAdditions.clear();
	}

	void Scene::processPendingBehaviors()
	{
		for (auto& b : m_pendingBehaviors) {
			if (b) m_behaviors.push_back(b);
			else ST_CORE_WARN("试图注册 Behavior 空指针！");
		}
		m_pendingBehaviors.clear();
	}

	void Scene::registerBehavior(Behavior* behavior)
	{
		if (!behavior) {
			ST_CORE_WARN("试图在场景 {} 中注册 Behavior 空指针！", m_name);
			return;
		}

		m_pendingBehaviors.push_back(behavior);
	}

	void Scene::unregisterBehavior(Behavior* behavior)
	{
		if (!behavior) {
			ST_CORE_WARN("试图在场景 {} 中移除 Behavior 空指针！", m_name);
			return;
		}

		// 如果在延迟添加列表内
		m_pendingBehaviors.erase(
			std::remove(m_pendingBehaviors.begin(), m_pendingBehaviors.end(), behavior),
			m_pendingBehaviors.end()
		);

		// 从列表内删除
		m_behaviors.erase(
			std::remove(m_behaviors.begin(), m_behaviors.end(), behavior),
			m_behaviors.end()
		);
	}

	void Scene::registerRenderer(RendererComponent* renderer)
	{
		if (!renderer) {
			ST_CORE_WARN("试图在场景 {} 中注册 RendererComponent 空指针！", m_name);
			return;
		}

		m_renderers.push_back(renderer);
		m_isRenderersNeedSort = true; // 有新 RendererComponent 插入，需要排序
	}

	void Scene::unregisterRenderer(RendererComponent* renderer)
	{
		if (!renderer) {
			ST_CORE_WARN("试图在场景 {} 中移除 RendererComponent 空指针！", m_name);
			return;
		}

		// 从列表内删除
		m_renderers.erase(
			std::remove(m_renderers.begin(), m_renderers.end(), renderer),
			m_renderers.end()
		);
	}

	void Scene::registerCamera(CameraComponent *camera) {
		if (!camera) {
			ST_CORE_WARN("试图在场景 {} 中注册 CameraComponent 空指针！", m_name);
			return;
		}

		m_cameras.push_back(camera);
		m_isCamerasNeedSort = true;
	}

	void Scene::unregisterCamera(CameraComponent *camera) {
		if (!camera) {
			ST_CORE_WARN("试图在场景 {} 中移除 CameraComponent 空指针！", m_name);
			return;
		}

		m_cameras.erase(
			std::remove(m_cameras.begin(), m_cameras.end(), camera),
			m_cameras.end()
			);
	}
}
