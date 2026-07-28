#pragma once

#include <string>
#include <vector>
#include <typeindex>
#include <functional>
#include <cstring>

namespace Shit {

/// 字段元数据（由 SHIT_META 生成，供编辑器属性面板用）
struct RangeMeta {
    float min = 0.0f;
    float max = 0.0f;
};

struct FieldMeta {
    std::string displayName;      ///< 属性面板显示名（空 = 用字段名）
    std::string tooltip;          ///< 悬停提示
    RangeMeta   range;            ///< 数值范围（min==max 表示不限制）
    float       step = 0.0f;      ///< 步长（0 表示默认）
    std::string category;         ///< 属性分组
    bool        readOnly = false; ///< 编辑器是否只读
    std::string unit;             ///< 显示单位（如 "px"、"m/s"）
};

struct FieldInfo {
    std::string name;
    size_t      offset = 0;
    size_t      size   = 0;
    std::string typeName;
    std::vector<FieldMeta> meta;  ///< 字段元数据（编辑器属性面板用）

    // ── P1-1: 运行时字段读写 ──────────────────────────
    // 调用者自行保证缓冲区大小匹配 size 字段

    /// 获取可变对象中字段的指针（用于 in-place 修改）
    void* GetFieldPtr(void* obj) const {
        return static_cast<char*>(obj) + offset;
    }

    /// 获取 const 对象中字段的指针（只读）
    const void* GetFieldPtr(const void* obj) const {
        return static_cast<const char*>(obj) + offset;
    }

    /// 将字段值拷贝到 outBuffer（调用者确保 outBuffer 至少 size 字节）
    void GetValue(const void* obj, void* outBuffer) const {
        std::memcpy(outBuffer, static_cast<const char*>(obj) + offset, size);
    }

    /// 将 value 内容写入到字段（调用者确保 value 指向至少 size 字节的有效数据）
    void SetValue(void* obj, const void* value) const {
        std::memcpy(static_cast<char*>(obj) + offset, value, size);
    }
};

/// 枚举常量
struct EnumValue {
    std::string name;   ///< 枚举项名称（如 "None"、"Left"））
    int64_t     value;  ///< 枚举项数值
};

struct TypeInfo {
    std::string  name;
    size_t       size = 0;
    const TypeInfo* baseType = nullptr;
    std::string  baseTypeName;      ///< 基类类型名（用于延迟解析，消除 SIOF）
    std::vector<FieldInfo> fields;
    std::vector<EnumValue> enumValues;  ///< 枚举常量列表（仅枚举类型使用）

    // 用于 TypeRegistry::Get<T>() 的模板查询
    std::type_index typeIndex = typeid(nullptr);

    // ── P1-2: 工厂创建能力 ────────────────────────────
    // factory(void* memory) → void*
    //   memory != nullptr 时 placement new，否则 new T()
    std::function<void*(void*)> factory;

    /// 创建类型实例。传入 nullptr 则堆分配（new），否则 placement new 到给定地址
    void* Create(void* memory = nullptr) const {
        if (!factory) return nullptr;
        return factory(memory);
    }
};

} // namespace Shit
