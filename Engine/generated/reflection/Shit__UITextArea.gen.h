#pragma once

#include <ShitEngine/UI/UITextArea.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UITextArea() {
    Shit::ReflectType("UITextArea", sizeof(UITextArea))
        .Base("UITextInput")
        .Factory<UITextArea>()
        .Register<UITextArea>();
    return true;
}

} // namespace Shit
