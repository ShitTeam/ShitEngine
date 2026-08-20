#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Scene/SceneSerializer.h"

#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/GameObject/Prefab.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Reflection/TypeRegistry.h"
#include "ShitEngine/Reflection/TypeInfo.h"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace Shit {

namespace {

using json = nlohmann::json;

constexpr int kSceneVersion = 2;

/// 默认三系统（init 自动注册，不落盘）
const std::unordered_set<std::string> kDefaultSystemNames = {
	"BehaviorSystem", "RenderSystem", "UIRenderSystem"
};

/// 把单个系统字段值序列化到 JSON（按类型名分派）
json serializeFieldValue(const FieldInfo& field, void* obj) {
	void* ptr = field.GetFieldPtr(obj);
	const std::string& tn = field.typeName;
	if (tn == "float")              return json(*reinterpret_cast<float*>(ptr));
	if (tn == "int")                return json(*reinterpret_cast<int*>(ptr));
	if (tn == "bool")               return json(*reinterpret_cast<bool*>(ptr));
	if (tn == "Vector2") {
		auto* v = reinterpret_cast<Vector2*>(ptr);
		return json::array({v->x, v->y});
	}
	if (tn == "std::string")        return json(*reinterpret_cast<std::string*>(ptr));
	if (tn == "Color") {
		// Color 实际是 glm::vec4，序列化为 [r,g,b,a]
		auto* c = reinterpret_cast<glm::vec4*>(ptr);
		return json::array({c->r, c->g, c->b, c->a});
	}
	// 枚举：按 int 值序列化
	return json(*reinterpret_cast<int*>(ptr));
}

/// 从 JSON 反序列化到系统字段
void deserializeFieldValue(const FieldInfo& field, void* obj, const json& val) {
	void* ptr = field.GetFieldPtr(obj);
	const std::string& tn = field.typeName;
	if (val.is_null()) return;
	try {
		if (tn == "float")              *reinterpret_cast<float*>(ptr) = val.get<float>();
		else if (tn == "int")           *reinterpret_cast<int*>(ptr) = val.get<int>();
		else if (tn == "bool")          *reinterpret_cast<bool*>(ptr) = val.get<bool>();
		else if (tn == "Vector2" && val.is_array() && val.size() >= 2) {
			*reinterpret_cast<Vector2*>(ptr) = {val[0].get<float>(), val[1].get<float>()};
		}
		else if (tn == "std::string")   *reinterpret_cast<std::string*>(ptr) = val.get<std::string>();
		else if (tn == "Color" && val.is_array() && val.size() >= 4) {
			*reinterpret_cast<glm::vec4*>(ptr) = {val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>()};
		}
		else {
			// 枚举：按 int 读
			*reinterpret_cast<int*>(ptr) = val.get<int>();
		}
	} catch (...) {
		ST_CORE_WARN("[SceneSerializer] 反序列化字段 {} 失败", field.name);
	}
}

/// 序列化系统实例的反射字段为 JSON 对象
json serializeSystemFields(System* system) {
	json fields = json::object();
	const TypeInfo* ti = TypeRegistry::Get(std::type_index(typeid(*system)));
	if (!ti) return fields;
	for (const auto& f : ti->fields) {
		fields[f.name] = serializeFieldValue(f, system);
	}
	return fields;
}

/// 反序列化 JSON 字段到系统实例
void deserializeSystemFields(System* system, const json& fields) {
	if (!fields.is_object()) return;
	const TypeInfo* ti = TypeRegistry::Get(std::type_index(typeid(*system)));
	if (!ti) return;
	for (const auto& f : ti->fields) {
		auto it = fields.find(f.name);
		if (it != fields.end() && !it->is_null()) {
			deserializeFieldValue(f, system, *it);
		}
	}
}

/// @brief 若场景中没有已启用的相机，补一个默认 game_camera（否则渲染不出画面）
void ensureDefaultCamera(Scene* scene) {
	if (!scene) return;

	for (auto& go : scene->getGameObjects()) {
		if (go->getName() == "scene_camera") continue;   // 编辑器相机（约定名）不算场景相机，不参与兜底判定
		if (auto* cam = go->getComponent<CameraComponent>(); cam && cam->isEnabled()) {
			return;  // 已有可用相机
		}
	}

	// 兼容：编辑器双 pass 渲染轮流改相机 enabled，旧版本可能把 game_camera
	// 序列化成禁用态——此时复用同名相机对象并启用，而不是再建一个，
	// 否则场景出现两个 game_camera 后「同一画面渲染两遍」（对象显示双份）。
	for (auto& go : scene->getGameObjects()) {
		if (go->getName() != "game_camera") continue;
		if (auto* cam = go->getComponent<CameraComponent>()) {
			cam->setEnabled(true);
			ST_CORE_WARN("[SceneSerializer] 已启用被保存为禁用态的 game_camera（复用，不新建重复相机）");
			return;
		}
	}

	auto* gc = scene->createGameObject("game_camera");
	gc->addComponent<TransformComponent>();
	gc->addComponent<CameraComponent>();
	ST_CORE_WARN("[SceneSerializer] 场景没有可用相机，已自动补 game_camera");
}

} // namespace

/// 同步场景系统列表：期望 = 默认三系统 ∪ 文件列出系统；停用多余的非默认系统，注册缺失的。
static void syncSceneSystems(const json& doc, Scene* scene) {
	if (!scene) return;
	if (!doc.is_object() || !doc.contains("systems") || !doc["systems"].is_array()) {
		// 旧文件无 systems 字段 → 不做任何改动（已存在的非默认系统保留）
		return;
	}

	// 期望集合
	std::unordered_set<std::string> desired = kDefaultSystemNames;
	for (const auto& entry : doc["systems"]) {
		if (entry.is_object() && entry.contains("type") && entry["type"].is_string()) {
			desired.insert(entry["type"].get<std::string>());
		}
	}

	// 停用当前多余的非默认系统
	for (const auto& name : scene->getRegisteredSystemTypeNames()) {
		if (desired.count(name)) continue;
		if (kDefaultSystemNames.count(name)) continue; // 默认三系统不操作
		scene->unregisterSystem(name);
	}
	// 立即处理移除队列：确保 hasSystem 不会返回已标记移除的系统（防字段恢复跳过）
	scene->flushPendingSystemRemovals();

	// 注册缺失的期望系统 + 恢复字段（含已注册的系统也要恢复字段）
	for (const auto& entry : doc["systems"]) {
		if (!entry.is_object()) continue;
		const std::string name = entry.value("type", "");
		if (name.empty() || kDefaultSystemNames.count(name)) continue;

		System* sys = nullptr;
		if (scene->hasSystem(name)) {
			sys = scene->getSystem(name);  // 已注册（如自愈的 PhysicsSystem2D），复用并恢复字段
		} else {
			sys = scene->registerSystem(name);  // 新注册
		}
		if (sys && entry.contains("fields")) {
			deserializeSystemFields(sys, entry["fields"]);
			sys->onFieldChanged("");  // 通知系统所有字段已变更
		}
	}
}

json SceneSerializer::toJson(Scene* scene, const std::vector<std::string>& excludeNames) {
	json doc;
	doc["version"] = kSceneVersion;
	json objects = json::array();
	if (!scene) return doc;

	// 被排除的对象（如编辑器相机）：自身不落盘，子对象提升为根
	std::unordered_set<GameObject*> excluded;
	for (auto& go : scene->getGameObjects()) {
		if (std::find(excludeNames.begin(), excludeNames.end(), go->getName()) != excludeNames.end())
			excluded.insert(go.get());
	}

	// 后序遍历（父先于子）DFS 分配数组下标，保证父对象的 parent 下标必然已存在
	std::unordered_map<GameObject*, int> indexOf;
	std::function<void(GameObject*)> walk = [&](GameObject* go) {
		if (excluded.count(go)) {
			for (auto* child : go->getChildren()) walk(child);  // 子对象提升为根
			return;
		}
		indexOf[go] = static_cast<int>(objects.size());

		json entry;
		entry["name"] = go->getName();
		GameObject* parent = go->getParent();
		const bool parentExcluded = parent && excluded.count(parent);
		entry["parent"] = (parent && !parentExcluded) ? indexOf.at(parent) : -1;
		entry["data"] = Prefab::Capture(go).toJson();
		objects.push_back(std::move(entry));

		for (auto* child : go->getChildren()) walk(child);
	};

	// 根 = 无父或父被排除的对象（父先于子在 DFS 中天然成立）
	for (auto& go : scene->getGameObjects()) {
		GameObject* g = go.get();
		if (excluded.count(g)) continue;
		GameObject* parent = g->getParent();
		const bool parentExcluded = parent && excluded.count(parent);
		if (!parent || parentExcluded) walk(g);
	}

doc["objects"] = std::move(objects);

		// 系统列表（排除默认三系统，为空时不写入以保持 v2 兼容）
		json systemsList = json::array();
		for (const auto& name : scene->getRegisteredSystemTypeNames()) {
			if (kDefaultSystemNames.count(name)) continue;
			json sysEntry;
			sysEntry["type"] = name;
			// 序列化反射字段（如 PhysicsSystem2D 的 m_gravity 等）
			System* sys = scene->getSystem(name);
			if (sys) {
				json fields = serializeSystemFields(sys);
				if (!fields.empty()) sysEntry["fields"] = fields;
			}
			systemsList.push_back(std::move(sysEntry));
		}
		if (!systemsList.empty()) doc["systems"] = std::move(systemsList);

		return doc;
	}

	json SceneSerializer::toJson(GameObject* root) {
	json doc;
	doc["version"] = kSceneVersion;
	json objects = json::array();
	if (!root) return doc;

	// 子树 DFS（父先于子）：根对象 parent=-1，子树内按数组下标引用
	std::unordered_map<GameObject*, int> indexOf;
	std::function<void(GameObject*)> walk = [&](GameObject* go) {
		indexOf[go] = static_cast<int>(objects.size());
		json entry;
		entry["name"] = go->getName();
		GameObject* parent = go->getParent();
		entry["parent"] = (go != root && parent) ? indexOf.at(parent) : -1;
		entry["data"] = Prefab::Capture(go).toJson();
		objects.push_back(std::move(entry));
		for (auto* child : go->getChildren()) walk(child);
	};
	walk(root);

	doc["objects"] = std::move(objects);
	return doc;
}

void SceneSerializer::fromJson(const json& doc, Scene* scene) {
		if (!scene) return;

		// 先同步系统列表（确保 PhysicsSystem2D 等系统在对象加载前已注册，自愈机制不干扰）
		syncSceneSystems(doc, scene);

		if (!doc.is_object() || !doc.contains("objects") || !doc["objects"].is_array()) {
		ST_CORE_WARN("[SceneSerializer] fromJson: 缺少 objects 数组，仅做相机兜底");
		ensureDefaultCamera(scene);
		return;
	}

	const json& objects = doc["objects"];
	std::vector<GameObject*> created;
	created.reserve(objects.size());

	// 1) 逐对象创建 + 反射组件恢复
	for (const auto& obj : objects) {
		if (!obj.is_object()) {
			ST_CORE_WARN("[SceneSerializer] 跳过非法对象条目");
			continue;
		}
		const std::string name = obj.value("name", "Object");
		auto* go = scene->createGameObject(name);
		if (obj.contains("data") && obj["data"].is_array())
			Prefab::FromJson(obj["data"]).apply(go);
		// 兜底 TransformComponent（多数系统依赖；与 Prefab::instantiate 行为一致）
		if (!go->hasComponent<TransformComponent>())
			go->addComponent<TransformComponent>();
		created.push_back(go);
	}

	// 2) 层级重建：parent = objects 数组下标（v1 文件无 parent 字段 → 根对象）
	for (size_t i = 0; i < objects.size() && i < created.size(); ++i) {
			const auto& obj = objects[i];
			int parentIdx = (obj.contains("parent") && obj["parent"].is_number())
				? obj["parent"].get<int>() : -1;
			if (parentIdx >= 0 && parentIdx < static_cast<int>(created.size()) && parentIdx != static_cast<int>(i)) {
				created[i]->setParent(created[static_cast<size_t>(parentIdx)]);
			}
		}

		// 3) 相机兜底
		ensureDefaultCamera(scene);
	}

} // namespace Shit