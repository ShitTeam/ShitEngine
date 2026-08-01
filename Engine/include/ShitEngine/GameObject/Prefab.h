#pragma once
#include "GameObject.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <vector>

namespace Shit {

class Scene;

/**
 * @brief 预制体，可重复生成相同配置的 GameObject
 *
 * 支持两种构建方式：
 *
 * ① 数据驱动（推荐，可序列化/编辑器友好）：
 *   从现有 GameObject 捕获全部反射组件及字段值，实例化时通过反射工厂 + 字段拷贝重建。
 *
 *   auto prefab = Shit::Prefab::Capture(enemyGO);
 *   // 或从 JSON 反序列化
 *   auto prefab = Shit::Prefab::FromJson(jsonData);
 *
 * ② 兼容旧用法：lambda builder（不可序列化）
 *   auto prefab = Shit::Prefab::Build([](Shit::GameObject* go) {
 *       go->addComponent<Shit::SpriteRenderer>()->setTexturePath("enemy.png");
 *   });
 *
 * 实例化：
 *   auto* enemy = scene->instantiate(prefab);   // 或 prefab.instantiate(scene)
 */
class SHIT_API Prefab {
public:
    using Builder = std::function<void(GameObject*)>;

    /// 单个组件的数据（类型名 + 反射字段值，JSON 可序列化）
    struct ComponentData {
        std::string typeName;        ///< 组件类型名（TypeRegistry::Get 查询）
        nlohmann::json fields;       ///< 字段名 → 值（仅反射且可序列化的字段）
    };

    // ── 数据驱动构建 ──────────────────────────────────
    /// @brief 从现有 GameObject 捕获组件与字段（跳过 readOnly 字段与不可序列化类型）
    static Prefab Capture(GameObject* source);

    /// @brief 从 JSON 反序列化
    static Prefab FromJson(const nlohmann::json& j);

    /// @brief 序列化为 JSON
    nlohmann::json toJson() const;

    // ── 兼容：lambda builder ──────────────────────────
    static Prefab Build(Builder builder) {
        Prefab p;
        p.m_builder = std::move(builder);
        return p;
    }

    // ── 实例化 ────────────────────────────────────────
    /// @brief 在场景中实例化（数据驱动用反射克隆；否则回退 lambda builder）
    void apply(GameObject* go) const;

    /// @brief 创建新 GameObject 并应用此预制体
    GameObject* instantiate(Scene* scene, const std::string& name = "") const;

    /// @brief 是否包含反射数据（否则为纯 lambda builder）
    bool hasData() const { return !m_components.empty(); }

    /// @brief 已捕获的组件数据（只读，供编辑器序列化/预览）
    const std::vector<ComponentData>& getComponents() const { return m_components; }

private:
    Prefab() = default;

    std::vector<ComponentData> m_components;
    Builder m_builder;
};

} // namespace Shit
