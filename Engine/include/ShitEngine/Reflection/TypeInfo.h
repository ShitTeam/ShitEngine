#pragma once

#include <string>
#include <vector>
#include <typeindex>
#include <functional>
#include <cstring>

namespace Shit {

struct FieldInfo {
    std::string name;
    size_t      offset = 0;
    size_t      size   = 0;
    std::string typeName;

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

struct TypeInfo {
    std::string  name;
    size_t       size = 0;
    const TypeInfo* baseType = nullptr;
    std::vector<FieldInfo> fields;

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
