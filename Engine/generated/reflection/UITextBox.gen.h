#pragma once

#include <cstddef>
#include <ShitEngine/UI/UITextBox.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UITextBox() {
    Shit::ReflectType("UITextBox", sizeof(UITextBox))
        .Base("UITextInput")
        .Field("m_characterLimit",
            &Shit::UITextBox::m_characterLimit, "size_t")
        .Meta(Shit::FieldMeta{.displayName = "Character Limit", .tooltip = "最大字符数（0=不限）"})
        .Factory<UITextBox>()
        .Register<UITextBox>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(UITextBox) == 224,
        "UITextBox: size mismatch - regenerate reflection data");
    static_assert(offsetof(UITextBox, m_characterLimit) == 216,
        "UITextBox::m_characterLimit: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
