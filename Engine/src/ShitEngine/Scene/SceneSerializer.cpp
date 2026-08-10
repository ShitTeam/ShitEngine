#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Scene/SceneSerializer.h"

#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/GameObject/Prefab.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Core/Log.h"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace Shit {

namespace {

using json = nlohmann::json;

constexpr int kSceneVersion = 2;

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
	return doc;
}

void SceneSerializer::fromJson(const json& doc, Scene* scene) {
	if (!scene) return;
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