#pragma once

#include <cstddef>
#include <CameraComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_CameraComponent() {
    const auto* base = TypeRegistry::Get("Component");
    ReflectType("CameraComponent", sizeof(CameraComponent))
        .Base(base)
        .Field("m_worldSize",
            0, 4, "int")
        .Field("m_zoom",
            0, 4, "float")
        .Field("m_priority",
            0, 4, "int")
        .Field("m_viewportRatio",
            0, 4, "int")
        .Register<CameraComponent>();
    return true;
}

} // namespace Shit
