#pragma once

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
    return true;
}

} // namespace Shit
