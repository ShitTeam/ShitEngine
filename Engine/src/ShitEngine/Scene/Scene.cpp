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
#include "ShitEngine/Reflection/TypeRegistry.h"
#include "ShitEngine/Reflection/Macros.h"

namespace Shit {
	Scene::Scene(const std::string& name) : m_name(name) {
		ST_CORE_TRACE("场景 {} ：创建成功。", m_name);
	}

	Scene::~Scene() = default;

	bool Scene::hasEnabledCamera() {
		auto check = [](GameObject* go) -> bool {
			if (!go || go->getName() == "scene_camera") return false;  // 编辑器相机（约定名）不参与判定
			const auto* cam = go->getComponent<CameraComponent>();
			return cam && cam->isEnabled();
		};
		for (const auto& go : m_gameObjects)       if (check(go.get())) return true;
		for (const auto& go : m_pendingAdditions)  if (check(go.get())) return true;
		return false;
	}

	GameObject* Scene::findGameObjectByName(const std::string& name) {
		for (const auto& go : m_gameObjects) {
			if (go && go->getName() == name) return go.get();
		}
		// 运行态新建对象先进 pending 队列（帧末入列），须一并查找
		for (const auto& go : m_pendingAdditions) {
			if (go && go->getName() == name) return go.get();
		}
		return nullptr;
	}

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

		// 删除需要销毁的游戏对象：先把待销毁对象整体搬出容器（搬移阶段零回调），
		// 再逐个 clean。若在容器迭代/回调阶段删除，onDetach/onDestroy 内重入
		// removeGameObject/createGameObject 会使迭代器失效（见 AGENTS.md 约定）。
		std::vector<std::unique_ptr<GameObject>> dying;
		for (auto it = m_gameObjects.begin(); it != m_gameObjects.end();) {
			if (*it && (*it)->isNeedDestroy()) {
				dying.push_back(std::move(*it));
				it = m_gameObjects.erase(it);
			}
			else {
				++it;
			}
		}
		if (!dying.empty()) {
			for (auto& go : dying) {
				if (go) go->clean();
			}
			bumpGeneration();
		}

		processPendingAdditions();
		processPendingRemoveSystems();
	}

	void Scene::destroy() {
		// 快照清理：先把对象整体搬出容器（搬移阶段零回调），再逐个 clean。
		// onDetach/onDestroy 回调内增删对象（非运行态是立即增删）不会使迭代中的
		// 容器失效/重分配（见 AGENTS.md 约定：不要在迭代期间删除元素）。
		std::vector<std::unique_ptr<GameObject>> all;
		all.reserve(m_pendingAdditions.size() + m_gameObjects.size());
		for (auto& go : m_pendingAdditions) {
			if (go) all.push_back(std::move(go));
		}
		m_pendingAdditions.clear();
		for (auto& go : m_gameObjects) {
			if (go) all.push_back(std::move(go));
		}
		m_gameObjects.clear();

		for (auto& go : all) {
			if (go) go->clean();
		}

		// 回调期间（如 onDestroy 内）误新增的对象：回收，避免残留。
		// 下标 + 实时 size 遍历：非运行态回调可能向容器 push_back 新对象，
		// range-for 迭代器会因 vector 重分配失效（UB）。下标循环天然覆盖新增项
		// （clean 不改变容器，新对象也会被清洁，符合"全灭"语义）。
		for (size_t i = 0; i < m_pendingAdditions.size(); ++i) {
			if (m_pendingAdditions[i]) m_pendingAdditions[i]->clean();
		}
		for (size_t i = 0; i < m_gameObjects.size(); ++i) {
			if (m_gameObjects[i]) m_gameObjects[i]->clean();
		}
		m_pendingAdditions.clear();
		m_gameObjects.clear();
		bumpGeneration();

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
		m_uuidMap.clear();  // 组件已全部销毁，清空 UUID 索引

		ST_CORE_TRACE("场景 {} 已清除", m_name);
	}

void Scene::addGameObject(std::unique_ptr<GameObject>&& gameObject)
		{
			if (gameObject) {
				if (Game::IsRunning()) { // 如果游戏正在运行，则使用延时添加
					m_pendingAdditions.push_back(std::move(gameObject));
				}
				else { // 否则，直接添加
					// 先入容器再 setScene：setScene 触发 onAttach 时对象已在 m_gameObjects 中，
					// containsGameObject/getGameObjects 等查询能正确找到它
					m_gameObjects.push_back(std::move(gameObject));
					m_gameObjects.back()->setScene(this);
					bumpGeneration();
				}
			}
		else {
			ST_CORE_WARN("试图向场景 {} 中添加空游戏对象！", m_name);
		}
	}

	GameObject* Scene::createGameObject(const std::string& name) {
		auto go = std::unique_ptr<GameObject>(new GameObject(name));
		GameObject* ptr = go.get();
		if (Game::IsRunning()) {
			// 运行态：与 addGameObject 一致，先入待添加列表；setScene/onAttach 在
			// processPendingAdditions 正式入容器时执行，保证回调期间对象已在场景中。
			m_pendingAdditions.push_back(std::move(go));
		} else {
			// 非运行态：先入容器再 setScene（同 addGameObject 修复，
			// onAttach 触发时对象已在 m_gameObjects，containsGameObject 等查询正确）
			m_gameObjects.push_back(std::move(go));
			m_gameObjects.back()->setScene(this);
			bumpGeneration();
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
		indexComponentUuid(component);
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
		unindexComponentUuid(component);
		auto systems = m_systems;
		for (auto* system : systems) {
			if (system) system->onComponentDetached(component);
		}
	}

void Scene::indexComponentUuid(Component* component) {
			if (!component) return;
			// 冲突保护：随机 uuid 撞车概率极低，但 .scene 手改/重复加载可能出现
			// 同 uuid 指向不同组件——引用字段会串线，此时给后来者重发新 uuid。
			for (int guard = 0; guard < 4; ++guard) {
				const uint64_t uuid = component->getUuid();
				if (uuid == 0) return;  // 未分配（理论上构造时已分配，兜底）

				auto [it, inserted] = m_uuidMap.try_emplace(uuid, component);
				if (inserted || it->second == component) return;  // 幂等：已索引过/新插入

				ST_CORE_WARN("场景 {} 中组件 UUID 冲突 0x{:X}（目标已是 {}），为 {} 重新分配",
					m_name, uuid, it->second ? it->second->getOwner() ? it->second->getOwner()->getName() : "?" : "?",
					component->getOwner() ? component->getOwner()->getName() : "?");
				component->setUuid(GenerateComponentUuid());
			}
			// 4 次重试后仍冲突：强制插入（覆盖旧条目，避免组件在 m_uuidMap 中永久缺失）
			ST_CORE_WARN("场景 {} 组件 UUID 持续冲突，强制覆盖", m_name);
			m_uuidMap[component->getUuid()] = component;
		}

	void Scene::unindexComponentUuid(Component* component) {
		if (!component) return;
		auto it = m_uuidMap.find(component->getUuid());
		if (it != m_uuidMap.end() && it->second == component) {
			m_uuidMap.erase(it);
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
				// 先摘除再 clean：回调内重入删除其它待添加对象时不使迭代器失效
				auto go = std::move(*it);
				m_pendingAdditions.erase(it);
				go->clean();
				return;
			}
			gameObject->destroy(); // destroy() 会级联标记所有子物体
		}
		else {
			auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(), [&gameObject](const std::unique_ptr<GameObject>& go) {
				return go.get() == gameObject;
				});

			if (it != m_gameObjects.end()) {
				// 先摘除再 clean：onDetach/onDestroy 回调内若重入 removeGameObject
				//（含移除自身），容器已不含本对象，不会迭代器失效/双重清理
				auto go = std::move(*it);
				m_gameObjects.erase(it);
				bumpGeneration();
				go->clean();
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
					auto go = std::move(*it);
					it = m_pendingAdditions.erase(it);
					go->clean();
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
			// 先整体搬出再逐个 clean：clean 触发 onDetach/onDestroy 回调，回调内重入
			// removeGameObject/removeGameObjectByName 会再次 erase 容器——若在遍历中
			// clean，迭代器即失效（AGENTS.md 约定：迭代容器时不要直接删除元素）。
			std::vector<std::unique_ptr<GameObject>> dying;
			for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ) {
				if ((*it)->getName() == name) {
					dying.push_back(std::move(*it));
					it = m_gameObjects.erase(it);
					found = true;
				} else {
					++it;
				}
			}
			if (!dying.empty()) bumpGeneration();
			for (auto& go : dying) {
				if (go) go->clean();
			}
			if (!found) {
				ST_CORE_WARN("没有在场景 {} 中找到名称为 {} 的游戏对象！", m_name, name);
			}
		}
	}

	void Scene::processPendingAdditions()
	{
		if (m_pendingAdditions.empty()) return;

		// 先整体搬出再处理：onAttach 回调内若再 addGameObject（进新的
		// m_pendingAdditions）不会使迭代中的容器重分配/失效；新条目留到下帧。
		std::vector<std::unique_ptr<GameObject>> pending;
		pending.swap(m_pendingAdditions);

bool added = false;
			for (auto& go : pending) {
				if(go) {
					if (go->isNeedDestroy()) {
						go->clean();
						continue;
					}
					// 先入容器再 setScene：setScene 触发 onAttach 时对象已在 m_gameObjects 中，
					// containsGameObject/getGameObjects 等查询能正确找到它
					m_gameObjects.push_back(std::move(go));
					added = true;
					m_gameObjects.back()->setScene(this);
				}
				else ST_CORE_WARN("试图向场景 {} 中添加空游戏对象！", m_name);
			}
		if (added) bumpGeneration();
	}

	bool Scene::containsGameObject(const GameObject* gameObject) const
	{
		if (!gameObject) return false;
		for (const auto& go : m_gameObjects) {
			if (go.get() == gameObject) return true;
		}
		return false;
	}

	void Scene::processPendingRemoveSystems() {
		if (m_pendingRemoveSystems.empty()) return;

		// 按下标处理前 N 条；system->destroy() 内可能 unregisterSystem 追加新条目，
		// 追加的在 N 之后，留在容器中下帧处理，避免迭代期间 vector 重分配/失效。
		const size_t count = m_pendingRemoveSystems.size();
		for (size_t i = 0; i < count; ++i) {
			// 按值拷贝：destroy() 回调内 push_back 追加条目会使 vector 重分配，
			// 引用（const auto&）会悬垂——find 用悬垂 key 是未定义行为。
			const std::type_index type = m_pendingRemoveSystems[i];
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

		// ── 按字符串名称的系统管理 ──────────────────────────────

		/// 检查一个反射类型是否是 System 派生（沿基类链查找 "System"）
		namespace {
			bool isSystemDerived(const Shit::TypeInfo* ti) {
				for (const Shit::TypeInfo* b = ti; b; b = b->baseType)
					if (b->name == "System") return true;
				return false;
			}
		}

		System* Scene::registerSystem(const std::string& typeName) {
			const TypeInfo* ti = TypeRegistry::Get(typeName);
			if (!ti) {
				ST_CORE_WARN("[Scene] registerSystem: 类型 \"{}\" 未注册于 TypeRegistry", typeName);
				return nullptr;
			}
			if (!isSystemDerived(ti)) {
				ST_CORE_WARN("[Scene] registerSystem: \"{}\" 不是 System 派生", typeName);
				return nullptr;
			}
			if (!ti->factory) {
				ST_CORE_WARN("[Scene] registerSystem: \"{}\" 没有反射工厂（抽象类或未标记 Factory）", typeName);
				return nullptr;
			}

			// 幂等
			if (hasSystem(typeName)) return getSystem(typeName);

			System* sys = static_cast<System*>(ti->Create());
			if (!sys) {
				ST_CORE_WARN("[Scene] registerSystem: \"{}\" 工厂创建失败", typeName);
				return nullptr;
			}

			const std::type_index typeIdx = ti->typeIndex;
			sys->setScene(this);
			m_systemsMap[typeIdx] = std::unique_ptr<System>(sys);
			m_systems.push_back(sys);
			m_isSystemsNeedSort = true;
			sys->init();

			ST_CORE_DEBUG("[Scene] 已按名称注册系统 \"{}\"", typeName);
			return sys;
		}

		System* Scene::getSystem(const std::string& typeName) const {
			const TypeInfo* ti = TypeRegistry::Get(typeName);
			if (!ti) return nullptr;
			auto it = m_systemsMap.find(ti->typeIndex);
			return (it != m_systemsMap.end()) ? it->second.get() : nullptr;
		}

		bool Scene::hasSystem(const std::string& typeName) const {
			return getSystem(typeName) != nullptr;
		}

		void Scene::unregisterSystem(const std::string& typeName) {
			const TypeInfo* ti = TypeRegistry::Get(typeName);
			if (!ti) return;
			m_pendingRemoveSystems.push_back(ti->typeIndex);
		}

		std::vector<std::string> Scene::getRegisteredSystemTypeNames() const {
			std::vector<std::string> names;
			names.reserve(m_systems.size());
			for (const auto* sys : m_systems) {
				const TypeInfo* ti = TypeRegistry::Get(std::type_index(typeid(*sys)));
				if (ti) {
					names.push_back(ti->name);
				} else {
					names.push_back(Shit::DemangleTypeName(typeid(*sys).name()));
				}
			}
			return names;
		}

		void Scene::setSystemPriority(const std::string& typeName, int priority) {
			System* sys = getSystem(typeName);
			if (!sys) {
				ST_CORE_WARN("[Scene] setSystemPriority: 系统 \"{}\" 未注册", typeName);
				return;
			}
			sys->setPriority(priority);
			m_isSystemsNeedSort = true;
		}

		void Scene::unregisterSystemsBySource(const std::string& source) {
			// 收集需要移除的系统 type_index
			std::vector<std::type_index> toRemove;
			for (const auto& [typeIdx, sys] : m_systemsMap) {
				const TypeInfo* ti = TypeRegistry::Get(typeIdx);
				if (ti && ti->source == source) {
					toRemove.push_back(typeIdx);
				}
			}
			// 逐个延迟移除（防迭代中修改 map）
			for (const auto& typeIdx : toRemove) {
				m_pendingRemoveSystems.push_back(typeIdx);
			}
		}

		void Scene::flushPendingSystemRemovals() {
			processPendingRemoveSystems();
		}
	}