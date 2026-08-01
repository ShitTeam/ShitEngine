#pragma once

#include <ShitEngine/Component/TransformComponent.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_TransformComponent() {
    Shit::ReflectType("TransformComponent", sizeof(TransformComponent))
        .Base("Component")
        .Field("m_position",
            &Shit::TransformComponent::m_position, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Position"})
        .Field("m_scale",
            &Shit::TransformComponent::m_scale, "Vector2")
        .Meta(Shit::FieldMeta{.displayName = "Scale", .range = {0, 10}, .step = 0.1})
        .Field("m_rotation",
            &Shit::TransformComponent::m_rotation, "float")
        .Meta(Shit::FieldMeta{.displayName = "Rotation", .range = {-360, 360}})
        .Factory<TransformComponent>()
        .Register<TransformComponent>();
    return true;
}

} // namespace Shit
