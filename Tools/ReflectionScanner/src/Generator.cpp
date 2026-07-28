#include "Generator.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

// ── 辅助：输出 namespace open / close ───────────────
static void openNamespaces(std::ostream& out, const std::vector<std::string>& ns) {
    for (const auto& n : ns) {
        if (n.empty()) continue;  // skip anonymous namespaces
        out << "namespace " << n << " {\n";
    }
}

static void closeNamespaces(std::ostream& out, const std::vector<std::string>& ns) {
    for (auto it = ns.rbegin(); it != ns.rend(); ++it) {
        if (it->empty()) continue;  // skip anonymous namespaces
        out << "} // namespace " << *it << "\n";
    }
}

// ── 辅助：命名空间前缀（用于调用 namespace 内的函数）─
static std::string namespacePrefix(const std::vector<std::string>& ns) {
    std::string p;
    for (const auto& n : ns) {
        if (n.empty()) continue;  // skip anonymous namespaces
        p += n + "::";
    }
    return p;
}

// ── 生成单个类型 ────────────────────────────────────
std::string Generator::generateTypeFile(const ReflectedType& type, const std::string& outputDir) {
    fs::create_directories(outputDir);

    std::string filename = type.name + ".gen.h";
    fs::path outPath = fs::path(outputDir) / filename;

    std::ofstream out(outPath);
    if (!out.is_open()) return "";

    // 构造带命名空间的限定名（如 "Shit::TransformComponent"）
    std::string qualifiedType = namespacePrefix(type.namespacePath) + type.name;

    out << "#pragma once\n\n";
    // <cstddef> 提供 offsetof 宏（用于 static_assert 编译期校验）
    out << "#include <cstddef>\n";
    // 用尖括号 include 源文件路径（已由 main.cpp 修正为相对于 includeRoot 的路径）。
    // 约束：编译此 .gen.h 时，includeRoot 必须在 -I 路径中（Engine/include 或 Examples/src），
    // 否则 #include <相对路径> 找不到源文件。CMake 的 target_include_directories 已保证。
    out << "#include <" << type.sourceFile << ">\n";
    out << "#include <ShitEngine/Reflection/TypeRegistry.h>\n\n";

    // ── 打开命名空间 ──
    openNamespaces(out, type.namespacePath);

    out << "inline bool Register_" << type.name << "() {\n";

    out << "    Shit::ReflectType(\"" << type.name << "\", sizeof(" << type.name << "))\n";

    if (type.isEnum) {
        // 枚举：登记每个枚举常量
        for (const auto& ev : type.enumValues) {
            out << "        .Value(\"" << ev.name << "\", "
                << ev.value << ")\n";
        }
    } else {
        if (!type.baseName.empty()) {
            // 按名称指定基类，Register<T>() 时延迟解析，消除 SIOF
            out << "        .Base(\"" << type.baseName << "\")\n";
        }

        for (const auto& field : type.fields) {
            out << "        .Field(\"" << field.name << "\",\n";
            if (type.hasReflect) {
                // P1-3: 成员指针重载（自动计算 offset + sizeof，ABI 安全）
                out << "            &" << qualifiedType << "::" << field.name
                    << ", \"" << field.typeName << "\")\n";
            } else {
                // 回退：libClang 数值 offset + size（无 friend 授权时无法用成员指针）
                out << "            " << field.offset << ", "
                    << field.size << ", \"" << field.typeName << "\")\n";
            }

            // SHIT_META 结构化元数据
            if (!field.metaInit.empty()) {
                std::string init = field.metaInit;
                if (init.size() >= 2 && init.front() == '(' && init.back() == ')') {
                    init = init.substr(1, init.size() - 2);
                }
                out << "        .Meta(FieldMeta" << init << ")\n";
            }
        }
    }

    if (!type.isEnum) {
        out << "        .Factory<" << type.name << ">()\n";
    }
    out << "        .Register<" << type.name << ">();\n";

    if (!type.isEnum) {
        // 仅当 Scanner 成功获取到语义正确的 type.size 时才生成静态断言
        // （libClang 对包含外部库类型如 glm::vec2 的类型可能返回 0 或 1）
        bool sizeValid = type.size > 0 && type.size != 1;
        if (sizeValid) {
            out << "\n";
            out << "    // Static assertions: regenerate if struct layout changes\n";
            out << "    static_assert(sizeof(" << type.name << ") == "
                << type.size << ",\n";
            out << "        \"" << type.name << ": size mismatch - regenerate reflection data\");\n";
        }
        for (const auto& field : type.fields) {
            if (!field.offsetValid) continue;
            if (!sizeValid) {
                // 首次输出有效断言时加上空行/标头
                out << "\n";
                sizeValid = true;
            }
            out << "    static_assert(offsetof(" << type.name << ", "
                << field.name << ") == " << field.offset << ",\n";
            out << "        \"" << type.name << "::" << field.name
                << ": offset mismatch - regenerate reflection data\");\n";
        }
    }

    out << "    return true;\n";
    out << "}\n\n";

    // ── 关闭命名空间 ──
    closeNamespaces(out, type.namespacePath);

    out.close();
    return outPath.string();
}

// ── 生成总注册头文件 ────────────────────────────────
std::string Generator::generateRegisterAll(const ScanResult& result, const std::string& outputDir) {
    fs::create_directories(outputDir);

    fs::path outPath = fs::path(outputDir) / "ReflectionRegisterAll.h";
    std::ofstream out(outPath);
    if (!out.is_open()) return "";

    out << "#pragma once\n\n";
    out << "// Auto-generated by ReflectionScanner\n";
    out << "// DO NOT EDIT\n\n";

    for (const auto& type : result.types) {
        out << "#include \"" << type.name << ".gen.h\"\n";
    }

    out << "\ninline void RegisterAllReflectedTypes() {\n";
    for (const auto& type : result.types) {
        // 用命名空间前缀调用（函数在 namespace 内部定义）
        out << "    " << namespacePrefix(type.namespacePath) << "Register_" << type.name << "();\n";
    }
    // 注册全部完成后统一解析基类引用（消除 SIOF：所有 Register_X() 都已执行）
    out << "    Shit::TypeRegistry::ResolveBases();\n";
    out << "}\n";

    out.close();
    return outPath.string();
}
