#pragma once

#include <cstddef>
#include <ShitEngine/Physics/CircleCollider2D.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_CircleCollider2D() {
    Shit::ReflectType("CircleCollider2D", sizeof(CircleCollider2D))
        .Base("Component")
        .Field("m_radius",
            &Shit::CircleCollider2D::m_radius, "float")
        .Meta(FieldMeta{.displayName = "Radius", .tooltip = "碰撞圆半径（像素）", .range = {1, 1024}, .step = 1})
        .Field("m_density",
            &Shit::CircleCollider2D::m_density, "float")
        .Meta(FieldMeta{.displayName = "Density", .range = {0, 100}, .step = 0.1})
        .Field("m_friction",
            &Shit::CircleCollider2D::m_friction, "float")
        .Meta(FieldMeta{.displayName = "Friction", .tooltip = "摩擦系数", .range = {0, 1}, .step = 0.05})
        .Field("m_restitution",
            &Shit::CircleCollider2D::m_restitution, "float")
        .Meta(FieldMeta{.displayName = "Restitution", .tooltip = "弹性系数", .range = {0, 1}, .step = 0.05})
        .Factory<CircleCollider2D>()
        .Register<CircleCollider2D>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(CircleCollider2D) == 48,
        "CircleCollider2D: size mismatch - regenerate reflection data");
    static_assert(offsetof(CircleCollider2D, m_radius) == 20,
        "CircleCollider2D::m_radius: offset mismatch - regenerate reflection data");
    static_assert(offsetof(CircleCollider2D, m_density) == 24,
        "CircleCollider2D::m_density: offset mismatch - regenerate reflection data");
    static_assert(offsetof(CircleCollider2D, m_friction) == 28,
        "CircleCollider2D::m_friction: offset mismatch - regenerate reflection data");
    static_assert(offsetof(CircleCollider2D, m_restitution) == 32,
        "CircleCollider2D::m_restitution: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
