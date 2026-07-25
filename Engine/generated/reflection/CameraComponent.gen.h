#pragma once

#include <ShitEngine/Component/CameraComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_CameraComponent() {
    const auto* base = Shit::TypeRegistry::Get("Component");
    Shit::ReflectType("CameraComponent", sizeof(CameraComponent))
        .Base(base)
        .Field("m_worldSize",
            &Shit::CameraComponent::m_worldSize, "Vector2")
        .Field("m_zoom",
            &Shit::CameraComponent::m_zoom, "float")
        .Field("m_priority",
            &Shit::CameraComponent::m_priority, "int")
        .Field("m_viewportRatio",
            &Shit::CameraComponent::m_viewportRatio, "SDL_FRect")
        .Register<CameraComponent>();
    return true;
}

} // namespace Shit
