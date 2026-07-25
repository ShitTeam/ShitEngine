---
name: reflection-p0-p1-fixes
overview: 修复 P0（`/dev/null` Windows 兼容性、Generator 输出 `memberOffset` 调用、去掉 REFLECTION_GENERATED 硬编码列表），实施 P1（字段运行时读写 API、工厂创建能力、成员指针重载）。
todos:
  - id: runtime-api-extend
    content: 扩展运行时反射 API：修复 P0-1（detect_system_includes.cmake /dev/null→平台空设备）、P0-3（消除 CMakeLists.txt REFLECTION_GENERATED 硬编码）、实现 P1-1（FieldInfo 读写）、P1-2（TypeInfo 工厂+Create）、P1-3（TypeInfoBuilder 成员指针重载+DemangleTypeName）
    status: completed
  - id: scanner-detect-reflect
    content: 更新 Scanner 检测 SHIT_REFLECT：在 ReflectionTypes.h 的 TextMark/ReflectedType 新增 hasReflect，Scanner.h 同步更新，Scanner.cpp 新增 SHIT_REFLECT 正则匹配并传播 hasReflect 到 ReflectedType
    status: completed
  - id: generator-fix-output
    content: 修复 Generator 输出格式：根据 hasReflect 选择成员指针语法（调用 P1-3 Field 重载）或 offsetof 回退语法，带命名空间前缀
    status: completed
    dependencies:
      - runtime-api-extend
      - scanner-detect-reflect
  - id: regenerate-gen-files
    content: 重新生成 Engine/generated/reflection/ 下所有 .gen.h 文件并验证构建通过
    status: completed
    dependencies:
      - generator-fix-output
---


## 用户需求
修复反射系统 P0 问题并实施 P1 功能，涉及代码生成工具和运行时反射 API 两个层面。

## P0 问题

### P0-1: /dev/null Windows 不兼容
`detect_system_includes.cmake` 第27行使用 `INPUT_FILE /dev/null`，Windows 上该文件不存在，导致系统 include 路径检测失败，ReflectionScanner 无法找到标准库头文件。

### P0-2: Generator 输出格式不正确
当前 Generator（`Generator.cpp` 第64-66行）直接输出 libClang 计算的数值 offset（如 `24, 8, "int"`），而 `Engine/generated/reflection/` 下现有 `.gen.h` 使用 `memberOffset(&Type::field)` 调用。两种格式不一致，且数值 offset 跨 ABI 不安全。

### P0-3: REFLECTION_GENERATED 列表硬编码
`Tools/ReflectionScanner/CMakeLists.txt` 第93-98行硬编码了所有生成文件名，新增 Component 需手动更新此列表，`add_custom_command(OUTPUT ...)` 也依赖此列表，维护脆弱。

## P1 功能

### P1-1: FieldInfo 运行时读写 API
为 `FieldInfo` 添加 `GetFieldPtr()`、`GetValue()`、`SetValue()` 方法，通过 offset+size+memcpy 实现类型擦除的字段运行时读写。

### P1-2: TypeInfo 工厂创建能力
为 `TypeInfo` 添加 `factory` 函数指针和 `Create(void*)` 方法，`TypeInfoBuilder` 添加 `Factory<T>()` 模板方法，支持默认构造和 placement new。

### P1-3: TypeInfoBuilder 成员指针 Field 重载
添加 `.Field(name, &Type::member)` 和 `.Field(name, &Type::member, typeName)` 两个模板重载，内部自动调用 `memberOffset` + `sizeof`，大幅简化手写注册代码和生成的 `.gen.h` 输出。



## 技术栈
- 语言：C++20
- 构建：CMake 3.20+
- 代码生成：libClang C API (ReflectionScanner)
- 运行时：标准库 (typeindex, functional, unordered_map, list, cstring)

## 实现方案

### 整体策略
分四个阶段有序推进。P1-3 成员指针重载为 Generator 输出更简洁代码奠定基础；Scanner SHIT_REFLECT 检测是 Generator 正确选择输出语法的前提。

### P0-1: /dev/null 跨平台修复
在 `detect_system_includes.cmake` 顶部根据 `CMAKE_HOST_WIN32` 选择空设备：
```cmake
if(CMAKE_HOST_WIN32)
    set(NULL_DEVICE "NUL")
else()
    set(NULL_DEVICE "/dev/null")
endif()
```
将 `INPUT_FILE /dev/null` 改为 `INPUT_FILE ${NULL_DEVICE}`。

### P0-2: Generator 输出格式修复
分两步：
1. Scanner 端：在 `detectReflectionMarkers()` 中新增正则 `SHIT_REFLECT\s*\(\s*(\w+)\s*\)` 检测 friend 声明。在 `TextMark` 和 `ReflectedType` 结构体中新增 `hasReflect` 布尔字段。
2. Generator 端：
   - 有 SHIT_REFLECT → 输出成员指针调用 `.Field("m_owner", &Component::m_owner, "GameObject *")`（使用 P1-3 重载）
   - 无 SHIT_REFLECT → 回退 `offsetof`：`.Field("m_owner", offsetof(Component, m_owner), sizeof(GameObject*), "GameObject *")`
   - 带命名空间 → `&Shit::TransformComponent::m_position`

### P0-3: 消除 REFLECTION_GENERATED 硬编码
删除 `Tools/ReflectionScanner/CMakeLists.txt` 中：
- 第92-98行：`REFLECTION_GENERATED` 硬编码列表
- 第100-110行：`add_custom_command(OUTPUT ${REFLECTION_GENERATED} ...)`
- 第113行改为：`add_custom_target(reflect DEPENDS run-reflectionscanner)`

`reflect` → `run-reflectionscanner` → 自动扫描所有 `.h` 生成 `.gen.h`，零硬编码。`Engine/CMakeLists.txt` 第163-166行的 `add_dependencies(ShitEngine reflect)` 无需变动。

### P1-1: FieldInfo 读写 API
纯内存操作，无额外开销。`FieldInfo` 新增四个方法（声明在 `TypeInfo.h`，inline 实现）：
```cpp
void*       GetFieldPtr(void* obj)       const;
const void* GetFieldPtr(const void* obj) const;
void        GetValue(const void* obj, void* outBuffer)  const;
void        SetValue(void* obj, const void* value)       const;
```
调用者自行保证缓冲区大小匹配 `size` 字段。

### P1-2: TypeInfo 工厂
- `TypeInfo` 新增 `std::function<void*(void*)> factory`（灵活，支持 lambda）
- `TypeInfoBuilder::Factory<T>()` 生成默认构造 lambda
- `TypeInfo::Create(void* memory = nullptr)` → 有 memory 则 placement new，否则 `new T()`
- 内置类型（int/float/string 等）不设工厂

### P1-3: 成员指针重载
- 在 `TypeRegistry.h` 中新增 `DemangleTypeName()` — 封装 GCC `abi::__cxa_demangle` / MSVC 直通
- `TypeInfoBuilder` 新增两个 `Field` 重载：自动推导版和显式类型名版（Generator 用后者）

## 目录结构
```
Tools/ReflectionScanner/
├── detect_system_includes.cmake  # [MODIFY] P0-1: /dev/null → ${NULL_DEVICE}
├── CMakeLists.txt                # [MODIFY] P0-3: 删除硬编码列表，reflect→run-reflectionscanner
└── src/
    ├── ReflectionTypes.h         # [MODIFY] P0-2: TextMark + ReflectedType 新增 hasReflect
    ├── Scanner.h                 # [MODIFY] P0-2: TextMark 新增 hasReflect
    ├── Scanner.cpp               # [MODIFY] P0-2: 新增 SHIT_REFLECT 正则检测
    └── Generator.cpp             # [MODIFY] P0-2: 输出成员指针/offsetof 语法

Engine/
├── include/ShitEngine/Reflection/
│   ├── TypeInfo.h                # [MODIFY] P1-1: FieldInfo 新增读写方法；P1-2: TypeInfo 新增 factory+Create
│   └── TypeRegistry.h           # [MODIFY] P1-2: Factory<T>()；P1-3: Field 模板重载+DemangleTypeName
├── src/ShitEngine/Reflection/
│   └── TypeRegistry.cpp         # [MODIFY] P1-2: Create() 实现
└── generated/reflection/
    ├── Component.gen.h           # [REGENERATE] 新 Generator 格式
    ├── TransformComponent.gen.h  # [REGENERATE] 新 Generator 格式
    ├── CameraComponent.gen.h     # [REGENERATE] 新 Generator 格式
    ├── RendererComponent.gen.h   # [REGENERATE] 新 Generator 格式
    └── ReflectionRegisterAll.h   # [REGENERATE] 新 Generator 格式
```

## 关键代码结构

### TypeInfo.h — FieldInfo & TypeInfo 扩展
```cpp
struct FieldInfo {
    std::string name; size_t offset = 0; size_t size = 0; std::string typeName;

    void*       GetFieldPtr(void* obj) const;
    const void* GetFieldPtr(const void* obj) const;
    void        GetValue(const void* obj, void* outBuffer) const;
    void        SetValue(void* obj, const void* value) const;
};

struct TypeInfo {
    std::string name; size_t size = 0; const TypeInfo* baseType = nullptr;
    std::vector<FieldInfo> fields; std::type_index typeIndex = typeid(nullptr);
    std::function<void*(void*)> factory;
    void* Create(void* memory = nullptr) const;
};
```

### TypeRegistry.h — TypeInfoBuilder 扩展
```cpp
class TypeInfoBuilder {
    // ... existing Base/Field/Register ...

    template<typename T> TypeInfoBuilder& Factory();

    template<typename T, typename M>
    TypeInfoBuilder& Field(const char* name, M T::*member);

    template<typename T, typename M>
    TypeInfoBuilder& Field(const char* name, M T::*member, const char* typeName);
};

// P1-3 工具函数
inline std::string DemangleTypeName(const char* mangled);
```

### ReflectionTypes.h — 新增 hasReflect
```cpp
struct ReflectedType {
    // ... 现有字段 ...
    bool hasReflect = false;
};
```

### Generator.cpp — hasReflect 分支输出
有 SHIT_REFLECT（成员指针，调用 P1-3 重载）：
```cpp
.Field("m_owner", &Component::m_owner, "GameObject *")
```
无 SHIT_REFLECT（回退 offsetof）：
```cpp
.Field("m_owner", offsetof(Component, m_owner), sizeof(GameObject*), "GameObject *")
```

