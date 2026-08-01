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

// ── 辅助：生成 .gen.h 文件名 ──────────────────────
// 用命名空间限定名拼文件，避免不同命名空间下的同名类型互相覆盖生成文件
// （如 Shit::AudioManager 与 Editor::AudioManager 都应生成独立的 .gen.h）。
static std::string typeFileName(const ReflectedType& type) {
    std::string qualified = namespacePrefix(type.namespacePath) + type.name;  // "Shit::Foo"
    std::string sanitized;
    for (char c : qualified) {
        sanitized += (c == ':' ? '_' : c);  // "::" → "__"
    }
    return sanitized + ".gen.h";
}

// ── 生成单个类型 ────────────────────────────────────
std::string Generator::generateTypeFile(const ReflectedType& type, const std::string& outputDir) {
    fs::create_directories(outputDir);

    std::string filename = typeFileName(type);
    fs::path outPath = fs::path(outputDir) / filename;

    std::ofstream out(outPath);
    if (!out.is_open()) return "";

    // 构造带命名空间的限定名（如 "Shit::TransformComponent"）
    std::string qualifiedType = namespacePrefix(type.namespacePath) + type.name;

    out << "#pragma once\n\n";
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

            // SHIT_META 结构化元数据（一个字段可叠加多条，全部输出）
            for (const auto& metaInit : field.metaInits) {
                std::string init = metaInit;
                if (init.size() >= 2 && init.front() == '(' && init.back() == ')') {
                    init = init.substr(1, init.size() - 2);
                }
                // 用全限定名：类型可能在全局作用域（插件类型），裸 FieldMeta 无法解析
                out << "        .Meta(Shit::FieldMeta" << init << ")\n";
            }
        }
    }

    if (!type.isEnum) {
        out << "        .Factory<" << type.name << ">()\n";
    }
    out << "        .Register<" << type.name << ">();\n";

    if (!type.isEnum) {
        // 不再生成 static_assert（offsetof/sizeof 是 ABI 相关的）：
        //   - .gen.h 随源码分发、需在 MinGW/MSVC/Clang 等多工具链下编译，
        //     libClang 按生成时目标（MinGW）算出的 offset/size 在 MSVC ABI 下必然对不上；
        //   - 大多数组件类含虚函数（非标准布局），offsetof 是"条件支持"，各编译器结果不同；
        //   - SHIT_REFLECT_BODY 成员指针路径的 offset 在运行时由 memberOffset(&T::field) 现算，
        //     本身不依赖这里的静态值，无需编译期校验。
        // 字段增删改名会使成员指针编译失败，天然兜住了"反射数据过期"。
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
        out << "#include \"" << typeFileName(type) << "\"\n";
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
