#pragma once
#include "../Core/Core.h"
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Shit {
	class Scene;
	class GameObject;

	/**
	 * @brief 全场景序列化器 —— .scene 文件的事实标准（v2）
	 *
	 * 编辑器、Runtime、关卡切换共用同一格式与同一加载器：
	 *   编辑（toJson/fromJson）、运行（SceneManager::LoadSceneFromFile）。
	 *
	 * .scene v2 格式：
	 * ```json
	 * {
	 *   "version": 2,
	 *   "objects": [
	 *     { "name": "Play_Area", "parent": -1, "data": [ { "type": "...", "fields": {...} } ] },
	 *     { "name": "Child",     "parent": 0,  "data": [ ... ] }
	 *   ]
	 * }
	 * ```
	 * - `parent`：父对象在 objects 数组中的下标，-1 表示根对象（保存时父先于子出现）
	 * - `data`：组件数组，复用 Prefab 的反射序列化（Prefab::Capture/FromJson）
	 * - 兼容 v1：无 version / 对象无 parent 字段 → 全部按根对象加载
	 * - 相机兜底：加载后场景内无已启用相机则自动补 `game_camera`
	 */
	class SHIT_API SceneSerializer final {
	public:
		/// 序列化整个场景（含层级）为 v2 JSON。被排除的对象（如编辑器相机）不落盘，
		/// 其子对象提升为根（parent=-1）。
		static nlohmann::json toJson(Scene* scene, const std::vector<std::string>& excludeNames = {});

		/// 序列化单个对象及其子树（父先于子，parent 为数组内下标；根对象 parent=-1）——
		/// 编辑器「存为预置」的 .prefab 资产格式（与 .scene 同构，可直接 fromJson 实例化）。
		static nlohmann::json toJson(Shit::GameObject* root);

		/// 把 JSON 追加实例化进目标场景（对象 + 组件 + 层级 + 相机兜底）。
		/// 调用方负责前置清场（如编辑器"打开"先删旧对象）；
		/// 实例化失败仅跳过对应对象，不抛异常、不影响已加载部分。
		static void fromJson(const nlohmann::json& doc, Scene* scene);
	};
}