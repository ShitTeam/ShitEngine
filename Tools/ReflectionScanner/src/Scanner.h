#pragma once

#include "ReflectionTypes.h"
#include <clang-c/Index.h>

#include <string>
#include <vector>

/// 基于 libClang C API 的反射标记扫描器
///
/// 通过 AST 中的 __attribute__((annotate("shit-struct:Mode"))) /
/// annotate("shit-meta") 注解识别反射类型与字段，无需正则预扫文本，
/// 也不依赖行号匹配，支持嵌套命名空间。
class Scanner {
public:
    /// @param includePaths     -I 用户 include 路径
    /// @param systemIncludePaths -isystem 系统 include 路径
    /// @param resourceDir      libclang resource 目录（含 builtin headers），
    ///                         为空时不传 -resource-dir，由 libclang 自行查找
    explicit Scanner(const std::vector<std::string>& includePaths,
                     const std::vector<std::string>& systemIncludePaths = {},
                     const std::string& resourceDir = {});
    ~Scanner();

    ScanResult scanDirectory(const std::string& inputDir);
    bool scanFile(const std::string& filePath, ScanResult& result);

    // ├─ 工具方法（放 public 供 visitor 回调使用）─────
    static std::string getCursorSpelling(CXCursor cursor);
    static std::string getTypeSpelling(CXType type);
    static std::string getFileName(CXCursor cursor);

    /// 获取类型的完整命名空间路径
    static std::vector<std::string> getNamespacePath(CXCursor cursor);

private:
    /// 收集某个 Decl 上附着的所有 annotate 注解字符串
    struct AnnotateCtx { std::vector<std::string> annotations; };

    /// 扫描上下文：传递结果集合与当前主文件（用于去重，只收集定义在主文件中的类型）
    struct ScanCtx { ScanResult* result; CXFile mainFile; };

    /// 主 visitor：遍历 TU 找带 shit-struct/shit-class 注解的类型
    static CXChildVisitResult findReflectedTypes(CXCursor cursor, CXCursor parent,
                                                  CXClientData data);

    // AST visitor 回调（static，通过 CXClientData 传上下文）
    static CXChildVisitResult collectAnnotations(CXCursor cursor, CXCursor parent,
                                                  CXClientData data);
    static CXChildVisitResult collectFields(CXCursor cursor, CXCursor parent,
                                             CXClientData data);
    static CXChildVisitResult findBase(CXCursor cursor, CXCursor parent,
                                        CXClientData data);
    /// 检测类体内是否有 friend bool Register_Type() 声明（hasReflect）
    static CXChildVisitResult findFriendRegister(CXCursor cursor, CXCursor parent,
                                                  CXClientData data);

    struct FieldCtx  { ReflectionMode mode; std::vector<ReflectedField> fields; };
    struct BaseCtx   { std::string name; };
    struct FriendCtx { std::string expected; bool found = false; };

    std::vector<std::string> m_includeArgs;
    CXIndex m_index;
};
