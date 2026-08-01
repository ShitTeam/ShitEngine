#pragma once

#include <cstddef>
#include <ShitEngine/UI/UIRendererComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_UIRendererComponent() {
    Shit::ReflectType("UIRendererComponent", sizeof(UIRendererComponent))
        .Base("Component")
        .Field("m_zIndex",
            &Shit::UIRendererComponent::m_zIndex, "int")
        .Meta(Shit::FieldMeta{.displayName = "Z-Index", .tooltip = "渲染层级（值越大越靠上）"})
        .Field("m_isVisible",
            &Shit::UIRendererComponent::m_isVisible, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Visible", .tooltip = "是否参与渲染与命中"})
        .Factory<UIRendererComponent>()
        .Register<UIRendererComponent>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(UIRendererComponent) == 32,
        "UIRendererComponent: size mismatch - regenerate reflection data");
    static_assert(offsetof(UIRendererComponent, m_zIndex) == 20,
        "UIRendererComponent::m_zIndex: offset mismatch - regenerate reflection data");
    static_assert(offsetof(UIRendererComponent, m_isVisible) == 24,
        "UIRendererComponent::m_isVisible: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
