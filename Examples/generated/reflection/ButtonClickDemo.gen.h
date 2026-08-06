#pragma once

#include <Behaviors.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

inline bool Register_ButtonClickDemo() {
    Shit::ReflectType("ButtonClickDemo", sizeof(ButtonClickDemo))
        .Base("Behavior")
        .Field("m_buttonName",
            &ButtonClickDemo::m_buttonName, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "按钮对象名"})
        .Field("m_hintName",
            &ButtonClickDemo::m_hintName, "std::string")
        .Meta(Shit::FieldMeta{.displayName = "提示文本对象名"})
        .Field("m_clickCount",
            &ButtonClickDemo::m_clickCount, "int")
        .Meta(Shit::FieldMeta{.displayName = "点击数", .readOnly = true})
        .Factory<ButtonClickDemo>()
        .Register<ButtonClickDemo>();
    return true;
}

