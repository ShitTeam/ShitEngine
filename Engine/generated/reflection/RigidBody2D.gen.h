#pragma once

#include <cstddef>
#include <ShitEngine/Physics/RigidBody2D.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

namespace Shit {
inline bool Register_RigidBody2D() {
    Shit::ReflectType("RigidBody2D", sizeof(RigidBody2D))
        .Base("Component")
        .Field("m_type",
            &Shit::RigidBody2D::m_type, "Type")
        .Meta(FieldMeta{.displayName = "Body Type", .tooltip = "Static=不动, Kinematic=用户控制, Dynamic=物理驱动"})
        .Field("m_gravityScale",
            &Shit::RigidBody2D::m_gravityScale, "float")
        .Meta(FieldMeta{.displayName = "Gravity Scale", .tooltip = "重力影响系数", .range = {0, 10}, .step = 0.1})
        .Field("m_linearDamping",
            &Shit::RigidBody2D::m_linearDamping, "float")
        .Meta(FieldMeta{.displayName = "Linear Damping", .tooltip = "线性阻尼", .range = {0, 10}, .step = 0.1})
        .Field("m_fixedRotation",
            &Shit::RigidBody2D::m_fixedRotation, "bool")
        .Meta(FieldMeta{.displayName = "Fixed Rotation", .tooltip = "锁定旋转"})
        .Factory<RigidBody2D>()
        .Register<RigidBody2D>();

    // Static assertions: regenerate if struct layout changes
    static_assert(sizeof(RigidBody2D) == 48,
        "RigidBody2D: size mismatch - regenerate reflection data");
    static_assert(offsetof(RigidBody2D, m_type) == 32,
        "RigidBody2D::m_type: offset mismatch - regenerate reflection data");
    static_assert(offsetof(RigidBody2D, m_gravityScale) == 36,
        "RigidBody2D::m_gravityScale: offset mismatch - regenerate reflection data");
    static_assert(offsetof(RigidBody2D, m_linearDamping) == 40,
        "RigidBody2D::m_linearDamping: offset mismatch - regenerate reflection data");
    static_assert(offsetof(RigidBody2D, m_fixedRotation) == 44,
        "RigidBody2D::m_fixedRotation: offset mismatch - regenerate reflection data");
    return true;
}

} // namespace Shit
