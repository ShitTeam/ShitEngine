#pragma once

#include <cstddef>
#include <ShitEngine/UI/UIButton.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UIButton() {
    Shit::ReflectType("UIButton", sizeof(UIButton))
        .Base("UIRendererComponent")
        .Field("m_interactable",
            &Shit::UIButton::m_interactable, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Interactable", .tooltip = "是否可交互"})
        .Factory<UIButton>()
        .Register<UIButton>();
    return true;
}

} // namespace Shit
