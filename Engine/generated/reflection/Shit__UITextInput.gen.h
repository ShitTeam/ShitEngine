#pragma once

#include <ShitEngine/UI/UITextInput.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UITextInput() {
    Shit::ReflectType("UITextInput", sizeof(UITextInput))
        .Base("UIRendererComponent")
        .Field("m_text",
            &Shit::UITextInput::m_text, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Text", .tooltip = "文本内容"})
        .Field("m_placeholder",
            &Shit::UITextInput::m_placeholder, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Placeholder", .tooltip = "占位符文字"})
        .Field("m_fontPath",
            &Shit::UITextInput::m_fontPath, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Font Path", .tooltip = "字体文件路径"})
        .Field("m_fontSize",
            &Shit::UITextInput::m_fontSize, "float")
        .Meta(Shit::FieldMeta{.displayName = "Font Size", .tooltip = "字号", .range = {1, 300}})
        .Field("m_textColor",
            &Shit::UITextInput::m_textColor, "Color")
        .Meta(Shit::FieldMeta{.displayName = "Text Color", .tooltip = "文字颜色"})
        .Field("m_placeholderColor",
            &Shit::UITextInput::m_placeholderColor, "Color")
        .Meta(Shit::FieldMeta{.displayName = "Placeholder Color", .tooltip = "占位符颜色"})
        .Field("m_cursorColor",
            &Shit::UITextInput::m_cursorColor, "Color")
        .Meta(Shit::FieldMeta{.displayName = "Cursor Color", .tooltip = "光标颜色"})
        .Field("m_selectionColor",
            &Shit::UITextInput::m_selectionColor, "Color")
        .Meta(Shit::FieldMeta{.displayName = "Selection Color", .tooltip = "选区颜色"})
        .Factory<UITextInput>()
        .Register<UITextInput>();
    return true;
}

} // namespace Shit
