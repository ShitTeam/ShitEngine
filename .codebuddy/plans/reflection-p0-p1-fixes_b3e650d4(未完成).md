---
name: reflection-p0-p1-fixes
overview: 修复 P0（`/dev/null` Windows 兼容性、Generator 输出格式修正为 `memberOffset` 调用），实施 P1（字段运行时读写 API、工厂创建能力、成员指针重载）。
todos:
  - id: runtime-api-extend
    content: 扩展运行时反射 API：修复 P0-1（detect_system_includes.cmake /dev/null）、实现 P1-1（FieldInfo 读写）、P1-2（TypeInfo 工厂+Create）、P1-3（TypeInfoBuilder 成员指针重载+DemangleTypeName）
    status: pending
  - id: scanner-detect-reflect
    content: 更新 Scanner 检测 SHIT_REFLECT：在 ReflectionTypes.h 的 TextMark/ReflectedType 新增 hasReflect，Scanner.cpp 新增 SHIT_REFLECT 正则匹配逻辑
    status: pending
  - id: generator-fix-output
    content: 修复 Generator 输出格式：根据 hasReflect 选择 memberOffset/成员指针 或 offsetof 语法，输出正确的 .gen.h
    status: pending
    dependencies:
      - runtime-api-extend
      - scanner-detect-reflect
  - id: regenerate-gen-files
    content: 重新生成 Engine/generated/reflection/ 下所有 .gen.h 文件，验证构建通过
    status: pending
    dependencies:
      - generator-fix-output
---


## 用户需求
修复反射系统 P0 问题，实施 P1 功能，涉及两个层面：
1. 代码生成工具（Tools/ReflectionScanner/）- Scanner 与 Generator
2. 运行时反射 API（Engine/include/ShitEngine/Reflection/ + Engine/src/ShitEngine/Reflection/）

## P0 问题

### P0-1: `/dev/null` Windows 不兼容
`detect_system_includes.cmake` 第27行使用 `INPUT_FILE /dev/null`，Windows 上该文件不存在，导致系统 include 路径检测失败，ReflectionScanner 无法找到标准库头文件。修复为平台检测：Windows 用 `NUL`，Unix 用 `/dev/null`。

### P0-2: Generator 输出格式不正确
当前 Generator 直接输出 `24, 8` 硬编码数值 offset，而 `Engine/generated/reflection/` 下现有 `.gen.h` 使用 `memberOffset(&Type::field), sizeof(Type)`。运行 Scanner 会覆盖手写文件，破坏构建。修复：Generator 输出 `memberOffset` 语法，Scanner 新增 `SHIT_REFLECT` 检测以决定使用 `memberOffset`（有 friend）还是 `offsetof`（无 friend）。

## P1 功能

### P1-1: FieldInfo 运行时读写 API
给 `FieldInfo` 添加 `GetFieldPtr()`、`GetValue()`、`SetValue()` 方法，通过 offset+size 基于 `memcpy` 实现类型擦除的字段读写，使反射系统从"只读"变为"可读写"。

### P1-2: 工厂创建能力
给 `TypeInfo` 添加 `factory` 函数指针（接收 placement memory，返回构造对象），`TypeInfoBuilder` 添加 `Factory<T>()` 模板方法，`TypeInfo` 添加 `Create(void* memory)` 方法。支持默认构造和 placement new。

### P1-3: TypeInfoBuilder 成员指针 Field 重载
添加 `.Field(name, &Type::member, typeName)` 模板重载，内部自动调用 `memberOffset` + `sizeof`，大幅简化手写注册代码和生成的 `.gen.h` 输出。



## 技术栈
- 语言: C++20
- 构建: CMake 3.20+
- 代码生成: libClang C API (ReflectionScanner)
- 运行时: 标准库 (typeindex, functional, unordered_map, list)

## 实现方案

### 整体策略
分四个阶段有序推进：先完成运行时 API 扩展（P1-1/2/3+P0-1），再更新 Scanner 检测能力，接着修复 Generator 输出格式，最后重新生成 `.gen.h` 文件。各阶段依赖关系：P1-3 成员指针重载为 Generator 输出更简洁的代码奠定基础；Scanner SHIT_REFLECT 检测是 Generator 正确选择 memberOffset/offsetof 的前提。

### P0-1: /dev/null 跨平台修复
**方案**: 在 `detect_system_includes.cmake` 顶部根据 `CMAKE_HOST_WIN32` 变量选择空设备路径：
```cmake
if(CMAKE_HOST_WIN32)
    set(NULL_DEVICE "NUL")
else()
    set(NULL_DEVICE "/dev/null")
endif()
```
然后将 `INPUT_FILE /dev/null` 改为 `INPUT_FILE ${NULL_DEVICE}`。

### P0-2: Generator 输出格式修复
**方案**: 分两步走：
1. Scanner 端 (`Scanner.cpp`)：在 `detectReflectionMarkers()` 中新增正则 `SHIT_REFLECT\s*\(\s*(\w+)\s*\)` 检测 friend 声明，在 `TextMark` 结构体和 `ReflectedType` 结构体中新增 `hasReflect` 布尔字段。
2. Generator 端 (`Generator.cpp`)：根据 `type.hasReflect` 选择输出语法：
   - 有 SHIT_REFLECT：`memberOffset(&{nsPrefix}{TypeName}::{fieldName}), sizeof({fieldType}), "{typeName}"`
   - 无 SHIT_REFLECT：`offsetof({TypeName}, {fieldName}), sizeof({fieldType}), "{typeName}"`

**性能考量**: 正则检测 O(n) 扫描行数，对源文件规模可忽略。libClang AST 解析本身是主要开销。

### P1-1: FieldInfo 读写 API
**方案**: 纯内存操作，无额外开销。在 `FieldInfo` 结构体中添加三个方法：
```cpp
void* GetFieldPtr(void* obj) const { return static_cast<char*>(obj) + offset; }
const void* GetFieldPtr(const void* obj) const { return static_cast<const char*>(obj) + offset; }
void GetValue(const void* obj, void* outBuffer) const { std::memcpy(outBuffer, GetFieldPtr(obj), size); }
void SetValue(void* obj, const void* value) const { std::memcpy(GetFieldPtr(obj), value, size); }
```
调用者需自行保证 `outBuffer`/`value` 缓冲区大小匹配 `size` 字段。

### P1-2: TypeInfo 工厂
**方案**: 以 `std::function` 存储工厂（灵活，支持 lambda 捕获），内存开销 ~32 bytes 每类型。`TypeInfoBuilder::Factory<T>()` 生成默认构造 lambda。`TypeInfo::Create(void* memory)` 当 `memory != nullptr` 时 placement new，否则 `new T()`。内置类型（int/float/string 等）不设置工厂，因为它们在 `initBuiltinTypes` 中注册且无需构造。

### P1-3: TypeInfoBuilder 成员指针重载
**方案**: 
1. 在 `TypeRegistry.h` 中添加 `DemangleTypeName(const char*)` 工具函数，封装 GCC/MinGW 的 `abi::__cxa_demangle` 和 MSVC 直通逻辑。
2. 添加两个 `Field` 模板重载：
   - `.Field(name, &Type::member)` — 自动从 `typeid(M).name()` 推导类型名
   - `.Field(name, &Type::member, typeName)` — 使用显式类型名（Generator 首选，因为 libClang 类型名更准确）
   内部调用 `memberOffset(member)` 和 `sizeof(M)` 计算。

## 目录结构
```
project-root/
├── Tools/ReflectionScanner/
│   ├── detect_system_includes.cmake    # [MODIFY] P0-1: /dev/null → ${NULL_DEVICE}
│   ├── src/ReflectionTypes.h           # [MODIFY] P0-2: TextMark + ReflectedType 新增 hasReflect
│   ├── src/Scanner.h                   # [MODIFY] P0-2: TextMark 新增 hasReflect
│   ├── src/Scanner.cpp                 # [MODIFY] P0-2: 新增 SHIT_REFLECT 正则检测
│   └── src/Generator.cpp              # [MODIFY] P0-2: 输出 memberOffset/offsetof 语法
├── Engine/
│   ├── include/ShitEngine/Reflection/
│   │   ├── TypeInfo.h                  # [MODIFY] P1-1: FieldInfo 新增读写方法；P1-2: TypeInfo 新增 factory + Create()
│   │   └── TypeRegistry.h             # [MODIFY] P1-2: TypeInfoBuilder::Factory<T>()；P1-3: Field 模板重载 + DemangleTypeName
│   ├── src/ShitEngine/Reflection/
│   │   └── TypeRegistry.cpp           # [MODIFY] P1-2: TypeInfo::Create() 实现
│   └── generated/reflection/
│       ├── Component.gen.h            # [REGENERATE] P0-2: 新 Generator 格式
│       ├── TransformComponent.gen.h   # [REGENERATE] P0-2: 新 Generator 格式
│       ├── CameraComponent.gen.h      # [REGENERATE] P0-2: 新 Generator 格式
│       ├── RendererComponent.gen.h    # [REGENERATE] P0-2: 新 Generator 格式
│       └── ReflectionRegisterAll.h    # [REGENERATE] P0-2: 新 Generator 格式
```

## 关键代码结构

### TypeInfo.h — FieldInfo 扩展
```cpp
struct FieldInfo {
    std::string name;
    size_t      offset = 0;
    size_t      size   = 0;
    std::string typeName;

    // P1-1: 运行时字段访问
    void* GetFieldPtr(void* obj) const;
    const void* GetFieldPtr(const void* obj) const;
    void  GetValue(const void* obj, void* outBuffer) const;
    void  SetValue(void* obj, const void* value) const;
};

struct TypeInfo {
    std::string  name;
    size_t       size = 0;
    const TypeInfo* baseType = nullptr;
    std::vector<FieldInfo> fields;
    std::type_index typeIndex = typeid(nullptr);

    // P1-2: 工厂创建
    std::function<void*(void*)> factory;
    void* Create(void* memory = nullptr) const;
};
```

### TypeRegistry.h — TypeInfoBuilder 扩展
```cpp
class TypeInfoBuilder {
    // ... 现有 Base/Field/Register 方法 ...

    // P1-2: 工厂注册
    template<typename T>
    TypeInfoBuilder& Factory() {
        m_info.factory = [](void* mem) -> void* {
            return mem ? ::new(mem) T() : new T();
        };
        return *this;
    }

    // P1-3: 成员指针重载
    template<typename T, typename M>
    TypeInfoBuilder& Field(const char* name, M T::*member) {
        m_info.fields.push_back({name, memberOffset(member), sizeof(M),
                                 DemangleTypeName(typeid(M).name())});
        return *this;
    }

    template<typename T, typename M>
    TypeInfoBuilder& Field(const char* name, M T::*member, const char* typeName) {
        m_info.fields.push_back({name, memberOffset(member), sizeof(M), typeName});
        return *this;
    }
};
```

### ReflectionTypes.h — 新增 hasReflect
```cpp
struct ReflectedType {
    // ... 现有字段 ...
    bool hasSourceReflect = false;  // P0-2: 源文件中是否存在 SHIT_REFLECT
};
```

### Generator.cpp — 输出逻辑变更
对于 `hasReflect == true` 的类型，Generator 输出：
```cpp
out << "        .Field(\"" << field.name << "\",\n";
out << "            &" << qualifiedTypeName << "::" << field.name << ", \"" << field.typeName << "\")\n";
```
（qualifiedTypeName 通过 `namespacePrefix(type.namespacePath) + type.name` 构造）

对于 `hasReflect == false` 的类型，Generator 回退输出：
```cpp
out << "        .Field(\"" << field.name << "\",\n";
out << "            offsetof(" << type.name << ", " << field.name << "), "
    << "sizeof(" << field.typeName << "), \"" << field.typeName << "\")\n";
```

