#pragma once

#include <cstddef>
#include <TransformComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_TransformComponent() {
    const auto* base = TypeRegistry::Get("Component");
    ReflectType("TransformComponent", sizeof(TransformComponent))
        .Base(base)
        .Field("m_position",
            0, 4, "int")
        .Field("m_scale",
            0, 4, "int")
        .Field("m_rotation",
            0, 4, "float")
        .Register<TransformComponent>();
    return true;
}

} // namespace Shit
