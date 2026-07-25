#pragma once

#include <ShitEngine/Component/TransformComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_TransformComponent() {
    const auto* base = Shit::TypeRegistry::Get("Component");
    Shit::ReflectType("TransformComponent", sizeof(TransformComponent))
        .Base(base)
        .Field("m_position",
            &Shit::TransformComponent::m_position, "Vector2")
        .Field("m_scale",
            &Shit::TransformComponent::m_scale, "Vector2")
        .Field("m_rotation",
            &Shit::TransformComponent::m_rotation, "float")
        .Register<TransformComponent>();
    return true;
}

} // namespace Shit
