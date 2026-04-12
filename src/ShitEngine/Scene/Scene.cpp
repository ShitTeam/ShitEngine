#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Component/RendererComponent.h"
#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/System/BehaviorSystem.h"

namespace Shit {
	Scene::Scene(const std::string& name) : m_name(name) {
		ST_CORE_TRACE("场景 {} ：创建成功。", m_name);
	}
	
	Scene::~Scene() = default;

	void Scene::init() { // 场景初始化
		registerSystem<BehaviorSystem>();
		registerSystem<RenderSystem>();
	}

	void Scene::update() {
		if (m_isSystemsNeedSort) { // System 排序
			std::sort(m_systems.begin(), m_systems.end(), [](System* a, System* b) {
				return a->getPriority() < b->getPriority();
			});
			m_isSystemsNeedSort = false;
		}

		for (auto* system : m_systems) {
			system->update();
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
		processPendingRemoveSystems();
	}

	void Scene::destroy() {
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

	void Scene::processPendingRemoveSystems() {
		if (m_pendingRemoveSystems.empty()) return;

		for (const auto& type : m_pendingRemoveSystems) {
			auto it = m_systemsMap.find(type);
			if (it != m_systemsMap.end()) {
				auto system_ptr = it->second.get();

				system_ptr->destroy();

				m_systems.erase(
					std::remove(m_systems.begin(), m_systems.end(), system_ptr),
					m_systems.end()
				);
				m_systemsMap.erase(it);
			}
		}
		m_pendingRemoveSystems.clear();
	}
}
