#pragma once

#include <cstddef>
#include <ShitEngine/UI/UITextArea.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UITextArea() {
    Shit::ReflectType("UITextArea", sizeof(UITextArea))
        .Base("UITextInput")
        .Factory<UITextArea>()
        .Register<UITextArea>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(UITextArea) == 224,
        "UITextArea: size mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
