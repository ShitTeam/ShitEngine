#pragma once

#include <ShitEngine/Component/RendererComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_RendererComponent() {
    Shit::ReflectType("RendererComponent", sizeof(RendererComponent))
        .Base("Component")
        .Field("m_zIndex",
            &Shit::RendererComponent::m_zIndex, "int")
        .Meta(FieldMeta{.displayName = "Z-Index", .tooltip = "渲染层级（值越大越靠上）"})
        .Field("m_isVisible",
            &Shit::RendererComponent::m_isVisible, "bool")
        .Meta(FieldMeta{.displayName = "Visible"})
        .Factory<RendererComponent>()
        .Register<RendererComponent>();
    return true;
}

} // namespace Shit
