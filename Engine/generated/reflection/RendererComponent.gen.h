#pragma once

#include <ShitEngine/Component/RendererComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_RendererComponent() {
    const auto* base = Shit::TypeRegistry::Get("Component");
    Shit::ReflectType("RendererComponent", sizeof(RendererComponent))
        .Base(base)
        .Field("m_zIndex",
            &Shit::RendererComponent::m_zIndex, "int")
        .Field("m_isVisible",
            &Shit::RendererComponent::m_isVisible, "bool")
        .Register<RendererComponent>();
    return true;
}

} // namespace Shit
