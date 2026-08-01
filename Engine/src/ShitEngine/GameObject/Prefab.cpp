#include "ShitEngine/Core/pch.h"
#include "ShitEngine/GameObject/Prefab.h"

#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Reflection/TypeRegistry.h"
#include "ShitEngine/Core/Log.h"

#include <cstring>

namespace Shit {

namespace {

using json = nlohmann::json;

/// @brief 字段是否编辑器只读（runtime/derived 状态，不随预制体持久化）
bool isReadOnly(const FieldInfo& field) {
    for (const auto& meta : field.meta) {
        if (meta.readOnly) return true;
    }
    return false;
}

/// @brief 把字段值转换为 JSON（支持常见数值/字符串/Vector2/Color/枚举）
json fieldToJson(const FieldInfo& field, const void* obj) {
    const void* p = field.GetFieldPtr(obj);
    const std::string& t = field.typeName;

    if (t == "float")            return json(*static_cast<const float*>(p));
    if (t == "int")              return json(*static_cast<const int*>(p));
    if (t == "unsigned int")     return json(*static_cast<const unsigned int*>(p));
    if (t == "long")             return json(*static_cast<const long*>(p));
    if (t == "unsigned long")    return json(*static_cast<const unsigned long*>(p));
    if (t == "long long")        return json(*static_cast<const long long*>(p));
    if (t == "unsigned long long") return json(*static_cast<const unsigned long long*>(p));
    if (t == "double")           return json(*static_cast<const double*>(p));
    if (t == "size_t")           return json(*static_cast<const size_t*>(p));
    if (t == "bool")             return json(*static_cast<const bool*>(p));
    if (t == "std::string")      return json(*static_cast<const std::string*>(p));
    if (t == "Vector2") {
        auto v = *static_cast<const Vector2*>(p);
        return json({ v.x, v.y });
    }
    if (t == "Color") {
        auto c = *static_cast<const Color*>(p);
        return json({ c.red, c.green, c.blue, c.alpha });
    }
    // 枚举/未知 4 字节类型：按 int32 读写（字节数一致即可正确往返）
    if (field.size == 4) {
        int32_t v = 0;
        std::memcpy(&v, p, 4);
        return json(v);
    }
    return json();  // 指针/Sprite/矩形等不可序列化类型 → 跳过
}

/// @brief 从 JSON 写回字段值
bool fieldFromJson(const FieldInfo& field, void* obj, const json& j) {
    void* p = field.GetFieldPtr(obj);
    const std::string& t = field.typeName;

    if (t == "float")            { *static_cast<float*>(p) = j.get<float>(); return true; }
    if (t == "int")              { *static_cast<int*>(p) = j.get<int>(); return true; }
    if (t == "unsigned int")     { *static_cast<unsigned int*>(p) = j.get<unsigned int>(); return true; }
    if (t == "long")             { *static_cast<long*>(p) = j.get<long>(); return true; }
    if (t == "unsigned long")    { *static_cast<unsigned long*>(p) = j.get<unsigned long>(); return true; }
    if (t == "long long")        { *static_cast<long long*>(p) = j.get<long long>(); return true; }
    if (t == "unsigned long long") { *static_cast<unsigned long long*>(p) = j.get<unsigned long long>(); return true; }
    if (t == "double")           { *static_cast<double*>(p) = j.get<double>(); return true; }
    if (t == "size_t")           { *static_cast<size_t*>(p) = j.get<size_t>(); return true; }
    if (t == "bool")             { *static_cast<bool*>(p) = j.get<bool>(); return true; }
    if (t == "std::string")      { *static_cast<std::string*>(p) = j.get<std::string>(); return true; }
    if (t == "Vector2")          { *static_cast<Vector2*>(p) = Vector2{ j[0].get<float>(), j[1].get<float>() }; return true; }
    if (t == "Color") {
        auto& c = *static_cast<Color*>(p);
        c.red = j[0].get<uint8_t>(); c.green = j[1].get<uint8_t>();
        c.blue = j[2].get<uint8_t>(); c.alpha = j[3].get<uint8_t>();
        return true;
    }
    if (field.size == 4 && j.is_number()) {
        int32_t v = j.get<int32_t>();
        std::memcpy(p, &v, 4);
        return true;
    }
    return false;
}

} // namespace

// ═══════════════════════════════════════════
// Prefab
// ═══════════════════════════════════════════

Prefab Prefab::Capture(GameObject* source) {
    Prefab p;
    if (!source) return p;

    source->forEachComponent([&](Component* comp) {
        const TypeInfo* ti = TypeRegistry::Get(std::type_index(typeid(*comp)));
        if (!ti) return;  // 未反射组件无法克隆，跳过

        ComponentData data;
        data.typeName = ti->name;
        data.fields = json::object();

        for (const auto& field : ti->fields) {
            if (isReadOnly(field)) continue;  // runtime/派生状态不入预制体
            json v = fieldToJson(field, comp);
            if (!v.is_null()) {
                data.fields[field.name] = v;
            }
        }
        p.m_components.push_back(std::move(data));
    });
    return p;
}

Prefab Prefab::FromJson(const json& j) {
    Prefab p;
    if (!j.is_array()) {
        ST_CORE_WARN("[Prefab] FromJson: 顶层不是数组，返回空预制体");
        return p;
    }
    for (const auto& entry : j) {
        if (!entry.is_object()) continue;
        ComponentData data;
        if (entry.contains("type") && entry["type"].is_string()) {
            data.typeName = entry["type"].get<std::string>();
        }
        data.fields = (entry.contains("fields") && entry["fields"].is_object())
            ? entry["fields"] : json::object();
        if (!data.typeName.empty()) {
            p.m_components.push_back(std::move(data));
        }
    }
    return p;
}

json Prefab::toJson() const {
    json j = json::array();
    for (const auto& c : m_components) {
        j.push_back({ { "type", c.typeName }, { "fields", c.fields } });
    }
    return j;
}

void Prefab::apply(GameObject* go) const {
    if (!go) return;

    // 反射工厂创建 + 字段拷贝
    for (const auto& data : m_components) {
        const TypeInfo* ti = TypeRegistry::Get(data.typeName);
        if (!ti) {
            ST_CORE_WARN("[Prefab] 实例化时找不到反射类型 {}", data.typeName);
            continue;
        }
        Component* comp = static_cast<Component*>(ti->Create());
        if (!comp) {
            ST_CORE_WARN("[Prefab] 类型 {} 无工厂（抽象类？），跳过", data.typeName);
            continue;
        }
        for (const auto& [fieldName, value] : data.fields.items()) {
            for (const auto& field : ti->fields) {
                if (field.name == fieldName) {
                    if (!fieldFromJson(field, comp, value)) {
                        ST_CORE_WARN("[Prefab] 字段 {}.{} 无法从 JSON 恢复", data.typeName, fieldName);
                    }
                    break;
                }
            }
        }
        go->addComponentInstance(comp);
    }
}

GameObject* Prefab::instantiate(Scene* scene, const std::string& name) const {
    if (!scene) return nullptr;
    auto* go = scene->createGameObject(name.empty() ? "PrefabInstance" : name);
    apply(go);
    // 兜底：确保 GameObject 有 TransformComponent（多数系统依赖）
    if (!go->hasComponent<TransformComponent>()) {
        go->addComponent<TransformComponent>();
    }
    return go;
}

} // namespace Shit
