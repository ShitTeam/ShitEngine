#pragma once

// 反射标记宏（annotate 方案）
//
// SHIT_CLASS(Type, Mode) / SHIT_STRUCT(Type, Mode)
//   放在 struct/class 关键字之后、类名之前（clang 要求 annotate 属性在此位置
//   才附着到类型——放在 struct/class 关键字之前会被忽略）。展开为
//   __attribute__((annotate("shit-class:Mode"))) / ("shit-struct:Mode")，
//   ReflectionScanner 通过 AST 的 CXCursor_AnnotateAttr 读取（单一信息源，
//   不再依赖正则+行号匹配）。运行期对类型布局无任何影响。
//   Mode: Fields | WhiteListFields
//
// 用法示例：
//   struct SHIT_STRUCT(MyType, Fields) MyType { ... };
//   class  SHIT_CLASS(MyClass, Fields) SHIT_API MyClass : public Base { ... };
//
// SHIT_REFLECT(Type)
//   放在类/结构体体内。展开为 friend 声明，授予生成的 Register_Type()
//   访问 private/protected 成员的权限（供成员指针取 offset，ABI 安全）。
//   注意：必须保留，annotate 无法替代 friend 的运行期授权作用。
//
// SHIT_META(Enable)
//   放在字段上方（仅 WhiteListFields 模式需要）。展开为
//   __attribute__((annotate("shit-meta")))，附着到紧随其后的字段声明。
//   Scanner 据此判断该字段启用反射。

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

// 类型级注解：宏展开为完整的 class/struct 声明头部（含类名+注解），
// 消除类名重复。clang 要求 annotate 在 struct/class 关键字之后。
//
// 用法：
//   SHIT_API SHIT_CLASS(MyComponent, Fields) : public Base { ... };
//   SHIT_STRUCT(MyStruct, Fields)                { ... };
//
// 注意：SHIT_API（__declspec）必须在宏之前——GCC 的 -fdeclspec 不接受
// __declspec 出现在类名与 { 之间。放在 class 关键字前是 MSVC/GCC 通用位置。
#define SHIT_STRUCT(Type, Mode) struct SHIT_DETAIL_ANNOTATE("shit-struct:" #Mode) Type
#define SHIT_CLASS(Type, Mode)  class  SHIT_DETAIL_ANNOTATE("shit-class:"  #Mode) Type

// 字段级注解（仅 WhiteListFields 模式用）
#define SHIT_META(...) SHIT_DETAIL_ANNOTATE("shit-meta")

// friend 授权宏（运行期需要，不可去除）
#define SHIT_REFLECT(Type) friend bool Register_##Type();
