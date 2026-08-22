#pragma once

/**
 * @file Macros.h
 * @brief 反射标记宏（annotate 方案）——引擎反射系统的源码侧入口
 *
 * 用法总览（完整说明见各宏定义处的注释）：
 * @code
 * // 组件：BlackList = 反射全部字段（SHIT_META(Disable) 除外）
 * class SHIT_API SHIT_REFLECT(BlackList) Health : public Shit::Behavior {
 *     SHIT_REFLECT_BODY(Health)
 * public:
 *     /// 当前血量（编辑器属性面板可编辑，随 .scene 序列化）
 *     SHIT_META(({.displayName = "HP", .range = {0, 100}}))
 *     float hp = 100.0f;
 *
 *     /// 属性示例：getter 上标记一对 getter/setter，编辑器经其读写
 *     SHIT_PROPERTY(GetRatio, SetRatio)
 *     float GetRatio() const;
 *     void  SetRatio(float v);
 * };
 * @endcode
 *
 * 构建时 ReflectionScanner（libClang）扫描这些注解生成 .gen.h，
 * 经 RegisterAllReflectedTypes() 注册进 TypeRegistry——编辑器属性面板、
 * .scene 序列化、Prefab 复制粘贴均依赖此链路。改过带宏的头文件后
 * 需重扫（BUILD_TOOLS=ON 时自动）。
 */

// 反射标记宏（annotate 方案）
//
// SHIT_REFLECT(Mode)
//   放在 class/struct 关键字之后、类名之前。展开为
//   __attribute__((annotate("shit-reflection:Mode")))，
//   ReflectionScanner 通过 AST 的 CXCursor_AnnotateAttr 读取。
//   Mode: WhiteList | BlackList
//     WhiteList — 仅反射 SHIT_META(Enable) 标记的字段（opt-in）
//     BlackList — 反射全部字段，SHIT_META(Disable) 标记的除外（opt-out）
//     （向后兼容：Fields ≈ BlackList，WhiteListFields ≈ WhiteList）
//
//   用法：
//     class  SHIT_API SHIT_REFLECT(BlackList) MyComponent : Base { ... };
//     struct           SHIT_REFLECT(WhiteList) MyStruct                { ... };
//
//   展开示例（libClang 下）：
//     class  __declspec(dllexport) __attribute__((annotate("shit-reflection:BlackList")))
//     struct                          __attribute__((annotate("shit-reflection:WhiteList")))
//
// SHIT_REFLECT_BODY(Type)
//   放在类/结构体体内。展开为 friend 声明，授予生成的 Register_Type()
//   访问 private/protected 成员的权限（供成员指针取 offset，ABI 安全）。
//   注意：annotate 无法替代 friend 的运行期授权作用，必须保留。
//
// SHIT_ENUM(Type)
//   标记 enum / enum class。放在 enum/class 关键字之后、枚举名之前。
//   展开为 __attribute__((annotate("shit-enum:Type")))。
//
//   用法：
//     enum class SHIT_ENUM(MyEnum) MyEnum { A, B, C };
//
//   Scanner 遍历枚举项（CXCursor_EnumConstantDecl），提取名称与数值。
//   生成的注册代码调用 .Value("A", 0) 登记每个枚举常量。
//
// SHIT_META(...)
//   放在字段上方，为字段添加结构化元数据（供编辑器属性面板显示用）。
//   使用双括号语法：SHIT_META(({...}))
//   多条 SHIT_META 可叠加（每个生成独立的 annotate 属性）。
//
//   示例：
//     SHIT_META(({.displayName = "HP", .range = {0, 100}}))
//     SHIT_META(Enable)  ← WhiteListFields 模式的简写标记

// libClang（ReflectionScanner 使用的解析器）定义 __clang__，GCC 不定义。
// 仅在 __clang__ 下启用 annotate，避免 GCC 编译时产生
// -Wattributes "annotate attribute directive ignored" 告警。
#if defined(__clang__)
  #define SHIT_DETAIL_ANNOTATE(str) __attribute__((annotate(str)))
#else
  // 非 clang 编译器（GCC/MSVC）：annotate 退化为空。
  // 不影响 ReflectionScanner——libClang 自带解析器，即使目标编译器是 GCC
  // 也能识别 __attribute__ 语法（libClang 定义 __clang__）。
  #define SHIT_DETAIL_ANNOTATE(str)
#endif

// ── 类/结构体级反射 ────────────────────────────────
#define SHIT_REFLECT(Mode) SHIT_DETAIL_ANNOTATE("shit-reflection:" #Mode)

// ── 类内 friend 授权（必须保留） ───────────────────
#define SHIT_REFLECT_BODY(Type) friend bool Register_##Type();

// ── 枚举反射 ──────────────────────────────────────
#define SHIT_ENUM(Type) SHIT_DETAIL_ANNOTATE("shit-enum:" #Type)

// ── 字段级元数据 ──────────────────────────────────
#define SHIT_META(...) SHIT_DETAIL_ANNOTATE("shit-meta:" #__VA_ARGS__)

// ── 方法反射 ──────────────────────────────────────
// 放在方法声明上方，标记该方法可被反射系统发现。
// Scanner 遍历 CXCursor_CXXMethod 时检查此注解。
//
// 用法：
//   SHIT_METHOD
//   float getSourceRectX() const;
//
#define SHIT_METHOD SHIT_DETAIL_ANNOTATE("shit-method")

// ── 属性反射（getter/setter 对）────────────────────
// 放在 getter 方法声明上方，标记一对 getter/setter 为一个属性。
// Scanner 检测到此注解后，会查找同名 setter 方法。
//
// 用法：
//   SHIT_PROPERTY(getSourceRectX, setSourceRectX)
//   float getSourceRectX() const;
//   void setSourceRectX(float v);
//
// 属性在编辑器中渲染为可编辑字段，通过 getter/setter 读写。
// setter 为空字符串时表示只读属性。
//
#define SHIT_PROPERTY(Getter, Setter) SHIT_DETAIL_ANNOTATE("shit-property:" #Getter ":" #Setter)
