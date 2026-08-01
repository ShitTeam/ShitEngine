#pragma once

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
    return true;
}

} // namespace Shit
