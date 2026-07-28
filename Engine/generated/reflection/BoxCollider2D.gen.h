#pragma once

#include <ShitEngine/Physics/BoxCollider2D.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_BoxCollider2D() {
    Shit::ReflectType("BoxCollider2D", sizeof(BoxCollider2D))
        .Base("Component")
        .Field("m_size",
            &Shit::BoxCollider2D::m_size, "Vector2")
        .Meta(FieldMeta{.displayName = "Size", .tooltip = "碰撞盒像素尺寸（宽/高）"})
        .Field("m_density",
            &Shit::BoxCollider2D::m_density, "float")
        .Meta(FieldMeta{.displayName = "Density", .range = {0, 100}, .step = 0.1})
        .Field("m_friction",
            &Shit::BoxCollider2D::m_friction, "float")
        .Meta(FieldMeta{.displayName = "Friction", .tooltip = "摩擦系数", .range = {0, 1}, .step = 0.05})
        .Field("m_restitution",
            &Shit::BoxCollider2D::m_restitution, "float")
        .Meta(FieldMeta{.displayName = "Restitution", .tooltip = "弹性系数", .range = {0, 1}, .step = 0.05})
        .Factory<BoxCollider2D>()
        .Register<BoxCollider2D>();
    return true;
}

} // namespace Shit
