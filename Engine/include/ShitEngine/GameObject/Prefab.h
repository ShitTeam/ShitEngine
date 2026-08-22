#pragma once
#include "GameObject.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Shit {

class Scene;
class TypeInfo;

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
    /// 单个组件的数据（类型名 + 持久 UUID + 反射字段值，JSON 可序列化）
    struct ComponentData {
        std::string typeName;        ///< 组件类型名（TypeRegistry::Get 查询）
        uint64_t    uuid = 0;        ///< 组件持久 ID（0 = 未记录，加载时现场分配）
        nlohmann::json fields;       ///< 字段名 → 值（仅反射且可序列化的字段）
        nlohmann::json properties;   ///< 属性名 → 值（SHIT_PROPERTY getter/setter 对，可读写者随档）
    };

    /// @brief 从现有 GameObject 捕获组件与字段（跳过 readOnly 字段与不可序列化类型）
    static Prefab Capture(GameObject* source);

    /// @brief 从 JSON 反序列化
    static Prefab FromJson(const nlohmann::json& j);

    /// @brief 序列化为 JSON
    nlohmann::json toJson() const;

    /// @brief 把此预制体应用到已有 GameObject（反射克隆；恢复记录的组件 UUID）
    void apply(GameObject* go) const;

    /// @brief 创建新 GameObject 并应用此预制体
    GameObject* instantiate(Scene* scene, const std::string& name = "") const;

    /// @brief 是否包含反射数据
    bool hasData() const { return !m_components.empty(); }

    /// @brief 已捕获的组件数据（只读，供编辑器序列化/预览）
    const std::vector<ComponentData>& getComponents() const { return m_components; }

private:
    Prefab() = default;

    /// @brief 应用组件数据；restoreUuid=true 时按记录恢复组件 UUID（场景反序列化用），
    /// false 时保留组件构造时随机分配的 UUID（运行时实例化用，防跨实例引用串线）
    void applyInternal(GameObject* go, bool restoreUuid) const;

    /// @brief 经 setter 恢复反射属性（须在 onAfterDeserialize 之后调用：
    /// 部分属性的 setter 依赖组件内部已重建的状态，如纹理加载）
    void applyProperties(const TypeInfo* ti, const ComponentData& data, Component* comp) const;

    std::vector<ComponentData> m_components;
};

} // namespace Shit
