#pragma once

#include <cstddef>
#include <ShitEngine/UI/UITextInput.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UITextInput() {
    Shit::ReflectType("UITextInput", sizeof(UITextInput))
        .Base("UIRendererComponent")
        .Field("m_text",
            &Shit::UITextInput::m_text, "std::string")
        .Meta(FieldMeta{.displayName = "Text", .tooltip = "文本内容"})
        .Field("m_placeholder",
            &Shit::UITextInput::m_placeholder, "std::string")
        .Meta(FieldMeta{.displayName = "Placeholder", .tooltip = "占位符文字"})
        .Field("m_fontPath",
            &Shit::UITextInput::m_fontPath, "std::string")
        .Meta(FieldMeta{.displayName = "Font Path", .tooltip = "字体文件路径"})
        .Field("m_fontSize",
            &Shit::UITextInput::m_fontSize, "float")
        .Meta(FieldMeta{.displayName = "Font Size", .tooltip = "字号", .range = {1, 300}})
        .Field("m_textColor",
            &Shit::UITextInput::m_textColor, "Color")
        .Meta(FieldMeta{.displayName = "Text Color", .tooltip = "文字颜色"})
        .Field("m_placeholderColor",
            &Shit::UITextInput::m_placeholderColor, "Color")
        .Meta(FieldMeta{.displayName = "Placeholder Color", .tooltip = "占位符颜色"})
        .Field("m_cursorColor",
            &Shit::UITextInput::m_cursorColor, "Color")
        .Meta(FieldMeta{.displayName = "Cursor Color", .tooltip = "光标颜色"})
        .Field("m_selectionColor",
            &Shit::UITextInput::m_selectionColor, "Color")
        .Meta(FieldMeta{.displayName = "Selection Color", .tooltip = "选区颜色"})
        .Factory<UITextInput>()
        .Register<UITextInput>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(UITextInput) == 216,
        "UITextInput: size mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_text) == 32,
        "UITextInput::m_text: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_placeholder) == 64,
        "UITextInput::m_placeholder: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_fontPath) == 96,
        "UITextInput::m_fontPath: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_fontSize) == 128,
        "UITextInput::m_fontSize: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_textColor) == 136,
        "UITextInput::m_textColor: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_placeholderColor) == 140,
        "UITextInput::m_placeholderColor: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_cursorColor) == 144,
        "UITextInput::m_cursorColor: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UITextInput, m_selectionColor) == 148,
        "UITextInput::m_selectionColor: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
