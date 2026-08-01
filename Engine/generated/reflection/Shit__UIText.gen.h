#pragma once

#include <ShitEngine/UI/UIText.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UIText() {
    Shit::ReflectType("UIText", sizeof(UIText))
        .Base("UIRendererComponent")
        .Field("m_text",
            &Shit::UIText::m_text, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Text", .tooltip = "文字内容"})
        .Field("m_fontPath",
            &Shit::UIText::m_fontPath, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "Font Path", .tooltip = "字体文件路径"})
        .Field("m_fontSize",
            &Shit::UIText::m_fontSize, "float")
        .Meta(Shit::FieldMeta{.displayName = "Font Size", .tooltip = "字号", .range = {1, 300}})
        .Field("m_color",
            &Shit::UIText::m_color, "Color")
        .Meta(Shit::FieldMeta{.displayName = "Color", .tooltip = "文字颜色"})
        .Field("m_anchor",
            &Shit::UIText::m_anchor, "TextAnchor")
        .Meta(Shit::FieldMeta{.displayName = "Anchor", .tooltip = "对齐方式（左/中/右）"})
        .Factory<UIText>()
        .Register<UIText>();
    return true;
}

} // namespace Shit
