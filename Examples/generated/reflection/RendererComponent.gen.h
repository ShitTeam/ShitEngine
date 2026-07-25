#pragma once

#include <cstddef>
#include <RendererComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_RendererComponent() {
    const auto* base = TypeRegistry::Get("Component");
    ReflectType("RendererComponent", sizeof(RendererComponent))
        .Base(base)
        .Field("m_zIndex",
            20, 4, "int")
        .Field("m_isVisible",
            24, 1, "bool")
        .Register<RendererComponent>();
    return true;
}

} // namespace Shit
