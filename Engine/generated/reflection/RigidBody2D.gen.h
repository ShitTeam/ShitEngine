#pragma once

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
    return true;
}

} // namespace Shit
