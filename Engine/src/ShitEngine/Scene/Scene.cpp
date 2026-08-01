#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/GameObject/Prefab.h"
#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Component/RendererComponent.h"
#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/System/BehaviorSystem.h"
#include "ShitEngine/UI/UIRenderSystem.h"

namespace Shit {
	Scene::Scene(const std::string& name) : m_name(name) {
		ST_CORE_TRACE("场景 {} ：创建成功。", m_name);
	}

	Scene::~Scene() = default;

	void Scene::init() { // 场景初始化
		if (m_isInited) return;  // 幂等：防止 SceneManager 自动 init 与手动 init 重复注册
		m_isInited = true;

		registerSystem<BehaviorSystem>();
		registerSystem<RenderSystem>();
		registerSystem<UIRenderSystem>();
	}

	void Scene::update() {
		if (m_isSystemsNeedSort) { // System 排序
			std::sort(m_systems.begin(), m_systems.end(), [](System* a, System* b) {
				return a->getPriority() < b->getPriority();
			});
			m_isSystemsNeedSort = false;
		}

		for (size_t i = 0; i < m_systems.size(); ++i) {
			// 用下标迭代并读取实时 size()：若某系统 update 中动态注册新系统，
			// push_back 不会使下标失效；用实时 size() 保证遍历完所有已注册系统。
			// 新注册的系统在下标范围内会被本轮执行（若不想本轮执行，可在入口捕获 size）。
			if (m_systems[i]) {
				m_systems[i]->update();
			}
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
		// 先清理所有待添加的对象
		for (auto& go : m_pendingAdditions) {
			if (go) go->clean();
		}
		m_pendingAdditions.clear();

		// 清理游戏对象
		for (auto& go : m_gameObjects) {
			if (go) go->clean();
		}
		m_gameObjects.clear();

		// 销毁所有系统
		for (auto& [type, system] : m_systemsMap) {
			if (system) system->destroy();
		}
		m_systemsMap.clear();
		m_systems.clear();
		m_pendingRemoveSystems.clear();

		// 重置幂等守卫：场景被销毁后若复用（重新 loadScene 时 hasSystems()==false 会自动 init），
		// 不能因 m_isInited 残留 true 而跳过系统注册。
		m_isInited = false;

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

	GameObject* Scene::createGameObject(const std::string& name) {
		auto go = std::unique_ptr<GameObject>(new GameObject(name));
		go->setScene(this);
		auto* ptr = go.get();
		if (Game::IsRunning()) {
			m_pendingAdditions.push_back(std::move(go));
		} else {
			m_gameObjects.push_back(std::move(go));
		}
		return ptr;
	}

	GameObject* Scene::instantiate(const Prefab& prefab, const std::string& name) {
		auto goName = name.empty() ? "PrefabInstance" : name;
		auto* go = createGameObject(goName);
		prefab.apply(go);
		return go;
	}

	bool Scene::registerComponent(Component* component) {
		if (!component) return false;
		// 快照遍历：组件 onAttach 期间可能注册/移除系统，避免迭代器失效。
		// 组件不再关心"哪个系统驱动我"——由各系统通过 dynamic_cast 自行认领（支持继承）。
		auto systems = m_systems;
		bool handled = false;
		for (auto* system : systems) {
			if (system && system->onComponentAttached(component)) {
				handled = true;
			}
		}
		return handled;
	}

	void Scene::unregisterComponent(Component* component) {
		if (!component) return;
		auto systems = m_systems;
		for (auto* system : systems) {
			if (system) system->onComponentDetached(component);
		}
	}

	void Scene::removeGameObject(GameObject* gameObject) {
		if (!gameObject) {
			ST_CORE_WARN("试图从场景 {} 中移除一个空的游戏对象指针！", m_name);
			return;
		}

		if (Game::IsRunning()) { // 如果正在运行，则使用更安全的移除
			// 同时检查待添加列表（可能该对象尚未被正式加入场景）
			auto it = std::find_if(m_pendingAdditions.begin(), m_pendingAdditions.end(),
				[&gameObject](const std::unique_ptr<GameObject>& go) {
					return go.get() == gameObject;
				});
			if (it != m_pendingAdditions.end()) {
				(*it)->clean();
				m_pendingAdditions.erase(it);
				return;
			}
			gameObject->destroy(); // destroy() 会级联标记所有子物体
		}
		else {
			auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(), [&gameObject](const std::unique_ptr<GameObject>& go) {
				return go.get() == gameObject;
				});

			if (it != m_gameObjects.end()) {
				(*it)->clean();
				m_gameObjects.erase(it);
			}
			else {
				ST_CORE_WARN("场景 {} 中没有找到对应的游戏对象 ！", m_name);
			}
		}
	}

	void Scene::removeGameObjectByName(const std::string& name)
	{
		if (Game::IsRunning()) {
			// 同时检查待添加列表
			for (auto it = m_pendingAdditions.begin(); it != m_pendingAdditions.end(); ) {
				if ((*it)->getName() == name) {
					(*it)->clean();
					it = m_pendingAdditions.erase(it);
				} else {
					++it;
				}
			}
			for (auto& go : m_gameObjects) {
				if (go->getName() == name) {
					go->destroy(); // destroy() 会级联标记所有子物体
				}
			}
		}
		else {
			bool found = false;
			for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ) {
				if ((*it)->getName() == name) {
					(*it)->clean();
					it = m_gameObjects.erase(it);
					found = true;
				}
				else {
					++it;
				}
			}
			if (!found) {
				ST_CORE_WARN("没有在场景 {} 中找到名称为 {} 的游戏对象！", m_name, name);
			}
		}
	}

	void Scene::processPendingAdditions()
	{
		for (auto& go : m_pendingAdditions) {
			if(go) {
				if (go->isNeedDestroy()) {
					go->clean();
					continue;
				}
				go->setScene(this);
				m_gameObjects.push_back(std::move(go));
			}
			else ST_CORE_WARN("试图向场景 {} 中添加空游戏对象！", m_name);
		}
		m_pendingAdditions.clear();
	}

	void Scene::processPendingRemoveSystems() {
		if (m_pendingRemoveSystems.empty()) return;

		// 按下标处理前 N 条；system->destroy() 内可能 unregisterSystem 追加新条目，
		// 追加的在 N 之后，留在容器中下帧处理，避免迭代期间 vector 重分配/失效。
		const size_t count = m_pendingRemoveSystems.size();
		for (size_t i = 0; i < count; ++i) {
			const auto& type = m_pendingRemoveSystems[i];
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
		m_pendingRemoveSystems.erase(m_pendingRemoveSystems.begin(), m_pendingRemoveSystems.begin() + count);
	}
}