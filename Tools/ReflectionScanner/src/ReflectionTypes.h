#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// 反射模式
///   WhiteList — 只反射 SHIT_META(Enable) 标记的字段（白名单，opt-in）
///   BlackList — 反射全部字段，SHIT_META(Disable) 标记的除外（黑名单，opt-out）
enum class ReflectionMode {
    WhiteList,
    BlackList
};

/// 扫描器提取的单个字段信息
struct ReflectedField {
    std::string name;
    std::string typeName;
    size_t      offset  = 0;
    size_t      size    = 0;
    bool        enabled = true;
    bool        offsetValid = false;  ///< clang 成功计算出 field offset（非 -1）
    std::vector<std::string> metaInits;  ///< 字段上所有 SHIT_META(({...})) 原文（含 {…}，每个直接嵌入 .gen.h 的 FieldMeta 初始化器）
};

/// 枚举常量（Scanner 从 CXCursor_EnumConstantDecl 提取）
struct ReflectedEnumValue {
    std::string name;    ///< 枚举项名称（如 "None"、"Dynamic"）
    int64_t     value;   ///< 枚举项数值
};

/// 扫描器提取的完整类型信息
struct ReflectedType {
    std::string     name;
    std::string     baseName;
    ReflectionMode  mode = ReflectionMode::BlackList;
    std::vector<std::string> namespacePath;  ///< 命名空间路径，如 ["Shit"]
    std::vector<ReflectedField> fields;
    std::vector<ReflectedEnumValue> enumValues;  ///< 枚举常量列表（仅枚举类型使用）
    std::string     sourceFile;
    size_t          size = 0;                 ///< 类型的 sizeof（字节数，用于 static_assert 编译期校验）
    bool            hasReflect = false;       ///< 源文件是否包含 SHIT_REFLECT_BODY(Type) friend 声明
    bool            isEnum = false;           ///< 是否为枚举类型（由 SHIT_ENUM 标记）
};

/// 扫描结果汇总
struct ScanResult {
    std::vector<ReflectedType> types;
    size_t totalFilesScanned = 0;
    size_t reflectedFiles    = 0;
    size_t parseFailedFiles  = 0;  ///< 实际解析失败数（CTE / AST 错误）
    size_t skippedFiles      = 0;  ///< 无反射标记而被跳过的文件数
};
