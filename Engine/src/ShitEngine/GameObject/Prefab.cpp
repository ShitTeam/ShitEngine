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

/// @brief 去掉类型名的命名空间前缀
/// 引擎头文件在 namespace Shit 内声明（如 "Vector2"）；插件全局类引用时写成
/// "Shit::Vector2"。序列化按裸类型名分派，故先归一化。
std::string_view normalizedTypeName(std::string_view type) {
    constexpr std::string_view prefix = "Shit::";
    if (type.substr(0, prefix.size()) == prefix) return type.substr(prefix.size());
    return type;
}

/// @brief 把字段值转换为 JSON（支持常见数值/字符串/Vector2/Color/枚举/组件引用）
json fieldToJson(const FieldInfo& field, const void* obj) {
    const void* p = field.GetFieldPtr(obj);
    const std::string_view t = normalizedTypeName(field.typeName);

    // P20: 组件引用字段 → 目标组件 UUID（0 = 空引用；字段内存放 8 字节 uint64）
    // 防御：ComponentRef<T> 恒为 8 字节；若扫描器误解析（嵌套模板等）出非 8 字节
    // 引用字段，回退普通未知类型处理（跳过），避免按错误宽度读写越界。
    if (field.isReference() && field.size == sizeof(uint64_t)) {
        uint64_t uuid = 0;
        std::memcpy(&uuid, p, sizeof(uuid));
        return json(uuid);
    }
    if (t == "float")            return json(*static_cast<const float*>(p));
    // "int" 分支校验 size：libclang 会把解析失败的模板字段（std::vector 等）
    // 拼写退化为 "int"，此时 size 为真实对象大小（如 24），按 4 字节读写会损坏内存。
    if (t == "int" && field.size == sizeof(int)) return json(*static_cast<const int*>(p));
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
    const std::string_view t = normalizedTypeName(field.typeName);

    // P20: 组件引用字段 → 目标组件 UUID（0 = 空引用；须为数值）
    // 与 fieldToJson 对称：仅当字段确为 8 字节引用时按 UUID 写回
    if (field.isReference() && field.size == sizeof(uint64_t)) {
        if (!j.is_number_unsigned() && !j.is_number()) return false;
        const uint64_t uuid = j.get<uint64_t>();
        std::memcpy(p, &uuid, sizeof(uuid));
        return true;
    }
    if (t == "float")            { *static_cast<float*>(p) = j.get<float>(); return true; }
    // 与 fieldToJson 对称：int 分支校验 size（防 libclang 模板字段退化 "int" 的 4 字节误写）
    if (t == "int" && field.size == sizeof(int)) { *static_cast<int*>(p) = j.get<int>(); return true; }
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
        data.uuid     = comp->getUuid();   // P20: 持久 ID 随预制体记录（引用字段寻址）
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
        // P20: 可选恢复组件 UUID（旧 .scene 无该字段 → 0，加载时现场分配）
        if (entry.contains("uuid") && entry["uuid"].is_number_unsigned()) {
            data.uuid = entry["uuid"].get<uint64_t>();
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
        j.push_back({ { "type", c.typeName }, { "uuid", c.uuid }, { "fields", c.fields } });
    }
    return j;
}

void Prefab::apply(GameObject* go) const {
    applyInternal(go, true);
}

void Prefab::applyInternal(GameObject* go, bool restoreUuid) const {
	    if (!go) return;

	    // 反射工厂创建 + 字段拷贝
	    for (const auto& data : m_components) {
	        const TypeInfo* ti = TypeRegistry::Get(data.typeName);
	        if (!ti) {
	            ST_CORE_WARN("[Prefab] 实例化时找不到反射类型 {}", data.typeName);
	            continue;
	        }

	        // 先检查 GameObject 是否已有同类型组件。若有，直接把字段写入已有组件，
	        // 避免 addComponentInstance 丢弃新实例导致字段写入丢失。
	        Component* existing = nullptr;
	        for (auto& [type, comp] : go->getComponents()) {
	            if (comp) {
	                const TypeInfo* existingTi = TypeRegistry::Get(type);
	                if (existingTi && existingTi->name == data.typeName) {
	                    existing = comp.get();
	                    break;
	                }
	            }
	        }

	        if (existing) {
	            // 字段写入已有组件
	            for (const auto& [fieldName, value] : data.fields.items()) {
	                for (const auto& field : ti->fields) {
	                    if (field.name == fieldName) {
	                        if (!fieldFromJson(field, existing, value)) {
	                            ST_CORE_WARN("[Prefab] 字段 {}.{} 无法从 JSON 恢复", data.typeName, fieldName);
	                        }
	                        break;
	                    }
	                }
	            }
	            existing->onAfterDeserialize();
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
	        // P20: 恢复持久 UUID。必须在 addComponentInstance 挂载（组件 UUID 入场景索引）
	        // 之前设置，保证索引键是最终 UUID；运行时实例化（restoreUuid=false）保留
	        // 构造时随机 ID，避免复制出的多个实例共享同一 UUID 导致引用串线。
	        if (restoreUuid && data.uuid != 0) {
	            comp->setUuid(data.uuid);
	        }
	        // 反序列化钩子：字段已直写，通知组件重建依赖引擎状态的内部数据（如纹理加载）。
	        if (Component* attached = go->addComponentInstance(comp))
	            attached->onAfterDeserialize();
	    }
	}

GameObject* Prefab::instantiate(Scene* scene, const std::string& name) const {
    if (!scene) return nullptr;
    auto* go = scene->createGameObject(name.empty() ? "PrefabInstance" : name);
    applyInternal(go, false);   // 运行时实例化：uuid 全部现场分配，防跨实例引用串线
    // 兜底：确保 GameObject 有 TransformComponent（多数系统依赖）
    if (!go->hasComponent<TransformComponent>()) {
        go->addComponent<TransformComponent>();
    }
    return go;
}

} // namespace Shit
