#pragma once
#include "GameObject.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Shit {

class Scene;

/**
 * @brief 预制体，可重复生成相同配置的 GameObject
 *
 * 数据驱动：从现有 GameObject 捕获全部反射组件及字段值，实例化时通过
 * 反射工厂 + 字段拷贝重建。支持 JSON 序列化（编辑器保存/加载预制体）。
 *
 *   auto prefab = Shit::Prefab::Capture(enemyGO);          // 捕获
 *   auto prefab = Shit::Prefab::FromJson(jsonData);        // 或从 JSON 反序列化
 *   auto* enemy = prefab.instantiate(scene, "enemy");      // 实例化
 *   auto json   = prefab.toJson();                         // 序列化
 */
class SHIT_API Prefab {
public:
    /// 单个组件的数据（类型名 + 反射字段值，JSON 可序列化）
    struct ComponentData {
        std::string typeName;        ///< 组件类型名（TypeRegistry::Get 查询）
        nlohmann::json fields;       ///< 字段名 → 值（仅反射且可序列化的字段）
    };

    /// @brief 从现有 GameObject 捕获组件与字段（跳过 readOnly 字段与不可序列化类型）
    static Prefab Capture(GameObject* source);

    /// @brief 从 JSON 反序列化
    static Prefab FromJson(const nlohmann::json& j);

    /// @brief 序列化为 JSON
    nlohmann::json toJson() const;

    /// @brief 把此预制体应用到已有 GameObject（反射克隆）
    void apply(GameObject* go) const;

    /// @brief 创建新 GameObject 并应用此预制体
    GameObject* instantiate(Scene* scene, const std::string& name = "") const;

    /// @brief 是否包含反射数据
    bool hasData() const { return !m_components.empty(); }

    /// @brief 已捕获的组件数据（只读，供编辑器序列化/预览）
    const std::vector<ComponentData>& getComponents() const { return m_components; }

private:
    Prefab() = default;

    std::vector<ComponentData> m_components;
};

} // namespace Shit
