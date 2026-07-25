#pragma once

#include "ReflectionTypes.h"

#include <string>

/// 代码生成器：将 Scanner 结果输出为 .gen.h 注册代码
class Generator {
public:
    /// @brief 为单个反射类型生成 .gen.h 文件
    /// @return 生成的文件路径
    static std::string generateTypeFile(const ReflectedType& type, const std::string& outputDir);

    /// @brief 生成总注册头文件 ReflectionRegisterAll.h
    static std::string generateRegisterAll(const ScanResult& result, const std::string& outputDir);
};
