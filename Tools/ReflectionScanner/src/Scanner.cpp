#include "Scanner.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// ── 注解收集 ───────────────────────────────────────
// 属性（含 AnnotateAttr）作为被修饰 Decl 的子节点出现在 AST 中，
// clang_visitChildren 会访问到它们，这里挑出 CXCursor_AnnotateAttr 并取其字符串。
CXChildVisitResult Scanner::collectAnnotations(CXCursor cursor, CXCursor,
                                                CXClientData data) {
    auto* ctx = static_cast<AnnotateCtx*>(data);
    if (clang_getCursorKind(cursor) == CXCursor_AnnotateAttr) {
        ctx->annotations.push_back(getCursorSpelling(cursor));
    }
    return CXChildVisit_Continue;
}

// ── 枚举常量收集 ───────────────────────────────────
CXChildVisitResult Scanner::collectEnumValues(CXCursor cursor, CXCursor,
                                                CXClientData data) {
    if (clang_getCursorKind(cursor) != CXCursor_EnumConstantDecl)
        return CXChildVisit_Continue;

    auto* values = static_cast<std::vector<ReflectedEnumValue>*>(data);
    ReflectedEnumValue ev;
    ev.name  = getCursorSpelling(cursor);
    ev.value = clang_getEnumConstantDeclValue(cursor);
    values->push_back(std::move(ev));
    return CXChildVisit_Continue;
}

// ── 主 visitor：发现带 shit-struct/shit-class/shit-enum 注解的类型 ──
CXChildVisitResult Scanner::findReflectedTypes(CXCursor cursor, CXCursor,
                                                CXClientData data) {
    auto* ctx = static_cast<ScanCtx*>(data);
    CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl) {
        // 跳过前向声明，只处理有定义的类型
        if (clang_isCursorDefinition(cursor)) {
            // 只收集定义在当前主文件中的类型，跳过从 #include 引入的类型定义
            // （否则每个 include Component.h 的文件都会重复发现 Component）
            CXSourceLocation loc = clang_getCursorLocation(cursor);
            CXFile file = nullptr;
            clang_getSpellingLocation(loc, &file, nullptr, nullptr, nullptr);
            if (file != ctx->mainFile)
                return CXChildVisit_Continue;

            AnnotateCtx actx;
            clang_visitChildren(cursor, collectAnnotations, &actx);

            for (const auto& ann : actx.annotations) {
                // 匹配 "shit-reflection:Mode" / "shit-struct:Mode" / "shit-class:Mode"
                const std::string kReflect   = "shit-reflection:";
                const std::string kStruct    = "shit-struct:";
                const std::string kClass     = "shit-class:";
                bool isReflect = ann.rfind(kReflect, 0) == 0;
                bool isStruct  = ann.rfind(kStruct, 0) == 0;
                bool isClass   = ann.rfind(kClass, 0) == 0;
                if (!isReflect && !isStruct && !isClass) continue;

                std::string mode = ann.substr(
                    (isReflect ? kReflect : isStruct ? kStruct : kClass).size());

                ReflectedType type;
                type.name         = getCursorSpelling(cursor);
                type.mode         = (mode == "WhiteList" || mode == "WhiteListFields")
                                  ? ReflectionMode::WhiteList
                                  : ReflectionMode::BlackList;
                type.sourceFile   = getFileName(cursor);
                type.namespacePath = getNamespacePath(cursor);

                // 基类
                BaseCtx bctx;
                clang_visitChildren(cursor, findBase, &bctx);
                type.baseName = bctx.name;
                // 剥离基类名的命名空间前缀（如 "Shit::Component" → "Component"），
                // 使 TypeRegistry::Get(baseName) 能匹配，支持跨命名空间继承
                if (!type.baseName.empty()) {
                    auto pos = type.baseName.rfind("::");
                    if (pos != std::string::npos)
                        type.baseName = type.baseName.substr(pos + 2);
                }

                // 检测 SHIT_REFLECT(Type) → friend bool Register_Type() 是否存在。
                // 存在则用成员指针取 offset（ABI 安全），否则回退到 libclang 数值 offset。
                FriendCtx friendCtx{"Register_" + type.name, false};
                clang_visitChildren(cursor, findFriendRegister, &friendCtx);
                type.hasReflect = friendCtx.found;

                // 字段（WhiteList 时按 SHIT_META(Enable) 过滤）
                FieldCtx fieldCtx{type.mode, {}};
                clang_visitChildren(cursor, collectFields, &fieldCtx);
                type.fields = std::move(fieldCtx.fields);

                // 方法（SHIT_METHOD 标记）
                MethodCtx methodCtx{};
                clang_visitChildren(cursor, collectMethods, &methodCtx);
                type.methods = std::move(methodCtx.methods);

                // 属性（SHIT_PROPERTY 标记）
                PropertyCtx propertyCtx{};
                clang_visitChildren(cursor, collectProperties, &propertyCtx);
                type.properties = std::move(propertyCtx.properties);

                ctx->result->types.push_back(std::move(type));
                break;  // 一个类型只处理一次
            }
        }
        return CXChildVisit_Continue;  // 不递归进入类体
    }

    // ── 枚举类型 ──────────────────────────────────────
    if (kind == CXCursor_EnumDecl) {
        // 跳过前向声明
        if (clang_isCursorDefinition(cursor)) {
            CXSourceLocation loc = clang_getCursorLocation(cursor);
            CXFile file = nullptr;
            clang_getSpellingLocation(loc, &file, nullptr, nullptr, nullptr);
            if (file != ctx->mainFile)
                return CXChildVisit_Continue;

            AnnotateCtx actx;
            clang_visitChildren(cursor, collectAnnotations, &actx);

            for (const auto& ann : actx.annotations) {
                const std::string kEnum = "shit-enum:";
                if (ann.rfind(kEnum, 0) != 0) continue;

                std::string typeName = ann.substr(kEnum.size());

                ReflectedType type;
                type.name         = typeName;
                type.sourceFile   = getFileName(cursor);
                type.namespacePath = getNamespacePath(cursor);
                type.isEnum       = true;

                // 提取枚举常量
                std::vector<ReflectedEnumValue> values;
                clang_visitChildren(cursor, collectEnumValues, &values);
                type.enumValues = std::move(values);

                ctx->result->types.push_back(std::move(type));
                break;
            }
        }
        return CXChildVisit_Continue;
    }

    // 递归进入命名空间 / TU 顶层
    return (kind == CXCursor_Namespace || kind == CXCursor_TranslationUnit)
           ? CXChildVisit_Recurse
           : CXChildVisit_Continue;
}

// ── 字段收集 ───────────────────────────────────────
// ── SHIT_META 结构化原文提取（字段/方法/属性三路共用）──
// 仅结构体语法 SHIT_META(({...})) → shit-meta:({...}) 计入 metaInits；
// 简单标记（如 SHIT_META(Enable)）只影响 WhiteList 模式，不生成 FieldMeta
// 初始化器（否则会拼出 Shit::FieldMetaEnable 这类未声明标识符）。
static std::vector<std::string> collectStructuredMeta(
        const std::vector<std::string>& annotations) {
    std::vector<std::string> result;
    const std::string kMetaPrefix = "shit-meta:";
    for (const auto& ann : annotations) {
        if (ann.rfind(kMetaPrefix, 0) != 0) continue;
        std::string raw = ann.substr(kMetaPrefix.size());
        size_t s = raw.find_first_not_of(" \t\r\n");
        if (s == std::string::npos) continue;
        size_t e = raw.find_last_not_of(" \t\r\n");
        raw = raw.substr(s, e - s + 1);
        if (raw.size() >= 2 && raw.front() == '(' && raw[1] == '{') {
            result.push_back(std::move(raw));
        }
    }
    return result;
}

CXChildVisitResult Scanner::collectFields(CXCursor cursor, CXCursor,
                                           CXClientData data) {
    if (clang_getCursorKind(cursor) != CXCursor_FieldDecl)
        return CXChildVisit_Continue;

    auto* ctx = static_cast<FieldCtx*>(data);

    // 收集字段上的所有 annotate 属性
    AnnotateCtx actx;
    clang_visitChildren(cursor, collectAnnotations, &actx);

    // 字段级反射控制
    //   WhiteList → 仅 SHIT_META(Enable) 标记的字段
    //   BlackList → 跳过 SHIT_META(Disable) 标记的字段
    if (ctx->mode == ReflectionMode::WhiteList) {
        bool enabled = false;
        for (const auto& ann : actx.annotations) {
            if (ann.rfind("shit-meta:", 0) != 0) continue;
            std::string tag = ann.substr(10); // strip "shit-meta:"
            // SHIT_META(Enable) = 显式启用；结构化 SHIT_META(({…})) 不算启用
            if (tag == "Enable") { enabled = true; break; }
        }
        if (!enabled) return CXChildVisit_Continue;
    } else if (ctx->mode == ReflectionMode::BlackList) {
        for (const auto& ann : actx.annotations) {
            if (ann.rfind("shit-meta:", 0) != 0) continue;
            std::string tag = ann.substr(10);
            if (tag == "Disable") return CXChildVisit_Continue;
        }
    }

    CXType ft = clang_getCursorType(cursor);
    ReflectedField field;
    field.name     = getCursorSpelling(cursor);
    field.typeName = getTypeSpelling(ft);
    field.size     = clang_Type_getSizeOf(ft);

    long long bits = clang_Cursor_getOffsetOfField(cursor);
    if (bits != -1 && bits % 8 == 0) {
        field.offset = static_cast<size_t>(bits / 8);
    }

    field.enabled  = true;

    // ── P20: ComponentRef<T> 引用字段识别 ──
    // 类型拼写形如 "ComponentRef<Shit::UIText>"（引擎头内不带前缀）或
    // "Shit::ComponentRef<UIText>"（插件全局引用）。先剥掉全部 "Shit::"，
    // 再匹配 "ComponentRef<"，取首 '<' 与末 '>' 之间的内容为被引用类型。
    // 约束：参数必须是具体组件类型；解析失败时记 WARN 且不标记为引用（保持原行为）。
    {
        std::string normalized = field.typeName;
        for (size_t p; (p = normalized.find("Shit::")) != std::string::npos; ) {
            normalized.erase(p, 6);
        }
        const std::string kRefPrefix = "ComponentRef<";
        if (normalized.rfind(kRefPrefix, 0) == 0) {
            const size_t firstLt = normalized.find('<');
            const size_t lastGt  = normalized.rfind('>');
            if (firstLt != std::string::npos && lastGt > firstLt) {
                std::string inner = normalized.substr(firstLt + 1, lastGt - firstLt - 1);
                // trim 空白（clang 拼写一般无空白，防御性处理）
                size_t s = inner.find_first_not_of(" \t\r\n");
                size_t e = inner.find_last_not_of(" \t\r\n");
                if (s != std::string::npos) inner = inner.substr(s, e - s + 1);
                if (!inner.empty()) {
                    field.isRef       = true;
                    field.refTypeName = std::move(inner);
                }
                else {
                    std::cerr << "[Scanner] WARN: 字段 " << field.name
                              << " 的 ComponentRef<> 缺少类型参数，不标记为引用字段\n";
                }
            }
        }
    }

    // ── 捕获 SHIT_META 原文 ──
    // 仅结构体语法（如 displayName/tooltip）进 metaInits，简单标记不进（见辅助函数注释）
    field.metaInits = collectStructuredMeta(actx.annotations);

    // ── 不可序列化模板字段跳过 ──
    // 除 ComponentRef<T> 外的模板字段（std::vector<...> 等容器）在反射体系中
    // 不可序列化，BlackList 模式会无条件收录 → 在此统一跳过。
    // 注意：libclang 对解析失败的模板类型会把拼写**直接退化为 "int"** 且 size
    // 也报 4（无法与真 int 区分），该路退化由运行时兜底防御（Prefab/检查器
    // 校验 field.size == sizeof(int) 才按 int 处理；vector 实际 24 字节被跳过）。
    {
        std::string norm = field.typeName;
        for (size_t p; (p = norm.find("Shit::")) != std::string::npos; ) {
            norm.erase(p, 6);
        }
        if (norm.find('<') != std::string::npos && norm.rfind("ComponentRef<", 0) != 0) {
            return CXChildVisit_Continue;
        }
    }

    ctx->fields.push_back(std::move(field));
    return CXChildVisit_Continue;
}

// ── 方法收集 ───────────────────────────────────────
CXChildVisitResult Scanner::collectMethods(CXCursor cursor, CXCursor,
                                           CXClientData data) {
    if (clang_getCursorKind(cursor) != CXCursor_CXXMethod)
        return CXChildVisit_Continue;

    auto* ctx = static_cast<MethodCtx*>(data);

    // 收集方法上的所有 annotate 属性
    AnnotateCtx actx;
    clang_visitChildren(cursor, collectAnnotations, &actx);

    // 检查是否有 shit-method 注解
    bool hasMethodAnnotation = false;
    for (const auto& ann : actx.annotations) {
        if (ann == "shit-method") {
            hasMethodAnnotation = true;
            break;
        }
    }
    if (!hasMethodAnnotation) return CXChildVisit_Continue;

    ReflectedMethod method;
    method.name = getCursorSpelling(cursor);

    // 返回类型
    CXType returnType = clang_getCursorResultType(cursor);
    method.returnType = getTypeSpelling(returnType);

    // 参数类型：遍历方法声明的 ParmDecl 子节点（libclang 稳定 API）
    clang_visitChildren(cursor, [](CXCursor c, CXCursor, CXClientData d) -> CXChildVisitResult {
        if (clang_getCursorKind(c) == CXCursor_ParmDecl) {
            auto* out = static_cast<std::vector<std::string>*>(d);
            out->push_back(Scanner::getTypeSpelling(clang_getCursorType(c)));
        }
        return CXChildVisit_Continue;
    }, &method.paramTypes);

    // 收集 SHIT_META（仅结构化语法）
    method.metaInits = collectStructuredMeta(actx.annotations);

    ctx->methods.push_back(std::move(method));
    return CXChildVisit_Continue;
}

// ── 属性收集 ───────────────────────────────────────
CXChildVisitResult Scanner::collectProperties(CXCursor cursor, CXCursor,
                                              CXClientData data) {
    if (clang_getCursorKind(cursor) != CXCursor_CXXMethod)
        return CXChildVisit_Continue;

    auto* ctx = static_cast<PropertyCtx*>(data);

    // 记录类内每个方法的返回类型拼写（供按 getter 名解析属性类型）
    const std::string methodSpelling = getCursorSpelling(cursor);
    ctx->methodReturnTypes[methodSpelling] =
        getTypeSpelling(clang_getCursorResultType(cursor));

    // 收集方法上的所有 annotate 属性
    AnnotateCtx actx;
    clang_visitChildren(cursor, collectAnnotations, &actx);

    // 检查是否有 shit-property 注解
    for (const auto& ann : actx.annotations) {
        const std::string kPropPrefix = "shit-property:";
        if (ann.rfind(kPropPrefix, 0) != 0) continue;

        // 解析 "shit-property:Getter:Setter"
        std::string payload = ann.substr(kPropPrefix.size());
        size_t colon1 = payload.find(':');
        if (colon1 == std::string::npos) continue;

        ReflectedProperty prop;
        prop.getterName = payload.substr(0, colon1);
        prop.setterName = payload.substr(colon1 + 1);

        // 属性名从 getter 名推导（去掉 get/is 前缀，首字母小写）
        std::string getterName = prop.getterName;
        if (getterName.rfind("get", 0) == 0 && getterName.size() > 3) {
            prop.name = getterName.substr(3);
            prop.name[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(prop.name[0])));
        } else if (getterName.rfind("is", 0) == 0 && getterName.size() > 2) {
            prop.name = getterName.substr(2);
            prop.name[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(prop.name[0])));
        } else {
            prop.name = getterName;
        }

        // 属性类型：优先按 getter 名查类内方法的真实返回类型。
        // 注解要求放在 getter 声明上方，但放错位置（附着到其他声明）时
        // 被注解 cursor 的返回类型不可信，用名字查表兜底。
        auto it = ctx->methodReturnTypes.find(prop.getterName);
        prop.typeName = (it != ctx->methodReturnTypes.end())
                            ? it->second
                            : getTypeSpelling(clang_getCursorResultType(cursor));
        if (prop.typeName == "void") {
            std::cerr << "[Scanner] WARN: 属性 " << prop.name << " 的 getter "
                      << prop.getterName << " 返回 void（注解位置错误或 getter 不存在？），跳过\n";
            continue;  // 生成 void 属性会产生不可编译代码
        }

        // 收集 SHIT_META（仅结构化语法）
        prop.metaInits = collectStructuredMeta(actx.annotations);

        ctx->properties.push_back(std::move(prop));
        break;  // 一个方法只有一个 SHIT_PROPERTY 注解
    }

    return CXChildVisit_Continue;
}

CXChildVisitResult Scanner::findBase(CXCursor cursor, CXCursor, CXClientData data) {
    if (clang_getCursorKind(cursor) == CXCursor_CXXBaseSpecifier) {
        auto* ctx = static_cast<BaseCtx*>(data);
        // 只记录第一个基类（多基类时，第一个通常是主基类）
        if (ctx->name.empty()) {
            ctx->name = getTypeSpelling(clang_getCursorType(cursor));
        }
    }
    return CXChildVisit_Continue;
}

CXChildVisitResult Scanner::findFriendRegister(CXCursor cursor, CXCursor,
                                                CXClientData data) {
    auto* ctx = static_cast<FriendCtx*>(data);

    // 按拼写匹配直接子节点（不限制 cursor kind，friend 声明在不同 libclang
    // 版本下可能表现为 CXCursor_FriendDecl 或 CXCursor_FunctionDecl）
    if (getCursorSpelling(cursor) == ctx->expected) {
        ctx->found = true;
        return CXChildVisit_Break;
    }

    if (clang_getCursorKind(cursor) == CXCursor_FriendDecl) {
        // FriendDecl 的拼写可能为空，检查其引用的函数
        CXCursor ref = clang_getCursorReferenced(cursor);
        if (!clang_Cursor_isNull(ref) && getCursorSpelling(ref) == ctx->expected) {
            ctx->found = true;
            return CXChildVisit_Break;
        }
        // 遍历 FriendDecl 的子节点（FunctionDecl 可能是其子节点而非 FriendDecl 本身的拼写）
        clang_visitChildren(cursor, [](CXCursor c, CXCursor, CXClientData d) -> CXChildVisitResult {
            auto* fc = static_cast<FriendCtx*>(d);
            if (Scanner::getCursorSpelling(c) == fc->expected) {
                fc->found = true;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Continue;
        }, ctx);
        if (ctx->found) return CXChildVisit_Break;
    }

    return CXChildVisit_Continue;
}

// ── Scanner 实现 ────────────────────────────────────
Scanner::Scanner(const std::vector<std::string>& includePaths,
                 const std::vector<std::string>& systemIncludePaths,
                 const std::string& resourceDir)
    : m_index(clang_createIndex(0, 0))
{
    for (const auto& p : includePaths) {
        m_includeArgs.push_back("-I" + p);
    }
    for (const auto& p : systemIncludePaths) {
        m_includeArgs.push_back("-isystem");
        m_includeArgs.push_back(p);
    }
    // libclang 默认 target 可能是 windows-msvc（系统装了 VS Build Tools 时），但项目用
    // MinGW g++ 编译，系统 include 路径也是 MinGW 的。用 MSVC 模式解析 MinGW 头会失败
    //（__MINGW_EXTENSION / __declspec / VARARGS 等语义不兼容）。显式指定 MinGW target，
    // 让 libclang 定义 __MINGW32__ / __GNUC__ 等内置宏并启用 MinGW 兼容语义。
    m_includeArgs.push_back("-target");
    m_includeArgs.push_back("x86_64-w64-mingw32");
    // MinGW 系统头通过 _CRTIMP 等宏使用 __declspec(dllimport)，需显式启用
    m_includeArgs.push_back("-fdeclspec");
    // clang_parseTranslationUnit2 解析 .h 时默认按 C 语言，-std=c++20 会冲突
    //（"not allowed with 'C'"）触发 CXError_ASTReadError，-x c++ 强制按 C++ 解析。
    m_includeArgs.push_back("-x");
    m_includeArgs.push_back("c++");
    m_includeArgs.push_back("-std=c++20");
    m_includeArgs.push_back("-U__STRICT_ANSI__");
    m_includeArgs.push_back("-w");
    m_includeArgs.push_back("-ferror-limit=100");

    // libclang 默认从 DLL 所在目录查找 resource dir，但 DLL 被拷贝到 bin/ 后会找不到
    // builtin headers（stddef.h 等），导致 CXError_ASTReadError。
    // resource dir 由 CMake 配置期检测并通过 --resource-dir 传入，避免硬编码版本号。
    if (!resourceDir.empty()) {
        m_includeArgs.push_back("-resource-dir");
        m_includeArgs.push_back(resourceDir);
    }
}

Scanner::~Scanner() { clang_disposeIndex(m_index); }

bool Scanner::scanFile(const std::string& filePath, ScanResult& result) {
    ++result.totalFilesScanned;

    std::vector<const char*> argv;
    for (const auto& arg : m_includeArgs) argv.push_back(arg.c_str());

    CXTranslationUnit tu = nullptr;
    CXErrorCode parseErr = clang_parseTranslationUnit2(
        m_index, filePath.c_str(),
        argv.data(), (int)argv.size(),
        nullptr, 0,
        CXTranslationUnit_None, &tu);

    if (parseErr != CXError_Success || !tu) {
        std::cerr << "[reflect] parse failed: " << filePath
                  << " (err=" << parseErr << ")\n";
        ++result.parseFailedFiles;
        return false;
    }

    CXCursor tuCursor = clang_getTranslationUnitCursor(tu);
    CXFile mainFile = clang_getFile(tu, filePath.c_str());
    ScanCtx ctx{&result, mainFile};
    size_t before = result.types.size();
    clang_visitChildren(tuCursor, findReflectedTypes, &ctx);

    bool foundAny = result.types.size() > before;
    if (foundAny)
        ++result.reflectedFiles;
    else
        ++result.skippedFiles;
    clang_disposeTranslationUnit(tu);
    return foundAny;
}

ScanResult Scanner::scanDirectory(const std::string& inputDir) {
    static const char* kExtensions[] = {".h", ".hpp", ".hxx"};
    ScanResult result;
    if (!fs::exists(inputDir)) return result;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
            auto ext = entry.path().extension().string();
            for (auto* e : kExtensions) {
                if (ext == e) {
                    if (!scanFile(entry.path().string(), result)) {
                        // scanFile 内部已更新 parseFailedFiles / skippedFiles 计数器
                    }
                    break;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[reflect] filesystem error scanning directory: " << e.what() << "\n";
    }
    return result;
}

// ── 工具 ────────────────────────────────────────────
std::string Scanner::getCursorSpelling(CXCursor cursor) {
    CXString s = clang_getCursorSpelling(cursor);
    std::string r = clang_getCString(s);
    clang_disposeString(s);
    return r;
}

std::string Scanner::getTypeSpelling(CXType type) {
    CXString s = clang_getTypeSpelling(type);
    std::string r = clang_getCString(s);
    clang_disposeString(s);
    return r;
}

std::string Scanner::getFileName(CXCursor cursor) {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    CXFile file = nullptr;
    clang_getSpellingLocation(loc, &file, nullptr, nullptr, nullptr);
    if (!file) return "";
    CXString s = clang_getFileName(file);
    std::string r = clang_getCString(s);
    clang_disposeString(s);
    return r;
}

std::vector<std::string> Scanner::getNamespacePath(CXCursor cursor) {
    std::vector<std::string> namespaces;
    CXCursor parent = clang_getCursorSemanticParent(cursor);
    while (clang_getCursorKind(parent) == CXCursor_Namespace) {
        namespaces.insert(namespaces.begin(), getCursorSpelling(parent));
        parent = clang_getCursorSemanticParent(parent);
    }
    return namespaces;
}
