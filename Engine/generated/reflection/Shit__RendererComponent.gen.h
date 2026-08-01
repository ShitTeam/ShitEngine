#pragma once

#include <cstddef>
#include <ShitEngine/Component/RendererComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_RendererComponent() {
    Shit::ReflectType("RendererComponent", sizeof(RendererComponent))
        .Base("Component")
        .Field("m_zIndex",
            &Shit::RendererComponent::m_zIndex, "int")
        .Meta(Shit::FieldMeta{.displayName = "Z-Index", .tooltip = "渲染层级（值越大越靠上）"})
        .Field("m_isVisible",
            &Shit::RendererComponent::m_isVisible, "bool")
        .Meta(Shit::FieldMeta{.displayName = "Visible"})
        .Factory<RendererComponent>()
        .Register<RendererComponent>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(RendererComponent) == 32,
        "RendererComponent: size mismatch - regenerate reflection data");
    static_assert(offsetof(RendererComponent, m_zIndex) == 20,
        "RendererComponent::m_zIndex: offset mismatch - regenerate reflection data");
    static_assert(offsetof(RendererComponent, m_isVisible) == 24,
        "RendererComponent::m_isVisible: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
