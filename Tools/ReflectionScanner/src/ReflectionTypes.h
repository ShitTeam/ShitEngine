#pragma once

#include <string>
#include <vector>

/// 反射模式：Fields（全部）/ WhiteListFields（META 白名单）
enum class ReflectionMode {
    Fields,
    WhiteListFields
};

/// 扫描器提取的单个字段信息
struct ReflectedField {
    std::string name;
    std::string typeName;
    size_t      offset  = 0;
    size_t      size    = 0;
    bool        enabled = true;
};

/// 扫描器提取的完整类型信息
struct ReflectedType {
    std::string     name;
    std::string     baseName;
    ReflectionMode  mode = ReflectionMode::Fields;
    std::vector<std::string> namespacePath;  ///< 命名空间路径，如 ["Shit"]
    std::vector<ReflectedField> fields;
    std::string     sourceFile;
    bool            hasReflect = false;       ///< 源文件是否包含 SHIT_REFLECT(Type) friend 声明
};

/// 扫描结果汇总
struct ScanResult {
    std::vector<ReflectedType> types;
    size_t totalFilesScanned = 0;
    size_t reflectedFiles    = 0;
};
